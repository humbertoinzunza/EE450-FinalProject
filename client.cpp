#include "client.h"
#include <fstream>
#include <cstdio>
#include <iostream>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define LOOPBACKIP "127.0.0.1"
#define SERVERPORT "45599"

Client::Client() {

}

Client::~Client() {

}

string Client::basic_encrypt(string msg) {
    string cypher_text = msg;
    for (unsigned int i = 0; i < msg.length(); i++) {
        if (msg[i] >= 'a' && msg[i] <= 'z') 
            cypher_text[i] = (msg[i] + 3 - 'a') % 26 + 'a';
        else if (msg[i] >= 'A' && msg[i] <= 'Z') 
            cypher_text[i] = (msg[i] + 3 - 'A') % 26 + 'A';
        else if (msg[i] >= '0' && msg[i] <= '9') 
            cypher_text[i] = (msg[i] + 3 - '0') % 10 + '0';
        else
            cypher_text[i] = msg[i];
    }
    return cypher_text;
}

/*
    Sets up the structs needed for the sockets.
*/
void Client::get_addrinfos() {
    int status;

    memset(&hints, 0, sizeof hints); // Set the struct to empty
    hints.ai_family = AF_INET; // Use IPv4
    hints.ai_socktype = SOCK_DGRAM; // UDP socket

    memset(&hintsM, 0, sizeof hintsM); // Set the struct to empty
    hintsM.ai_family = AF_INET; // Use IPv4
    hintsM.ai_socktype = SOCK_DGRAM; // UDP socket

    // For the UDP socket bound to this server
    if ((status = getaddrinfo(LOOPBACKIP, MY_UDP_PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }

    // For the UDP socket used with server M
    if ((status = getaddrinfo(LOOPBACKIP, SERVERMPORT, &hintsM, &servinfoM)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }
}

void Client::create_sockets() {
    // Loop through the linked list to get a valid socket to receive incoming UDP connections
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((socketfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
            continue; // Go for the next item in the list
        // Try to bind the socket to "listen" for incoming connections
        if (bind(socketfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(socketfd);
            continue;
        }
        break; // No errors while creating the socket, so we got a valid one in the list
    }

    // No valid socket was found in the linked list
    if (p == NULL) {
        fprintf(stderr, "Failed to create socket\n");
        exit(2);
    }
    
    // Loop through the linked list to get a valid socket to connect with server M
    for (pM = servinfoM; pM != NULL; pM = pM->ai_next) {
        if ((socketfdM = socket(pM->ai_family, pM->ai_socktype, pM->ai_protocol)) == -1)
            continue; // Go for the next item in the list
        break; // No errors while creating the socket, so we got a valid one in the list
    }

    // No valid socket was found in the linked list
    if (pM == NULL) {
        fprintf(stderr, "Failed to create socket\n");
        exit(2);
    }
}

int main(int argc, char *argv[]) {
    Client client;
    string s;
    getline(cin, s);
    cout << client.basic_encrypt(s) << endl;
}