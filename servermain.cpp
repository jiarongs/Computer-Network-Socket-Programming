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

#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>


using namespace std;
const char* SERVERA_PORTNO = "30296";
const char* SERVERB_PORTNO = "31296";
const char* MAIN_SERV_PORTNO = "32296";
const char* MAINSERV_PORTNO_TCP = "33296";

string NOT_FOUND = "user not found";


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


int main() {
    int sockfd, newsockfd_tcp, sockfd_tcp, portno_servA, portno_servB, portno_mainserv, portno_main_tcp;
    char bufferA[2048], bufferB[2048];
    struct sockaddr_in main_serv_addr, servA_addr, servB_addr, mainserv_addr_tcp, client_addr;
    socklen_t servA_len, servB_len, clientLen;
    int n1, n2, n0;
    map<string, int> country_backend_mapping;
    
    cout << "Main server is up and running." << endl;

    const char* hello = "send me country list";

    // UDP socket

    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed - UDP");
        exit(EXIT_FAILURE);
    }
  
    memset(&main_serv_addr, 0, sizeof(main_serv_addr));
    portno_servA = atoi(SERVERA_PORTNO); 
    portno_servB = atoi(SERVERB_PORTNO); 
    portno_mainserv = atoi(MAIN_SERV_PORTNO); 
    
    servA_addr.sin_family = AF_INET;
    servA_addr.sin_port = htons(portno_servA);
    servA_addr.sin_addr.s_addr = INADDR_ANY;

    servB_addr.sin_family = AF_INET;
    servB_addr.sin_port = htons(portno_servB);
    servB_addr.sin_addr.s_addr = INADDR_ANY;

    main_serv_addr.sin_family = AF_INET;
    main_serv_addr.sin_port = htons(portno_mainserv);
    main_serv_addr.sin_addr.s_addr = INADDR_ANY;  

    if ( bind(sockfd, (struct sockaddr *)&main_serv_addr, 
            sizeof(main_serv_addr)) < 0 )
    {
        error("ERROR on binding");
    }   


    // to A

    servA_len = sizeof(servA_addr);

    sendto(sockfd, hello, strlen(hello), 
                0, (const struct sockaddr *) &servA_addr, servA_len);


    n1 = recvfrom(sockfd, bufferA, 2048, 
                0, (struct sockaddr *) &servA_addr,
                &servA_len);

    if(n1 < 0) {
        error("ERROR on recvfrom - A");
    }

    bufferA[n1] = '\0';

    cout << "Main server has received the country list from server A using UDP over port "
        << SERVERA_PORTNO << endl;


    // to B

    servB_len = sizeof(servB_addr);

    sendto(sockfd, hello, strlen(hello), 
                0, (const struct sockaddr *) &servB_addr, servB_len);


    n2 = recvfrom(sockfd, bufferB, 2048, 
                0, (struct sockaddr *) &servB_addr,
                &servB_len);

    if(n2 < 0) {
        error("ERROR on recvfrom - B");
    }

    bufferB[n2] = '\0';

    cout << "Main server has received the country list from server B using UDP over port "
        << SERVERB_PORTNO << endl;

    // deserialize

    vector<string> servA_countries = deserialize(bufferA);
    vector<string> servB_countries = deserialize(bufferB);

    cout << "\n" << "Server A" << endl;

    for(int i = 0; i < servA_countries.size(); i++) {
        cout << servA_countries[i] << endl;
        country_backend_mapping[servA_countries[i]] = 0;
    }

    cout << "\n" << "Server B" << endl;

    for(int i = 0; i < servB_countries.size(); i++) {
        cout << servB_countries[i] << endl;
        country_backend_mapping[servB_countries[i]] = 1;
    }


    // TCP socket
    sockfd_tcp = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd_tcp < 0) {
        error("Error opening socket - TCP");
    }

    bzero((char*) &mainserv_addr_tcp, sizeof(mainserv_addr_tcp));

    portno_main_tcp = atoi(MAINSERV_PORTNO_TCP);  

    mainserv_addr_tcp.sin_family = AF_INET;
    mainserv_addr_tcp.sin_addr.s_addr = INADDR_ANY;
    mainserv_addr_tcp.sin_port = htons(portno_main_tcp);

    if( bind(sockfd_tcp, (struct sockaddr *) &mainserv_addr_tcp, sizeof(mainserv_addr_tcp)) < 0) {
        error("ERROR on binding - TCP");
    }

    listen(sockfd_tcp, 2);
    clientLen = sizeof(client_addr);

    int client_id = 0; 

    char* country = (char*)malloc(25*sizeof(char));
    char* userID = (char*)malloc(25*sizeof(char));


    while (1) {
        newsockfd_tcp = accept(sockfd_tcp, (struct sockaddr *) &client_addr, &clientLen);
        if(newsockfd_tcp < 0) {
            error("ERROR on accept");
        }
        
        client_id += 1;

        // add fork
        int child_proc_no;

        if(child_proc_no = fork() == 0) {
        //process is successfully forked
        while(1) {
                int size1 = sizeof(country);
                memset(country,'\0',size1);

                int size2 = sizeof(userID);
                memset(userID,'\0',size2);

                n1 = read(newsockfd_tcp, country, 2048);
                country[n1] = '\0';
                n2 = read(newsockfd_tcp, userID, 2048);
                userID[n2] = '\0';

                if(n1 < 0 || n2 < 0) {
                    error("ERROR reading from socket - server");
                }

                cout << "Main server has received the request on user " << userID << " in " << country 
                    << " from client " << client_id << " using TCP over port " << MAINSERV_PORTNO_TCP << endl;

                /*
                look up for the input country
                */
                map<string, int>::iterator res;
                res = country_backend_mapping.find(country); 

                if(res == country_backend_mapping.end()) {
                    // input country not found
                    cout << country << " does not show up in server A&B" << endl;


                    cout << "The Main Server has sent " << country << ": Not found "  << "to client "
                        << client_id << " using TCP over port " << MAINSERV_PORTNO_TCP << endl;

                    const char* country_not_found = "country not found";

                    n0 = write(newsockfd_tcp, country_not_found, string(country_not_found).length());
                    if(n0 < 0) {
                        error("ERROR writing to socket - server");
                    }

                } 
                else {
                    // input country found
                    if(country_backend_mapping[country] == 0) {
                        // send to servA
                        cout << country << " shows up in server A" << endl;
                        servA_len = sizeof(servA_addr);

                        sendto(sockfd, userID, strlen(userID), 
                            0, (const struct sockaddr *) &servA_addr, servA_len);

                        cout << "The Main Server has sent request of User " << userID 
                            << " to server A using UDP over port " << SERVERA_PORTNO << endl;

                        sendto(sockfd, country, strlen(country), 
                            0, (const struct sockaddr *) &servA_addr, servA_len);


                        // start userID searching in backend server

                        n1 = recvfrom(sockfd, bufferA, 2048, 0, (struct sockaddr *) &servA_addr,
                            &servA_len);
                        bufferA[n1] = '\0';

                        if(string(bufferA).compare(NOT_FOUND) == 0) {
                            // user id not found
                            cout << "Main server has received User " << userID 
                                << ": Not found from server A" << endl;

                            const char* userid_not_found = "user id not found";
                            n0 = write(newsockfd_tcp, userid_not_found, string(userid_not_found).length());
                            if(n0 < 0) {
                                error("ERROR writing to socket - server");
                            }

                            cout << "Main Server has sent message to client " 
                                << client_id << " using TCP over " << MAINSERV_PORTNO_TCP << endl;

                        } else {
                            // user id found!!
                            cout << "Main server has received searching result of User " << userID 
                                << " from server A" << endl;

                            n0 = write(newsockfd_tcp, bufferA, string(bufferA).length());

                            cout << "Main Server has sent searching result(s) to client " 
                                << client_id << " using TCP over port " << MAINSERV_PORTNO_TCP << endl;

                        }

                    } else {
                        // send to servB
                        cout << country << " shows up in server B" << endl;
                        servB_len = sizeof(servB_addr);

                        sendto(sockfd, userID, strlen(userID), 
                            0, (const struct sockaddr *) &servB_addr, servB_len);
                        cout << "The Main Server has sent request of User " << userID 
                            << " to server B using UDP over port " << SERVERB_PORTNO << endl;
                        
                        sendto(sockfd, country, strlen(country), 
                            0, (const struct sockaddr *) &servB_addr, servB_len);

                        // start userID searching in backend server

                        n2 = recvfrom(sockfd, bufferB, 2048, 0, (struct sockaddr *) &servB_addr,
                            &servB_len);
                        bufferB[n2] = '\0';

                        if(string(bufferB).compare(NOT_FOUND) == 0) {
                            // user id not found
                            cout << "Main server has received User " << userID 
                                << ": Not found from server B" << endl;

                            const char* userid_not_found = "user id not found";
                            n0 = write(newsockfd_tcp, userid_not_found, string(userid_not_found).length());
                            if(n0 < 0) {
                                error("ERROR writing to socket - server");
                            }

                            cout << "Main Server has sent message to client " 
                                << client_id << " using TCP over " << MAINSERV_PORTNO_TCP << endl;

                        }
                        else {
                            // user id found!!
                            cout << "Main server has received searching result of User " << userID 
                                << " from server B" << endl;

                            n0 = write(newsockfd_tcp, bufferB, string(bufferB).length());

                            cout << "Main Server has sent searching result(s) to client " 
                                << client_id << " using TCP over port " << MAINSERV_PORTNO_TCP << endl;                               

                        }                        

                    }
                }
            }
            close(newsockfd_tcp);
        } else {
            close(newsockfd_tcp);
        }
    }

    close(sockfd);
    close(sockfd_tcp);
    return 0;

}