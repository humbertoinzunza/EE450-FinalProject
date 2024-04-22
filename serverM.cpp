#include "serverM.h"
#include <cstdio>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#include <bitset>

#define BACKLOG 10
#define LOOPBACKIP  "127.0.0.1"
#define SERVERSPORT "41599"
#define SERVERDPORT "42599"
#define SERVERUPORT "43599"
#define UDPPORT     "44599"
#define TCPPORT     "45599"
#define MAXBUFLEN   256


ServerM::ServerM() {
    get_addrinfos();
    create_sockets();
    cout << "The main server is up and running." << endl;
    receive_initial_data();
}

ServerM::~ServerM() {
    // Free the memory used by the linked lists and the socket file descriptors
    freeaddrinfo(servinfoM);
    freeaddrinfo(servinfoS);
    freeaddrinfo(servinfoD);
    freeaddrinfo(servinfoU);
    close(socketfdM);
    close(socketfdS);
    close(socketfdD);
    close(socketfdU);
}

/*
    Sets up the structs needed for the sockets.
*/
void ServerM::get_addrinfos() {
    int status;

    memset(&hintsM, 0, sizeof hintsM); // Set the struct to empty
    hintsM.ai_family = AF_INET; // Use IPv4
    hintsM.ai_socktype = SOCK_DGRAM; // UDP socket

    memset(&hintsS, 0, sizeof hintsS); // Set the struct to empty
    hintsS.ai_family = AF_INET; // Use IPv4
    hintsS.ai_socktype = SOCK_DGRAM; // UDP socket

    memset(&hintsD, 0, sizeof hintsD); // Set the struct to empty
    hintsD.ai_family = AF_INET; // Use IPv4
    hintsD.ai_socktype = SOCK_DGRAM; // UDP socket

    memset(&hintsU, 0, sizeof hintsU); // Set the struct to empty
    hintsU.ai_family = AF_INET; // Use IPv4
    hintsU.ai_socktype = SOCK_DGRAM; // UDP socket

    // For the UDP socket used with server 
    if ((status = getaddrinfo(LOOPBACKIP, UDPPORT, &hintsM, &servinfoM)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }

    // For the UDP socket used with server S
    if ((status = getaddrinfo(LOOPBACKIP, SERVERSPORT, &hintsS, &servinfoS)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }

    // For the UDP socket used with server D
    if ((status = getaddrinfo(LOOPBACKIP, SERVERDPORT, &hintsD, &servinfoD)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }

    // For the UDP socket used with server U
    if ((status = getaddrinfo(LOOPBACKIP, SERVERUPORT, &hintsU, &servinfoU)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }
}

void ServerM::create_sockets() {
    // Loop through the linked list to get a valid socket to receive incoming UDP connections
    for (pM = servinfoM; pM != NULL; pM = pM->ai_next) {
        if ((socketfdM = socket(pM->ai_family, pM->ai_socktype, pM->ai_protocol)) == -1) {
            perror("listener: socket");
            continue; // Go for the next item in the list
        }
        // Try to bind the socket to "listen" for incoming connections
        if (bind(socketfdM, pM->ai_addr, pM->ai_addrlen) == -1) {
            close(socketfdM);
            perror("listener: bind");
            continue;
        }
        break; // No errors while creating the socket, so we got a valid one in the list
    }

    // No valid socket was found in the linked list
    if (pM == NULL) {
        fprintf(stderr, "Failed to create socket (M)\n");
        exit(2);
    }

    // Loop through the linked list to get a valid socket to connect with server S
    for (pS = servinfoS; pS != NULL; pS = pS->ai_next) {
        if ((socketfdS = socket(pS->ai_family, pS->ai_socktype, pS->ai_protocol)) == -1)
            continue; // Go for the next item in the list
        break; // No errors while creating the socket, so we got a valid one in the list
    }

    // No valid socket was found in the linked list
    if (pS == NULL) {
        fprintf(stderr, "Failed to create socket (S)\n");
        exit(2);
    }
        // Loop through the linked list to get a valid socket to connect with server D
    for (pD = servinfoD; pD != NULL; pD = pD->ai_next) {
        if ((socketfdD = socket(pD->ai_family, pD->ai_socktype, pD->ai_protocol)) == -1)
            continue; // Go for the next item in the list
        break; // No errors while creating the socket, so we got a valid one in the list
    }

    // No valid socket was found in the linked list
    if (pD == NULL) {
        fprintf(stderr, "Failed to create socket (D)\n");
        exit(2);
    }

        // Loop through the linked list to get a valid socket to connect with server U
    for (pU = servinfoS; pU != NULL; pU = pU->ai_next) {
        if ((socketfdU = socket(pU->ai_family, pU->ai_socktype, pU->ai_protocol)) == -1)
            continue; // Go for the next item in the list
        break; // No errors while creating the socket, so we got a valid one in the list
    }

    // No valid socket was found in the linked list
    if (pU == NULL) {
        fprintf(stderr, "Failed to create socket (U)\n");
        exit(2);
    }
}

/*
    Used to receive the map data from all backend servers, process it, and store it in their corresponding data structures.
*/
void ServerM::receive_initial_data() {
    int numbytes;
    uint16_t port;
    unsigned char* buf = receive_udp(&numbytes, &port);
    char server_id = get_sender_id(port);
    fill_room_data(server_id, buf, numbytes);
    delete [] buf;
    printf("The main server has received the room status from Server %c using UDP over port %i.\n", server_id, port);
    buf = receive_udp(&numbytes, &port);
    server_id = get_sender_id(port);
    fill_room_data(server_id, buf, numbytes);
    delete [] buf;
    printf("The main server has received the room status from Server %c using UDP over port %i.\n", server_id, port);
    buf = receive_udp(&numbytes, &port);
    server_id = get_sender_id(port);
    fill_room_data(server_id, buf, numbytes);
    delete [] buf;
    printf("The main server has received the room status from Server %c using UDP over port %i.\n", server_id, port);
}

/*
    Returns the letter ID of the server given the port number.
*/
char ServerM::get_sender_id(uint16_t port) {
    if (port == atoi(SERVERSPORT)) return 'S';
    if (port == atoi(SERVERDPORT)) return 'D';
    if (port == atoi(SERVERUPORT)) return 'U';
    return 0;
}

/*
    Gets the map data from a server and stores it in the local corresponding map.
*/
void ServerM::fill_room_data(char server, unsigned char *buffer, int length) {
    map<unsigned short,unsigned short> *data;
    switch (server) {
        case 'S': data = &rooms_S; break;
        case 'D': data = &rooms_D; break;
        case 'U': data = &rooms_U; break;
        default: data = &rooms_S; break;
    }
    unsigned short key, value;
    for (int i = 0; i < length; i += 4) {
        // Construct a short from two chars
        key = (((unsigned short)buffer[i + 1]) << 8) | buffer[i];
        value = (((unsigned short)buffer[i + 3]) << 8) | buffer[i + 2];
        // Store into appropiate map
        (*data)[key] = value;
    }
}

/*
    Receives data from the selected server (S, D, or U).
*/
unsigned char * ServerM::receive_udp(int *numbytes, uint16_t *port) {
    unsigned char* buf = new unsigned char[MAXBUFLEN];
    struct sockaddr from_addr;
    memset(&from_addr, 0, sizeof from_addr);
    socklen_t from_len = sizeof from_addr;
    // Receive data from the given server
    if ((*numbytes = recvfrom(socketfdM, buf, MAXBUFLEN - 1, 0, &from_addr, &from_len)) == -1) {
        perror("receive_from");
        exit(3);
    }
    struct sockaddr_in *from_addr_in = (struct sockaddr_in*)&from_addr;
    (*port) = htons(from_addr_in->sin_port);
    return buf;
}

void ServerM::print_room_data(char server) {
    map<unsigned short, unsigned short> *data;
    switch (server) {
        case 'S': data = &rooms_S; break;
        case 'D': data = &rooms_D; break;
        case 'U': data = &rooms_U; break;
        default: data = &rooms_S; break;
    }
    cout << "Room data for backend server " << server << ":" << endl;
    for (map<unsigned short, unsigned short>::iterator it = (*data).begin(); it != (*data).end(); it++) 
        cout << server << it->first << " : " << it->second << endl;
}

int main(int argc, char * argv[]) {
    ServerM main_server;
    main_server.print_room_data('S');
    main_server.print_room_data('D');
    main_server.print_room_data('U');
}