#include <serverM.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cstdio>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define BACKLOG 10
#define LOOPBACKIP "127.0.0.1"
#define SERVERSPORT "41599"
#define SERVERDPORT "42599"
#define SERVERUPORT "43599"
#define UDPPORT     "44599"
#define TCPPORT     "45599"



ServerM::ServerM() {
    get_addrinfos();
    // Use bind_socket = true to make the main server listen to the first data sent
    // by the three servers
    create_sockets(true);
}

ServerM::~ServerM() {
    // Free the memory used by the linked lists and the socket file descriptors
    freeaddrinfo(servinfoS);
    freeaddrinfo(servinfoD);
    freeaddrinfo(servinfoU);
    close(socketfdS);
    close(socketfdD);
    close(socketfdU);
}

/*
    Sets up the structs needed for the sockets.
*/
void ServerM::get_addrinfos() {
    int status;
    memset(&hintsS, 0, sizeof hintsS); // Set the struct to empty
    hintsS.ai_family = AF_INET; // Use IPv4
    hintsS.ai_socktype = SOCK_DGRAM; // UDP socket

    memset(&hintsD, 0, sizeof hintsD); // Set the struct to empty
    hintsD.ai_family = AF_INET; // Use IPv4
    hintsD.ai_socktype = SOCK_DGRAM; // UDP socket

    memset(&hintsS, 0, sizeof hintsU); // Set the struct to empty
    hintsU.ai_family = AF_INET; // Use IPv4
    hintsU.ai_socktype = SOCK_DGRAM; // UDP socket

    // For the UDP socket used with server S
    if ((status = getaddrinfo(LOOPBACKIP, SERVERSPORT, &hintsS, &servinfoS) != 0))
    {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }

    // For the UDP socket used with server D
    if ((status = getaddrinfo(LOOPBACKIP, SERVERDPORT, &hintsD, &servinfoD) != 0))
    {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }

    // For the UDP socket used with server U
    if ((status = getaddrinfo("127.0.0.1", SERVERUPORT, &hintsU, &servinfoU) != 0))
    {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }
}

void ServerM::create_sockets() {
    // Loop through the linked list to get a valid socket to connect with server S
    for (pS = servinfoS; pS != NULL; pS = pS->ai_next) {
        if ((socketfdS = socket(pS->ai_family, pS->ai_socktype, pS->ai_protocol) == -1))
            continue; // Go for the next item in the list
        break; // No errors while creating the socket, so we got a valid one in the list
    }

    // No valid socket was found in the linked list
    if (pS == NULL) {
        fprintf(stderr, "Failed to create socket\n");
        exit(2);
    }
        // Loop through the linked list to get a valid socket to connect with server D
    for (pD = servinfoD; pS != NULL; pD = pD->ai_next) {
        if ((socketfdD = socket(pD->ai_family, pD->ai_socktype, pD->ai_protocol) == -1))
            continue; // Go for the next item in the list
        if (bind_socket) {
            if (bind(socketfdD, pD->ai_addr, pD->ai_addrlen) == -1) {
                close(socketfdD); // Close the failed socket
                continue; // Go to the next item in the list
            }
        }
        break; // No errors while creating the socket, so we got a valid one in the list
    }

    // No valid socket was found in the linked list
    if (pD == NULL) {
        fprintf(stderr, "Failed to create socket\n");
        exit(2);
    }

        // Loop through the linked list to get a valid socket to connect with server U
    for (pU = servinfoS; pU != NULL; pU = pU->ai_next) {
        if ((socketfdU = socket(pU->ai_family, pU->ai_socktype, pU->ai_protocol) == -1))
            continue; // Go for the next item in the list
        if (bind_socket) {
            if (bind(socketfdU, pU->ai_addr, pU->ai_addrlen) == -1) {
                close(socketfdU); // Close the failed socket
                continue; // Go to the next item in the list
            }
        }
        break; // No errors while creating the socket, so we got a valid one in the list
    }

    // No valid socket was found in the linked list
    if (pU == NULL) {
        fprintf(stderr, "Failed to create socket\n");
        exit(2);
    }
}