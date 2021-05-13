#include <iostream>
#include <fstream>
#include<sstream>
#include <map>
#include <vector>
#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <algorithm>
#include <set>

#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>


using namespace std;
const char* SERVERA_PORTNO = "30296";
const char* MAIN_SERV_PORTNO = "32296";


void error(string err_msg) {
    cerr << err_msg << endl;
    exit(1);
}

char* serialize(vector<string> country_list) {
    unsigned int total_count = 0;

    for(int i = 0; i < country_list.size(); i++ ) {
        // cout << v[i]<< endl;
        total_count += country_list[i].length() + 1;
    }

    char *buffer = new char[total_count];

    int idx = 0;

    for(int i = 0; i < country_list.size(); i++ ) {
        string s = country_list[i];
        for (int j = 0; j < s.size(); j ++ ) {
            buffer[idx++] = s[j];
        }
        buffer[idx ++] = ' ';
    }

    return buffer;
}

int main() {
    int sockfd, portno_A, portno_mainserv;
        
    char buffer[2048], country_name[2048], userid[2048];
    struct sockaddr_in servA_addr,mainserv_addr;
    socklen_t len;
    int n, n1, n2;
    
    cout << "Server A is up and running using UDP on port 30296" << endl;

    /*
    Bootup
    */

    // read file
    ifstream infile; 
    infile.open("dataA.txt");
    map<string, vector<vector<string> > > myMap;
    string data;
    string country;
    vector<string> country_list;    
    vector<vector<string> > interest_groups;
    

    while(getline(infile,data)) {
        if(isalpha(data[0]) != 0) {
            // is country
            country = data;
            myMap[country];
            country_list.push_back(country);
            // cout << "country: " << country << endl;
            interest_groups.clear();
            
        } else if (isdigit(data[0]) != 0) {
            // is number
            vector<string> user_ids;
            stringstream input(data);
            string user_id;
            while(input >> user_id){
                user_ids.push_back(user_id);
            }
            interest_groups.push_back(user_ids);
            // cout << "interest group " << interest_groups.size() -1 << " has " << user_ids.size() << " users" << endl;
            myMap[country] = interest_groups;
            // cout << "country " << country << " interest group size :" << myMap[country].size() << endl;
        } 
    }



    //UDP Socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if(sockfd < 0) {
        error("socket creation failed");
    }

    memset(&servA_addr, 0, sizeof(servA_addr));
    memset(&mainserv_addr, 0, sizeof(mainserv_addr));
    portno_A = atoi(SERVERA_PORTNO); 
    portno_mainserv = atoi(MAIN_SERV_PORTNO);

    mainserv_addr.sin_family       = AF_INET;
    mainserv_addr.sin_addr.s_addr = INADDR_ANY;
    mainserv_addr.sin_port         = htons(portno_mainserv);

    servA_addr.sin_family       = AF_INET;
    servA_addr.sin_addr.s_addr = INADDR_ANY;
    servA_addr.sin_port         = htons(portno_A);

    if ( bind(sockfd, (struct sockaddr *)&servA_addr, 
            sizeof(servA_addr)) < 0 )
    {
        error("ERROR on binding");
    }

    len = sizeof(mainserv_addr);

    n = recvfrom(sockfd, buffer, 2048, 0, (struct sockaddr *) &mainserv_addr,
        &len);

    if (n < 0) {
        error("ERROR on recvfrom");
    }

    buffer[n] = '\0';

    char *country_list_to_mainserv = serialize(country_list);

    sendto(sockfd, (const char *)country_list_to_mainserv, strlen(country_list_to_mainserv), 
        0, (const struct sockaddr *) &mainserv_addr, len);
    printf("Server A has sent a country list to Main Server.\n"); 

    while(1) {

        n2 = recvfrom(sockfd, userid, 2048, 
                0, (struct sockaddr *) &mainserv_addr,
                &len);

        n1 = recvfrom(sockfd, country_name, 2048, 
                0, (struct sockaddr *) &mainserv_addr,
                &len);

        if (n1 < 0 || n2 < 0) {
            error("ERROR on recvfrom");
        }

        country_name[n1] = '\0';
        userid[n2] = '\0';

        cout << "Server A has received a request for finding possible friends of User " 
            << userid << " in " << country_name << endl;

        vector<vector<string> > result = myMap[country_name];
        set<string> friend_rec;
        int lookup_flag = 0;
        for(int i=0; i < result.size(); i++) {
            vector<string> ids = result[i];
            for(int j=0; j < ids.size(); j++) {
                if (ids[j].compare(string(userid)) == 0) {
                    cout << "start building set" << endl;
                    lookup_flag = 1;
                    for(int k=0; k < ids.size(); k++) {
                        friend_rec.insert(ids[k]);
                    }
                    friend_rec.erase(string(userid));
                }
            }
        }

        if(lookup_flag == 0) {
            // user id not found
            cout << "User " << userid << " does not show up in " << country_name << endl;
            const char* not_found = "user not found";
            sendto(sockfd, not_found, strlen(not_found), 
                0, (const struct sockaddr *) &mainserv_addr, len);

            cout << "The server A has sent User " << userid << " not found to Main Server" << endl;
        } else {
            // user id found
            vector<string> f_rec_vec(friend_rec.size());
            copy(friend_rec.begin(), friend_rec.end(), f_rec_vec.begin());
            cout << "Server A found the following possible friends for User " << userid
                << " in " << country_name << ": " << endl;

            for(int i = 0; i < f_rec_vec.size() - 1; i++) {
                cout << f_rec_vec[i] << ", ";
            }
            cout << f_rec_vec[f_rec_vec.size()-1] << endl;

            char* userid_list = serialize(f_rec_vec);

            sendto(sockfd, (const char *)userid_list, strlen(userid_list), 
                0, (const struct sockaddr *) &mainserv_addr, len);

            cout << "\n" << "Server A has sent the results to Main Server" << "\n" << endl;          

        }

    }
    close(sockfd);

    return 0;
}