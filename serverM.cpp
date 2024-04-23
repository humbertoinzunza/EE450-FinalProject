#include "serverM.h"
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#include <bitset>

#define BACKLOG 10
#define LOOPBACKIP      "127.0.0.1"
#define SERVERSPORT     "41599"
#define SERVERDPORT     "42599"
#define SERVERUPORT     "43599"
#define UDPPORT         "44599"
#define TCPPORT         "45599"
#define UDP_MAXBUFLEN   1024
#define TCP_MAXBUFLEN   128
#define USERDB          "member.txt"


ServerM::ServerM() {
    get_addrinfos();
    create_sockets();
    cout << "The main server is up and running." << endl;
    receive_initial_data();
    listen_for_connections();
}

ServerM::~ServerM() {
    // Free the memory used by the linked lists and the socket file descriptors
    freeaddrinfo(servinfo_udp);
    freeaddrinfo(servinfoS);
    freeaddrinfo(servinfoD);
    freeaddrinfo(servinfoU);
    close(socketfd_udp);
    close(socketfdS);
    close(socketfdD);
    close(socketfdU);
}

/*
    Sets up the structs needed for the sockets.
*/
void ServerM::get_addrinfos() {
    int status;
    struct addrinfo hints;

    memset(&hints, 0, sizeof hints); // Set the struct to empty
    hints.ai_family = AF_INET; // Use IPv4
    hints.ai_socktype = SOCK_DGRAM; // UDP socket

    // For the UDP socket used with server 
    if ((status = getaddrinfo(LOOPBACKIP, UDPPORT, &hints, &servinfo_udp)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }

    memset(&hints, 0, sizeof hints); // Set the struct to empty
    hints.ai_family = AF_INET; // Use IPv4
    hints.ai_socktype = SOCK_DGRAM; // UDP socket

    // For the UDP socket used with server S
    if ((status = getaddrinfo(LOOPBACKIP, SERVERSPORT, &hints, &servinfoS)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }

    memset(&hints, 0, sizeof hints); // Set the struct to empty
    hints.ai_family = AF_INET; // Use IPv4
    hints.ai_socktype = SOCK_DGRAM; // UDP socket

    // For the UDP socket used with server D
    if ((status = getaddrinfo(LOOPBACKIP, SERVERDPORT, &hints, &servinfoD)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }

    memset(&hints, 0, sizeof hints); // Set the struct to empty
    hints.ai_family = AF_INET; // Use IPv4
    hints.ai_socktype = SOCK_DGRAM; // UDP socket

    // For the UDP socket used with server U
    if ((status = getaddrinfo(LOOPBACKIP, SERVERUPORT, &hints, &servinfoU)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }

    memset(&hints, 0, sizeof hints); // Set the struct to empty
    hints.ai_family = AF_INET; // Use IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP socket

    // For the TCP socket used with clients
    if ((status = getaddrinfo(LOOPBACKIP, TCPPORT, &hints, &servinfo_tcp)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }
}

void ServerM::create_sockets() {
    // Loop through the linked list to get a valid socket to receive incoming UDP connections
    for (p_udp = servinfo_udp; p_udp != NULL; p_udp = p_udp->ai_next) {
        if ((socketfd_udp = socket(p_udp->ai_family, p_udp->ai_socktype, p_udp->ai_protocol)) == -1) {
            perror("listener: socket");
            continue; // Go for the next item in the list
        }
        // Try to bind the socket to "listen" for incoming connections
        if (bind(socketfd_udp, p_udp->ai_addr, p_udp->ai_addrlen) == -1) {
            close(socketfd_udp);
            perror("listener: bind");
            continue;
        }
        break; // No errors while creating the socket, so we got a valid one in the list
    }

    // No valid socket was found in the linked list
    if (p_udp == NULL) {
        fprintf(stderr, "Failed to create UDP socket (M)\n");
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

    // Loop through the linked list to get a valid socket to receive incoming TCP connections
    for (p_tcp = servinfo_tcp; p_tcp != NULL; p_tcp = p_tcp->ai_next) {
        if ((socketfd_tcp = socket(p_tcp->ai_family, p_tcp->ai_socktype, p_tcp->ai_protocol)) == -1) {
            perror("listener: socket");
            continue; // Go for the next item in the list
        }
        int yes = 1;
        if ((setsockopt(socketfd_tcp, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))) == -1) {
            perror("setsockopt");
            exit(2);
        }
        // Try to bind the socket to "listen" for incoming connections
        if (bind(socketfd_tcp, p_tcp->ai_addr, p_tcp->ai_addrlen) == -1) {
            close(socketfd_tcp);
            perror("listener: bind");
            continue;
        }
        break; // No errors while creating the socket, so we got a valid one in the list
    }

    // No valid socket was found in the linked list
    if (p_tcp == NULL) {
        fprintf(stderr, "Failed to create TCP socket (M)\n");
        exit(2);
    }
}

void sigchld_handler(int s) {
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

/*
    Uses the function listen() to look for incoming TCP connections.
*/
void ServerM::listen_for_connections() {
    if (listen(socketfd_tcp, BACKLOG) == -1) {
        perror("listen");
        exit(4);
    }
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(4);
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
    Closes the parent socket. To be used by children sockets after fork() call.
*/
void ServerM::close_parent_socket() {
    close(socketfd_tcp);
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
unsigned char *ServerM::receive_udp(int *numbytes, uint16_t *port) {
    unsigned char* buf = new unsigned char[UDP_MAXBUFLEN];
    struct sockaddr from_addr;
    memset(&from_addr, 0, sizeof from_addr);
    socklen_t from_len = sizeof from_addr;
    // Receive data from the given server
    if (((*numbytes) = recvfrom(socketfd_udp, buf, UDP_MAXBUFLEN - 1, 0, &from_addr, &from_len)) == -1) {
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

/*
    Accepts an incoming connection, 
*/
int ServerM::accept_connection(uint16_t *port) {
    struct sockaddr *from_addr;
    socklen_t addr_size = sizeof from_addr;
    int new_fd = accept(socketfd_tcp, from_addr, &addr_size);
    if (new_fd == -1) {
        perror("accept");
        exit(5);
    }
    struct sockaddr_in *from_addr_in = (sockaddr_in*)from_addr;
    (*port) = htons(from_addr_in->sin_port);
    return new_fd;
}

unsigned char *ServerM::receive_tcp(int socketfd, int *numbytes) {
    unsigned char *buffer = new unsigned char[TCP_MAXBUFLEN];
    if (((*numbytes) = recv(socketfd, buffer, TCP_MAXBUFLEN - 1, 0)) == -1) {
        perror("recv");
        exit(6);
    }
}

void ServerM::close_child_socket(int sd) {
    close(sd);
}

void ServerM::lookup_credentials(string uname, string enc_pass, unsigned char *response_code) {
    ifstream data_file;
    data_file.open(USERDB);
    int delim_loc;
    bool uname_match;
    for (string line; getline(data_file, line);) {
        // Location of delimiter
        delim_loc = line.find(',');
        if (uname.length() != delim_loc) continue;
        uname_match = true;
        for (int i = 0; i < delim_loc; i++) {
            if (uname[i] != line[i]) {
                uname_match = false;
                break;
            }
        }
        if (!uname_match) continue; // Not the right username, go to next line
        int j = 0;
        bool pass_match = true;
        for (int i = delim_loc + 2; i < line.length(); i++, j++) {
            if (enc_pass[j] != line[i]) {
                pass_match = false;
                break;
            }
        }
        if (!pass_match) { // Username found, but incorrect password
            (*response_code) = 2;
            return;
        }
        else { // Username and password match
            (*response_code) = 0;
            return;
        }
    }
    (*response_code) = 1; // Username not found
}

/*
    Takes the previously received buffer, processes the username and password, returns wether
    the user is authenticated (guests or failure = false), and sets the response code to be sent
    (0 = success, 1 = username not found, 2 = incorrect password).
*/
bool ServerM::authenticate(unsigned char *buffer, int numbytes, unsigned char *response_code) {
    // Skip the first byte that describes the required function
    string uname = "";
    string enc_pass = "";
    int i = 1;
    for (; buffer[i] != 0; i++) {
        uname += buffer[i];
    }
    for (; i < numbytes; i++) {
        enc_pass += buffer[i];
    }
    if (enc_pass.length() == 0){
        (*response_code) = 0;
        return false;
    }
    lookup_credentials(uname, enc_pass, response_code);
    return (*response_code) == 0;
}

void ServerM::send_response_code(unsigned char response_code) {

    if (send(socketfd_tcp, &response_code, 1, 0) == -1) {
        perror("send");
        exit(4);
    }
}

int main(int argc, char * argv[]) {
    ServerM main_server;
    // Main accept() loop
    while (1) {
        uint16_t port;
        int new_fd = main_server.accept_connection(&port);
        int numbytes;
        unsigned char *buffer;
        if (!fork()) {
            main_server.close_parent_socket();
            bool authenticated = false;
            unsigned char response_code;
            while (1) {
                buffer = main_server.receive_tcp(new_fd, &numbytes);
                if (numbytes == 0) { // Client disconnected
                    main_server.close_child_socket(new_fd);
                    exit(0);
                }
                if (buffer[0] == 0) { // Authentication
                    authenticated = main_server.authenticate(buffer, numbytes, &response_code);
                    main_server.send_response_code(response_code);
                }
                else if (buffer[0] == 1) { // Query

                }
                else if (buffer[0] == 2) { // Reservation

                }
            }
        }
        main_server.close_child_socket(new_fd);
    }
}