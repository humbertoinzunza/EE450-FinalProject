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
#define EXTRA_USERDB    "extra_member.txt"


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
    freeaddrinfo(servinfo_tcp);
    freeaddrinfo(servinfoS);
    freeaddrinfo(servinfoD);
    freeaddrinfo(servinfoU);
    close(socketfd_udp);
    close(socketfd_tcp);
    close(socketfdS);
    close(socketfdD);
    close(socketfdU);
}

/*
    Sets up the structs needed for the sockets.
    Source: Beej’s Guide to Network Programming.
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

/*
    Source: Beej’s Guide to Network Programming.
*/
void ServerM::create_sockets() {
    struct addrinfo *p;
    // Loop through the linked list to get a valid socket to receive incoming UDP connections
    for (p = servinfo_udp; p != NULL; p = p->ai_next) {
        if ((socketfd_udp = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("listener: socket");
            continue; // Go for the next item in the list
        }
        // Try to bind the socket to "listen" for incoming connections
        if (bind(socketfd_udp, p->ai_addr, p->ai_addrlen) == -1) {
            close(socketfd_udp);
            perror("listener: bind");
            continue;
        }
        break; // No errors while creating the socket, so we got a valid one in the list
    }

    // No valid socket was found in the linked list
    if (p == NULL) {
        fprintf(stderr, "Failed to create UDP socket (M)\n");
        exit(2);
    }

    struct sockaddr_in *address = (struct sockaddr_in *)p->ai_addr;
    my_udp_port = htons(address->sin_port);

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
    for (pU = servinfoU; pU != NULL; pU = pU->ai_next) {
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
    for (p = servinfo_tcp; p != NULL; p = p->ai_next) {
        if ((socketfd_tcp = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("listener: socket");
            continue; // Go for the next item in the list
        }
        int yes = 1;
        if ((setsockopt(socketfd_tcp, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))) == -1) {
            perror("setsockopt");
            exit(2);
        }
        // Try to bind the socket to "listen" for incoming connections
        if (bind(socketfd_tcp, p->ai_addr, p->ai_addrlen) == -1) {
            close(socketfd_tcp);
            perror("listener: bind");
            continue;
        }
        break; // No errors while creating the socket, so we got a valid one in the list
    }

    // No valid socket was found in the linked list
    if (p == NULL) {
        fprintf(stderr, "Failed to create TCP socket (M)\n");
        exit(2);
    }
    address = (struct sockaddr_in *)p->ai_addr;
    my_tcp_port = htons(address->sin_port);
}

// Source: Beej’s Guide to Network Programming
void sigchld_handler(int s) {
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

/*
    Uses the function listen() to look for incoming TCP connections.
    Obtained from Beej’s Guide to Network Programming.
*/
void ServerM::listen_for_connections() {
    if (listen(socketfd_tcp, BACKLOG) == -1) {
        perror("listen");
        exit(4);
    }
    // Reap dead processes
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
    unsigned char buf[UDP_MAXBUFLEN];
    // Get data from UDP socket and read the sender's port
    receive_udp(buf, &numbytes, &port);
    // Check who is the sender given the port
    char server_id = get_sender_id(port);
    // Put the data into the corresponding map
    fill_room_data(server_id, buf, numbytes);
    printf("The main server has received the room status from Server %c using UDP over port %i.\n", server_id, my_udp_port);
    // Repeat two more times
    receive_udp(buf, &numbytes, &port);
    server_id = get_sender_id(port);
    fill_room_data(server_id, buf, numbytes);
    printf("The main server has received the room status from Server %c using UDP over port %i.\n", server_id, my_udp_port);
    receive_udp(buf, &numbytes, &port);
    server_id = get_sender_id(port);
    fill_room_data(server_id, buf, numbytes);
    printf("The main server has received the room status from Server %c using UDP over port %i.\n", server_id, my_udp_port);
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
    // Pick the appropiate map data structure given the server ID
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
void ServerM::receive_udp(unsigned char *buf, int *numbytes, uint16_t *sender_port) {
    struct sockaddr from_addr;
    memset(&from_addr, 0, sizeof from_addr);
    socklen_t from_len = sizeof from_addr;
    // Receive data from the given server (Beej’s Guide to Network Programming)
    if (((*numbytes) = recvfrom(socketfd_udp, buf, UDP_MAXBUFLEN - 1, 0, &from_addr, &from_len)) == -1) {
        perror("receive_from");
        exit(3);
    }
    // Get the sender's port
    struct sockaddr_in *sender = (struct sockaddr_in*)&from_addr;
    (*sender_port) = htons(sender->sin_port);
}

/*
    Print the room data for the chosen server. For debugging purposes.
*/
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
    Accepts an incoming connection.
    Source: // Beej’s Guide to Network Programming
*/
int ServerM::accept_connection() {
    int new_fd = accept(socketfd_tcp, NULL, NULL);
    if (new_fd == -1) {
        perror("accept");
        exit(5);
    }
    return new_fd;
}

void ServerM::receive_tcp(unsigned char *buffer, int *numbytes, int socketfd) {
    // Beej’s Guide to Network Programming
    if (((*numbytes) = recv(socketfd, buffer, TCP_MAXBUFLEN - 1, 0)) == -1) {
        perror("recv");
        exit(6);
    }
}

void ServerM::close_child_socket(int sd) {
    close(sd);
}

/*
    Looks up credentials in the appropiate file. Only for SHA256.
*/
void ServerM::lookup_sha256_credentials(unsigned char *enc_username, unsigned char *enc_password, unsigned char *response_code) {
    ifstream data_file;
    data_file.open(EXTRA_USERDB);
    bool uname_match = false;
    bool pass_match = false;
    char const hex_characters[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
    unsigned char current_byte[2];
    int i, j = 0;
    // Go line by line
    for (string line; getline(data_file, line);) {
        // Location of delimiter
        uname_match = true;
        for (i = 0, j = 0; i < 32; i++, j += 2) {
            // Convert current byte to hex representation (byte[0] is most significant nibble)
            current_byte[0] = hex_characters[(enc_username[i] & 0xF0) >> 4];
            current_byte[1] = hex_characters[enc_username[i] & 0x0F];
            // If the two current hex chars in the line don't match with the current unsigned char's hex representation
            // then the username doesn't matches. End loop early.
            if (current_byte[0] != line[j] || current_byte[1] != line[j + 1]) {
                uname_match = false;
                break; // Failed
            }
        }
        if (!uname_match) continue; // Username didn't match so go to next line
        pass_match = true;
        for (i = 0, j += 2; i < 32; i++, j += 2) {
            // Convert current byte to hex representation (byte[0] is most significant nibble)
            current_byte[0] = hex_characters[(enc_password[i] & 0xF0) >> 4];
            current_byte[1] = hex_characters[enc_password[i] & 0x0F];
            // If the two current hex chars in the line don't match with the current unsigned char's hex representation
            // then the password doesn't matches. End loop early.
            if (current_byte[0] != line[j] || current_byte[1] != line[j + 1]) {
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
    Looks up credentials in the appropiate file. Only for basic encryption algorithm.
*/
void ServerM::lookup_credentials(string uname, string enc_pass, unsigned char *response_code) {
    ifstream data_file;
    data_file.open(USERDB);
    int delim_loc;
    bool uname_match;
    for (string line; getline(data_file, line);) {
        // Location of delimiter
        delim_loc = line.find(',');
        // If the username length is not the same as the index of the delimiter, then the lengths don't match
        // and it can't possibly be the username in question, so go to the next line
        if (uname.length() != delim_loc) continue;
        uname_match = true;
        for (int i = 0; i < delim_loc; i++) {
            if (uname[i] != line[i]) { // If one character is different, then it's not a match
                uname_match = false;
                break;
            }
        }
        if (!uname_match) continue; // Not the right username, go to next line
        int j = 0;
        bool pass_match = true;
        for (int i = line.find(' ', delim_loc) + 1; i < line.length(); i++, j++) {
            if (enc_pass[j] != line[i]) { // If one character is different, then it's not a match
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
    Converts an array of unsigned chars to their hexadecimal string representation.
*/
string ServerM::uchar_tohex(unsigned char *chars, int length) {
    char const hex_characters[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
    char c_str[length * 2];
    for (int i = 0, j = 0; i < length; i++, j += 2) {
        c_str[j] = hex_characters[(chars[i] & 0xF0) >> 4];
        c_str[j + 1] = hex_characters[chars[i] & 0x0F];
    }
    string str(c_str);
    return str;
}

/*
    Takes the previously received buffer, processes the username and password, returns wether
    the user is authenticated (guests or failure = false), and sets the response code to be sent
    (0 = success, 1 = username not found, 2 = incorrect password).
*/
bool ServerM::authenticate(unsigned char *buffer, int numbytes, unsigned char *response_code, string *username, bool use_sha256) {
    // If using sha256 algorithm
    if (use_sha256) {
        unsigned char enc_uname[32];
        unsigned char enc_pass[32];
        
        int i = 0;
        // Fill the username array (32 bytes) from the input buffer
        for (; i < 32; i++) {
            enc_uname[i] = buffer[i + 1];
        }
        int j = 0;
        bool pass_is_empty = true;
        // Fill the password array (32 bytes) from the input buffer
        for (; i < 64; i++, j++) {
            enc_pass[j] = buffer[i + 1];
            // If all bytes of the password are 0, then there is no password (guest login)
            if (enc_pass[j] != 0) pass_is_empty = false;
        }
        // Convert the username to hexadecimal string representation
        string uname_str = uchar_tohex(enc_uname, 32);
        if (pass_is_empty) { // It's a guest login
            (*response_code) = 0;
            printf("The main server received the guest request for %s using TCP over port %i. ", uname_str.c_str(), my_tcp_port);
            printf("The main server accepts %s as a guest.\n", uname_str.c_str());
            (*username) = uname_str;
            return false;
        }
        printf("The main server received the authentication for %s using TCP over port %i.\n", uname_str.c_str(), my_tcp_port);
        // If it's not a guest login look for the credentials in the file
        lookup_sha256_credentials(enc_uname, enc_pass, response_code);
        // Store the username
        (*username) = uname_str;
    }
    else {
        // Skip the first byte that describes the required function
        string uname = "";
        string enc_pass = "";
        int i = 1;
        // Put the username into a string
        for (; buffer[i] != 0; i++) {
            uname += buffer[i];
        }
        i++; // Skip separator
        // Put the password into a string
        for (; i < numbytes; i++) {
            enc_pass += buffer[i];
        }
        // If there is no password, then it is a guest login
        if (enc_pass.length() == 0){
            (*response_code) = 0;
            printf("The main server received the guest request for %s using TCP over port %i. ", uname.c_str(), my_tcp_port);
            printf("The main server accepts %s as a guest.\n", uname.c_str());
            (*username) = uname;
            return false;
        }
        printf("The main server received the authentication for %s using TCP over port %i.\n", uname.c_str(), my_tcp_port);
        // If the password has any characters, then look for the credentials into the appropiate file
        lookup_credentials(uname, enc_pass, response_code);
        // Store the username
        (*username) = uname;
    }
    // Return a boolean indicating if the username is authenticated (not a guest)
    return (*response_code) == 0;
}

void ServerM::send_response_code(int socketfd, unsigned char response_code) {
    // Beej’s Guide to Network Programming
    if (send(socketfd, &response_code, 1, 0) == -1) {
        perror("send");
        exit(4);
    }
}

void ServerM::process_query_request(unsigned char *buffer, int numbytes, unsigned char *response_code, string username) {
    // buffer[0] contains the request type code (1) and buffer[1] contains the letter of the room ID
    char backend_server = buffer[1];
    // If it starts with any letter other than S, D, or U then it's not a valid room layout
    if (backend_server != 'S' && backend_server != 'D' && backend_server != 'U') {
        (*response_code) = 2; // Not able to find the room layout.
        return;
    }
    // Get the room number from the buffer
    char room_number_str[numbytes - 2];
    for (int i = 2; i < numbytes; i++) {
        room_number_str[i - 2] = buffer[i];
    }
    // Put the room number into an unsigned short
    unsigned short room_number = (unsigned short)atoi(room_number_str);
    printf("The main server has received the availability request on Room %c%i from %s", backend_server, room_number, username.c_str());
    printf(" using TCP over port %i.\n", my_tcp_port);
    struct addrinfo *pointer;
    // Find the pointer of the socket address where the request is to be sent
    switch (backend_server) {
        case 'S': pointer = pS; break;
        case 'D': pointer = pD; break;
        case 'U': pointer = pU; break;
        // Not able to find the room layout.
        default:(*response_code) = 2; return;
    }
    unsigned char out_buffer[3];
    out_buffer[0] = 0; // Query request
    out_buffer[1] = (room_number >> 8) & 0xFF; // Most significant byte 
    out_buffer[2] = room_number & 0xFF; // Least significant byte
    // Send data to the appropriate server (Beej’s Guide to Network Programming)
    if ((numbytes = sendto(socketfd_udp, out_buffer, 3, 0, pointer->ai_addr, pointer->ai_addrlen)) == -1) {
        perror("send");
        exit(3);
    }
    printf("The main server sent a request to Server %c.\n", backend_server);
    unsigned char in_buf[UDP_MAXBUFLEN];
    int in_numbytes;
    uint16_t sender_port;
    // Get response from server
    receive_udp(in_buf, &in_numbytes, &sender_port);
    printf("The main server received the response from Server %c using UDP over port %i.\n", backend_server, my_udp_port);
    (*response_code) = in_buf[0]; // Pass the response code obtained from backend server
}

void ServerM::process_reservation_request(unsigned char *buffer, int numbytes, unsigned char *response_code, string username, bool is_authenticated) {
    // buffer[0] contains the request type code (1) and buffer[1] contains the letter of the room ID
    char backend_server = buffer[1];
    // If it starts with any letter other than S, D, or U then it's not a valid room layout
    if (backend_server != 'S' && backend_server != 'D' && backend_server != 'U') {
        (*response_code) = 2; // Not able to find the room layout.
        return;
    }
    // Get the room number from the buffer
    char room_number_str[numbytes - 2];
    for (int i = 2; i < numbytes; i++) {
        room_number_str[i - 2] = buffer[i];
    }
    // Put the room number into an unsigned short
    unsigned short room_number = (unsigned short)atoi(room_number_str);
    printf("The main server has received the reservation request on Room %c%i from %s", backend_server, room_number, username.c_str());
    printf(" using TCP over port %i.\n", my_tcp_port);
    if (!is_authenticated) {
        printf("%s cannot make a reservation.\n", username.c_str());
        (*response_code) = 3; // Unauthenticated client
        return;
    }
    struct addrinfo *pointer;
    // Find the pointer of the socket address where the request is to be sent
    switch (backend_server) {
        case 'S': pointer = pS; break;
        case 'D': pointer = pD; break;
        case 'U': pointer = pU; break;
        // Not able to find the room layout.
        default:(*response_code) = 2; return;
    }
    unsigned char out_buffer[3];
    out_buffer[0] = 1; // Reservation request
    out_buffer[1] = (room_number >> 8) & 0xFF; // Most significant byte 
    out_buffer[2] = room_number & 0xFF; // Least significant byte 
    // Send data to the appropriate server (Beej’s Guide to Network Programming)
    if ((numbytes = sendto(socketfd_udp, out_buffer, 3, 0, pointer->ai_addr, pointer->ai_addrlen)) == -1) {
        perror("send");
        exit(3);
    }
    printf("The main server sent a request to Server %c.\n", backend_server);
    unsigned char in_buf[UDP_MAXBUFLEN];
    int in_numbytes;
    // Get response from server
    uint16_t sender_port;
    receive_udp(in_buf, &in_numbytes, &sender_port);
    if (in_buf[0] == 0) { // Room status was updated
        printf("The main server received the response and the updated room status from Server %c using UDP over port %i.\n", backend_server, my_udp_port);
        // Update the room status
        switch (backend_server) {
            case 'S': rooms_S[room_number]--; break;
            case 'D': rooms_D[room_number]--; break;
            case 'U': rooms_U[room_number]--; break;
            default: break;
        }
        printf("The room status of Room %c%s has been updated.\n", backend_server, room_number_str);
    }
    else { // Room count was  not updated
        printf("The main server received the response from Server %c using UDP over port %i.\n", backend_server, my_udp_port);
    }
    (*response_code) = in_buf[0]; // Pass the response code obtained from backend server
}

int main(int argc, char * argv[]) {
    bool use_sha256;
    // Check if sha256 will be used
    if (argc > 1) {
         char *arg = argv[1];
        if (arg[0] == '-' && arg[1] == 'e')
            use_sha256 = true;
        else
            use_sha256 = false;
    }
    else
        use_sha256 = false;
    // Instantiate the server object
    ServerM main_server;
    // Main accept() loop with fork() (Beej’s Guide to Network Programming)
    while (1) {
        // Accept incoming connections
        int new_fd = main_server.accept_connection();
        int numbytes;
        unsigned char *buffer;
        if (!fork()) { // Fork into child process
            main_server.close_parent_socket();
            bool authenticated = false;
            unsigned char response_code;
            string username;
            unsigned char buffer[TCP_MAXBUFLEN];
            while (1) {
                // Get data from the client
                main_server.receive_tcp(buffer, &numbytes, new_fd);
                if (numbytes == 0) { // Client disconnected
                    main_server.close_child_socket(new_fd);
                    // cout << "Client disconnected." << endl;
                    exit(0);
                }
                if (buffer[0] == 0) { // Authentication
                    authenticated = main_server.authenticate(buffer, numbytes, &response_code, &username, use_sha256);
                    main_server.send_response_code(new_fd, response_code);
                    if (response_code == 0 && !authenticated)
                        cout << "The main server sent the guest response to the client." << endl;
                    else
                        cout << "The main server sent the authentication result to the client." << endl;
                }
                else if (buffer[0] == 1) { // Query
                    main_server.process_query_request(buffer, numbytes, &response_code, username);
                    main_server.send_response_code(new_fd, response_code);
                    cout << "The main server sent the availability information to the client." << endl;
                }
                else if (buffer[0] == 2) { // Reservation
                    main_server.process_reservation_request(buffer, numbytes, &response_code, username, authenticated);
                    main_server.send_response_code(new_fd, response_code);
                    if (!authenticated)
                        cout << "The main server sent the error message to the client." << endl;
                    else
                        cout << "The main server sent the reservation result to the client." << endl;
                }
            }
        }
        // Parent process doesn't need this socket
        main_server.close_child_socket(new_fd);
    }
}