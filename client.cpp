#include "client.h"
#include <fstream>
#include <cstdio>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define LOOPBACKIP "127.0.0.1"
#define SERVERPORT "45599"
#define MAXBUFLEN   8

Client::Client() {
    get_addrinfos();
    create_sockets();
    cout << "Client is up and running." << endl;
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
    struct addrinfo hints;

    memset(&hints, 0, sizeof hints); // Set the struct to empty
    hints.ai_family = AF_INET; // Use IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP socket

    // For the TCP socket used in the client
    if ((status = getaddrinfo(LOOPBACKIP, SERVERPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }
}

void Client::create_sockets() {
    // Loop through the linked list to get a valid socket to receive incoming UDP connections
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((socketfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("client: socket");
            continue; // Go for the next item in the list
        }
        // Try to connect to the socket
        if (connect(socketfd, p->ai_addr, p->ai_addrlen) == -1) {
            perror("client: connect");
            close(socketfd);
            continue;
        }
        break; // No errors while creating the socket, so we got a valid one in the list
    }

    // No valid socket was found in the linked list
    if (p == NULL) {
        fprintf(stderr, "Failed to connect\n");
        exit(2);
    }
}

void Client::receive(unsigned char *buffer, int *numbytes) {
    if (((*numbytes) = recv(socketfd, buffer, MAXBUFLEN - 1, 0)) == -1) {
        perror("recv");
        exit(3);
    }
}

bool Client::authenticate() {
    string username, password;
    get_user_credentials(&username, &password);
    // Build out buffer
    int buf_len = username.length() + password.length() + 2; // Data size + one byte of operation code + one byte of separation
    unsigned char buffer[buf_len];
    int i = 1;
    int stop = username.length() + 1;
    buffer[0] = 0; // Authentication request
    for (; i < stop; i++) {
        buffer[i] = username[i - 1]; // Username
    }
    buffer[i++] = 0; // Separation
    stop = buf_len;
    int j = 0;
    for (; i < stop; i++, j++) {
        buffer[i] = password[j]; // Encrypted password
    }
    
    if (send(socketfd, buffer, buf_len, 0) == -1) {
        perror("send");
        exit(4);
    }
    printf("%s sent an authentication request to the main server.\n", username.c_str());

    // Get in buffer
    unsigned char in_buffer[MAXBUFLEN];
    int numbytes;
    if ((numbytes = recv(socketfd, buffer, MAXBUFLEN - 1, 0)) == -1) {
        perror("receive_from");
        exit(4);
    }

    print_authentication_error(username, in_buffer[0]);

    return in_buffer[0] == 0;
}

void Client::print_authentication_error(string username, unsigned char code) {
    if (code == 0) printf("Welcome member %s!\n", username.c_str());
    else if (code == 1) cout << "Failed login: Username does not exist." << endl;
    else if (code == 2) cout << "Failed login: Password does not match." << endl;
}

void Client::get_user_credentials(string *username, string *password) {
    cout << "Please enter the username: ";
    getline(cin, (*username));
    while ((*username).length() < 5 || (*username).length() > 50) {
        cout << "Error. Username must be between 5 and 50 characters." << endl;
        cout << "Please enter the username: ";
        getline(cin, (*username));
    }
    cout << "Please enter the password: ";
    getline(cin, (*password));
    while ((*password).length() < 5 || (*password).length() > 50) {
        cout << "Error. Password must be between 5 and 50 characters." << endl;
        cout << "Please enter the password: ";
        getline(cin, (*password));
    }
    (*password) = basic_encrypt((*password));
}

int main(int argc, char *argv[]) {
    Client client;
    bool authenticated = client.authenticate();
    while (!authenticated){
        authenticated = client.authenticate();
    }
}