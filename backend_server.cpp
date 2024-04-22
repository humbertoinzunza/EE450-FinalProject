#include "backend_server.h"
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

/*
    Default constructor.
*/
BackendServer::BackendServer(const char *server_id, const char *my_udp_port, const char *my_data_fname) {
    SERVER_ID = server_id;
    MY_UDP_PORT = my_udp_port;
    MY_DATA_FNAME = my_data_fname;
    load_data();
    get_addrinfos();
    create_sockets();
    printf("The Server %s is up and running using UDP on port %s.\n", SERVER_ID, MY_UDP_PORT);
    send_initial_data();
    printf("The Server %s has sent the room status to the main server.\n", SERVER_ID);
}

/*
    Destructor
*/
BackendServer::~BackendServer() {
    // Free the memory used by the linked lists and the socket file descriptors
    freeaddrinfo(servinfo);
    freeaddrinfo(servinfoM);    
    close(socketfd);
    close(socketfdM);
}

/*
    Loads the data from the input file into the room_status map.
*/
void BackendServer::load_data() {
    ifstream data_file;
    data_file.open(MY_DATA_FNAME);
    for (string line; getline(data_file, line);) {
        string buffer = "";
        short number;
        short status;
        bool room_number_found = false;
        // Start at 1 to skip the first letter
        for (uint i = 1; i < line.length(); i++) {
            if (!room_number_found)
            {
                if (!isdigit(line[i])) {
                    number = stoi(buffer);
                    buffer = "";
                    room_number_found = true;
                }
                else
                    buffer += line[i];
            }
            else {
                if (isdigit(line[i]))
                    buffer += line[i];
            }
        }
        status = stoi(buffer);
        // Insert data point into map
        room_status[number] = status;
    }
}

/*
    Sets up the structs needed for the sockets.
*/
void BackendServer::get_addrinfos() {
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

void BackendServer::create_sockets() {
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

void BackendServer::send_initial_data() {
    // Put data to be sent into array so it's easier
    int i = 0;
    short data[room_status.size() * 2];
    for (auto const& kvp : room_status) {
        data[i] = kvp.first;
        data[i + 1] = kvp.second;
        i += 2;
    }
    int bytes_to_send = sizeof(short) * room_status.size() * 2;
    int numbytes;
    if ((numbytes = sendto(socketfd, data, bytes_to_send, 0, pM->ai_addr, pM->ai_addrlen)) == -1)
        exit(3);
}