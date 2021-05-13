#include <iostream>
#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <vector>
#include <fstream>
#include<sstream>

#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>

using namespace std;
const char* MAINSERV_PORT_TCP = "33296";
string COUNTRY_NOT_FOUND = "country not found";
string USERID_NOT_FOUND = "user id not found";


void error(string err_msg) {
    cerr << err_msg << endl;
    exit(1);
}

vector<string> deserialize(char* buffer) {
    vector<string> v;
    string str = string(buffer);
    stringstream lineStream(str);
    string s;
    while (lineStream >> s){
        v.push_back(s);
    }
    return v;
}


int main(int argc, char *argv[]) {
    
    int cli_sockfd, portno, n, n1, n2, time;
    time = 1;
    char buffer[2048];
    struct sockaddr_in serv_addr, client_addr;
    struct hostent *server;
    socklen_t clientlen;

    /*
    booting up
    */

    cout << "Client is up and running.\n" << endl;

    clientlen = sizeof(client_addr);

    cli_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(cli_sockfd < 0) {
        error("Error opening socket");
    }

    server = gethostbyname("localhost");

    if(server == NULL) {
        cout << stderr << "ERROR, no such host" <<endl;
    }

    //clear server adress
    bzero((char*) &serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;    

    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    portno = atoi(MAINSERV_PORT_TCP);
    serv_addr.sin_port = htons(portno);

    if(connect(cli_sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        error("ERROR connecting");
    }

    if(getsockname(cli_sockfd,(struct sockaddr *)&client_addr,&clientlen) < 0) {
        error("ERROR getting socket name");
    }

    while(1) {
        if(time > 1) 
            cout << "-----Start a new query-----" << endl;
        

        string country_input = " ";
        string userID = " ";
        
        cout << "Enter Country Name:" << endl;
        cin >> country_input;
        
        const char *country = country_input.c_str();
        n1 = write(cli_sockfd, country, strlen(country));

        cout << "Enter user ID:" << endl;
        cin >> userID;

        const char* user_id = userID.c_str();
        n2 = write(cli_sockfd, user_id, strlen(user_id));


        if(n1 < 0 || n2 < 0) {
            error("ERROR writing to socket - client");
        }

        cout << "Client has sent " << country << " and User " << userID 
            << " to Main Server using TCP over port " << client_addr.sin_port << endl;


        bzero(buffer, 2048);
        n = read(cli_sockfd, buffer, 2047);

        if(n < 0) {
            error("ERROR reading from socket - client");
        }

        buffer[n] = '\0';

        if (string(buffer).compare(COUNTRY_NOT_FOUND) == 0) {
            cout << country << ": Not found." << endl;
        } else if (string(buffer).compare(USERID_NOT_FOUND) == 0) {
            cout << userID << ": Not found." << endl;
        } else {
            vector<string> result = deserialize(buffer);

            cout << "User ";

            for(int i = 0; i < result.size() - 1; i++) {
                cout << result[i] << ", ";
            }
            cout << result[result.size()-1] << " is/are possible friend(s) of User " 
                << userID << " in " << country << endl;
        
        }

        time += 1;
    }

    return 0;

}