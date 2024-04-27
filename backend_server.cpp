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


#define MAXBUFLEN   8

/*
    Default constructor.
*/
BackendServer::BackendServer(const char *server_id, const char *my_udp_port, const char *my_data_fname) {
    // Initialize the server's data
    SERVER_ID = server_id;
    MY_UDP_PORT = my_udp_port;
    MY_DATA_FNAME = my_data_fname;
    // Load data from file
    load_data();
    // Create the addrinfo structs
    get_addrinfos();
    // Create the required sockets
    create_sockets();
    printf("The Server %s is up and running using UDP on port %s.\n", SERVER_ID, MY_UDP_PORT);
    // Send the initial data to the main server
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
    Source: Beej’s Guide to Network Programming.
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

/*
    Source: Beej’s Guide to Network Programming.
*/
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
    // Beej’s Guide to Network Programming
    if ((numbytes = sendto(socketfd, data, bytes_to_send, 0, pM->ai_addr, pM->ai_addrlen)) == -1) {
        perror("send");
        exit(3);
    }
}

void BackendServer::receive_loop() {
    int numbytes;
    unsigned char buf[MAXBUFLEN];
    unsigned char response_code;
    // Run forever
    while (1) {
        // Receive a request
        receive_udp(buf, &numbytes);
        if (buf[0] == 0) { // Query request
            query(buf, &response_code); // Check for availability and get the response code
            send_response(response_code); // Send the response code to the main server
            printf("The Server %s finished sending the response to the main server.\n", SERVER_ID);
        }
        else if (buf[0] == 1) { // Reservation request
            reservation(buf, &response_code); // Get the response code and reserve the room if necessary
            send_response(response_code); // Send the response code to the main server
            if (response_code == 0) // If reservation was successful
                printf("The Server %s finished sending the response and the updated room status to the main server.\n", SERVER_ID);
            else // If reservation failed
                printf("The Server %s finished sending the response to the main server.\n", SERVER_ID);
        }
    }
}

/*
    Searches for the requested room number and sets the response code.
*/
void BackendServer::query(unsigned char *buffer, unsigned char *response_code) {
    unsigned short room_number;
    printf("The Server %s received an availability request from the main server.\n", SERVER_ID);
    // Put the room number into an unsigned short
    room_number = (((unsigned short)buffer[1]) << 8) | buffer[2];
    // Look for the room number in the map
    map<unsigned short, unsigned short>::iterator it = room_status.find(room_number);
    if (it == room_status.end()) // Room not found
    {
        cout << "Not able to find the room layout." << endl;
        (*response_code) = 2;
        return;
    }
    else if (it->second == 0) // Room exists, but it's not available (= 0)
    {
        printf("Room %s%i is not available.\n", SERVER_ID, room_number);
        (*response_code) = 1;
        return;
    }
    printf("Room %s%i is available.\n", SERVER_ID, room_number);
    (*response_code) = 0; // Room exists, and status is > 0
}

/*
    Searches for the requested room number and sets the response code.
*/
void BackendServer::reservation(unsigned char *buffer, unsigned char *response_code) {
    unsigned short room_number;
    printf("The Server %s received a reservation request from the main server.\n", SERVER_ID);
    // Put the room number into an unsigned short
    room_number = (((unsigned short)buffer[1]) << 8) | buffer[2];
    // Look for the room number in the map
    map<unsigned short, unsigned short>::iterator it = room_status.find(room_number);
    if (it == room_status.end()) // Room not found
    {
        cout << "Cannot make a reservation. Not able to find the room layout." << endl;
        (*response_code) = 2;
        return;
    }
    else if (it->second == 0) // Room exists, but it's not available (= 0)
    {
        printf("Cannot make a reservation. Room %s%i is not available.\n", SERVER_ID, room_number);
        (*response_code) = 1;
        return;
    }
    // Update the room status
    it->second--;
    printf("Successful reservation. The count of Room %s%i is now %i.\n", SERVER_ID, room_number, it->second);
    (*response_code) = 0; // Reservation was successful
}

void BackendServer::send_response(unsigned char response_code) {
    int sent_bytes;
    // Beej’s Guide to Network Programming
    if ((sent_bytes = sendto(socketfd, &response_code, 1, 0, pM->ai_addr, pM->ai_addrlen)) == -1) {
        perror("send");
        exit(4);
    }
}

/*
    Receives data from the main server
    Source: // Beej’s Guide to Network Programming
*/
void BackendServer::receive_udp(unsigned char *buf, int *numbytes) {
    struct sockaddr from_addr;
    memset(&from_addr, 0, sizeof from_addr);
    socklen_t from_len = sizeof from_addr;
    // Receive data from the given server (Beej’s Guide to Network Programming)
    if (((*numbytes) = recvfrom(socketfd, buf, MAXBUFLEN - 1, 0, &from_addr, &from_len)) == -1) {
        perror("receive_from");
        exit(3);
    }
}