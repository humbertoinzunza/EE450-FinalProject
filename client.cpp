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
#include <cmath>

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

unsigned int *Client::sha256(string msg) {
    // 512-bit block, should be enough for password of length 5-50
    unsigned char block[64];
    unsigned int i = 0;
    for (; i < msg.length(); i++) {
        block[i] = msg[i];
    }
    // First byte after data is 1000 0000
    block[i++] = 0x80;
    // The rest of the bytes are filled by zeros, leaving 8 bytes at the end of the block for the message length
    for (; i < 56; i++) {
        block[i] = 0x00;
    }
    // 64-bits length, put at the end of the block
    unsigned long bit_count = msg.length() * 8;
    i = 63;
    for (unsigned char *ptr = (unsigned char *)&bit_count; i >= 56; i--, ptr++) {
        block[i] = *ptr;
    }
    
    unsigned int msg_schedule[64] = { 0 };
    // Copy first 512-bit chunk (the only one in this case) into the first 16 entries of the message schedule array
    unsigned int j = 0;
    for (unsigned char *ptr, i = 0; i < 16; i++, j +=4) {
        ptr = (unsigned char *)&msg_schedule[i];
        *ptr++ = block[j + 3];
        *ptr++ = block[j + 2];
        *ptr++ = block[j + 1];
        *ptr++ = block[j];
    }
    // Calculate the other 48 entries
    unsigned int *current_w[4] = { &msg_schedule[0], &msg_schedule[1], &msg_schedule[9], &msg_schedule[14] };
    unsigned int alpha_0 = 0;
    unsigned int alpha_1 = 0;
    for (i = 16; i < 64; i++) {
        // Rigth rotate ^ Right rotate ^ Right shift
        alpha_0 = ((*current_w[1] >> 7) | (*current_w[1] << 25)) ^ ((*current_w[1] >> 18) | (*current_w[1] << 14)) ^ (*current_w[1] >> 3);
        alpha_1 = ((*current_w[3] >> 17) | (*current_w[3] << 15)) ^ ((*current_w[3] >> 19) | (*current_w[3] << 13)) ^ (*current_w[3] >> 10);
        msg_schedule[i] = *current_w[0]++ + alpha_0 + *current_w[2]++ + alpha_1;
        current_w[1]++;
        current_w[3]++;
    }
    // Initial hash value "first 32 bits of the fractional parts of the square roots of the first 8 primes 2..19)"
    unsigned int *H = new unsigned int[8] { 0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A, 0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19 };
    // Initialize array K "first 32 bits of the fractional parts of the cube roots of the first 64 primes 2..311"
    unsigned int K[64] = {
        0X428A2F98, 0X71374491, 0XB5C0FBCF, 0XE9B5DBA5,
        0X3956C25B, 0X59F111F1, 0X923F82A4, 0XAB1C5ED5,
        0XD807AA98, 0X12835B01, 0X243185BE, 0X550C7DC3,
        0X72BE5D74, 0X80DEB1FE, 0X9BDC06A7, 0XC19BF174,
        0XE49B69C1, 0XEFBE4786, 0X0FC19DC6, 0X240CA1CC,
        0X2DE92C6F, 0X4A7484AA, 0X5CB0A9DC, 0X76F988DA,
        0X983E5152, 0XA831C66D, 0XB00327C8, 0XBF597FC7,
        0XC6E00BF3, 0XD5A79147, 0X06CA6351, 0X14292967,
        0X27B70A85, 0X2E1B2138, 0X4D2C6DFC, 0X53380D13,
        0X650A7354, 0X766A0ABB, 0X81C2C92E, 0X92722C85,
        0XA2BFE8A1, 0XA81A664B, 0XC24B8B70, 0XC76C51A3,
        0XD192E819, 0XD6990624, 0XF40E3585, 0X106AA070,
        0X19A4C116, 0X1E376C08, 0X2748774C, 0X34B0BCB5,
        0X391C0CB3, 0X4ED8AA4A, 0X5B9CCA4F, 0X682E6FF3,
        0X748F82EE, 0X78A5636F, 0X84C87814, 0X8CC70208,
        0X90BEFFFA, 0XA4506CEB, 0XBEF9A3F7, 0XC67178F2
    };
    // Initialize working variables
    unsigned int a = H[0];
    unsigned int b = H[1];
    unsigned int c = H[2];
    unsigned int d = H[3];
    unsigned int e = H[4];
    unsigned int f = H[5];
    unsigned int g = H[6];
    unsigned int h = H[7];
    unsigned int majority;
    unsigned int choice;
    unsigned int sigma_0;
    unsigned int sigma_1 ;
    unsigned int temp_1;
    unsigned int temp_2;
    for (i = 0; i < 64; i++) {
        majority = (a & b) ^ (a & c) ^ (b & c);
        choice = (e & f) ^ ((~e) & g);
        sigma_0 = ((a >> 2) | (a << 30)) ^ ((a >> 13) | (a << 19)) ^ ((a >> 22) | (a << 10));
        sigma_1 = ((e >> 6) | (e << 26)) ^ ((e >> 11) | (e << 21)) ^ ((e >> 25) | (e << 7));
        temp_1 = h + sigma_1 + choice + K[i] + msg_schedule[i];
        temp_2 = sigma_0 + majority;
        // Update working variables
        h = g;
        g = f;
        f = e;
        e = d + temp_1;
        d = c;
        c = b;
        b = a;
        a = temp_1 + temp_2;
    }
    // Add the working variables to the current hash value
    H[0] += a;
    H[1] += b;
    H[2] += c;
    H[3] += d;
    H[4] += e;
    H[5] += f;
    H[6] += g;
    H[7] += h;
    // Return hash value
    return H;
}

/*
    Sets up the structs needed for the sockets.
    Source: Beej’s Guide to Network Programming.
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

/*
    Creates the desired sockets and gets their file descriptors.
    Source: Beej’s Guide to Network Programming.
*/
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

    struct sockaddr_in *my_address;
    socklen_t my_address_len = sizeof(my_address);
    if (getsockname(socketfd, (struct sockaddr *)my_address, &my_address_len) == -1) {
        perror("getsockname");
        exit(2);
    }
    my_tcp_port = htons(my_address->sin_port);
}

bool Client::authenticate(bool use_sha256) {
    string username, password;
    get_user_credentials(&username, &password);
    id = username;
    if (!use_sha256) {
        string enc_username = basic_encrypt(username);
        string enc_password = basic_encrypt(password);
        // Build out buffer
        int buf_len = enc_username.length() + enc_password.length() + 2; // Data size + one byte of request code + one byte of separation
        unsigned char buffer[buf_len];
        int i = 1;
        int stop = enc_username.length() + 1;
        buffer[0] = 0; // Authentication request
        for (; i < stop; i++) {
            buffer[i] = enc_username[i - 1]; // Username
        }
        buffer[i++] = 0; // Separation
        stop = buf_len;
        int j = 0;
        for (; i < stop; i++, j++) {
            buffer[i] = enc_password[j]; // Encrypted password
        }
        // Beej’s Guide to Network Programming
        if (send(socketfd, buffer, buf_len, 0) == -1) {
            perror("send");
            exit(4);
        }
    }
    else { // Authentication using sha256
        unsigned int *enc_username = sha256(username);
        unsigned int *enc_password;
        if (password.length() == 0)
            // All zeros for guest authentication (assuming no possible password can produce this hash)
            enc_password = new unsigned int[8] { 0, 0, 0, 0, 0, 0, 0, 0 };
        else
            enc_password = sha256(password);
        // Build out buffer
        int buf_len = 2 * 8 * sizeof (unsigned int) + 1; // 8 unsigned int for username, 8 unsigned int for password, and one byte for request code
        unsigned char out_buffer[buf_len];
        out_buffer[0] = 0; // Authentication request
        // Put encrypted username into out_buffer
        for (int i = 0, j = 1; i < 8; i++) {
            unsigned char *ptr = (unsigned char *)&enc_username[i] + 3;
            out_buffer[j++] = *ptr--;
            out_buffer[j++] = *ptr--;
            out_buffer[j++] = *ptr--;
            out_buffer[j++] = *ptr--;
        }
        // Put encrypted password into out_buffer
        for (int i = 0, j = 33; i < 8; i++) {
            unsigned char *ptr = (unsigned char *)&enc_password[i] + 3;
            out_buffer[j++] = *ptr--;
            out_buffer[j++] = *ptr--;
            out_buffer[j++] = *ptr--;
            out_buffer[j++] = *ptr--;
        }
        // Send to main server (Beej’s Guide to Network Programming)
        if (send(socketfd, out_buffer, buf_len, 0) == -1) {
            perror("send");
            exit(4);
        }
        // Free memory
        delete [] enc_username;
        delete [] enc_password;
    }
    printf("%s sent an authentication request to the main server.\n", id.c_str());

    // Get in buffer
    unsigned char in_buffer[MAXBUFLEN];
    int numbytes;
    get_response_code(in_buffer, &numbytes);
    print_authentication_error(username, password.length() == 0, in_buffer[0]);

    return in_buffer[0] == 0;
}

void Client::print_authentication_error(string username, bool is_guest, unsigned char code) {
    if (code == 0)
    {
        if (is_guest)
            printf("Welcome guest %s!\n", username.c_str());
        else
            printf("Welcome member %s!\n", username.c_str());
        id = username;
    }
    else if (code == 1) cout << "Failed login: Username does not exist." << endl;
    else if (code == 2) cout << "Failed login: Password does not match." << endl;
}

void Client::get_user_credentials(string *username, string *password) {
    cout << "Please enter the username: ";
    getline(cin, (*username));
    cout << "Please enter the password: ";
    getline(cin, (*password));
}

string to_lower(string s) {
    string result = s;
    for (unsigned int i = 0; i < s.length(); i++)
        result[i] = tolower(s[i]);
    return result;
}

/*
    Sends a buffer to the main server in which the first byte is the request code (1 = query, 2 = reservation).
*/
void Client::send_room_data(string room, unsigned char request_code) {
    int buf_len = room.length() + 1;
    unsigned char buffer[buf_len];
    buffer[0] = request_code;
    // Fill the buffer
    for (int i = 1; i < buf_len; i++)
        buffer[i] = room[i - 1];
    // Beej’s Guide to Network Programming
    if (send(socketfd, buffer, buf_len, 0) == -1) {
        perror("send");
        exit(4);
    }
}

void Client::get_response_code(unsigned char *in_buffer, int *numbytes) {
    // Beej’s Guide to Network Programming
    if (((*numbytes) = recv(socketfd, in_buffer, MAXBUFLEN - 1, 0)) == -1) {
        perror("recv");
        exit(3);
    }
}

void Client::print_response_msg(unsigned char response_code, bool is_query_request, string room_code) {
    printf("The client received the response from the main server using TCP over port %i.\n", my_tcp_port);
    if (is_query_request) {
        switch(response_code) {
            case 0: cout << "The requested room is available.\n" << endl; break;
            case 1: cout << "The requested room is not available.\n" << endl; break;
            case 2: cout << "Not able to find the room layout.\n" << endl; break;
            default: printf("Unexpected response code (%i).\n\n", response_code);
        }
    }
    else {
        switch(response_code) {
            case 0: printf("Congratulation! The reservation for Room %s has been made.\n\n", room_code.c_str()); break;
            case 1: cout << "Sorry! The requested room is not available.\n" << endl; break;
            case 2: cout << "Oops! Not able to find the room.\n" << endl; break;
            case 3: cout << "Permission denied: Guest cannot make a reservation.\n" << endl; break;
            default: printf("Unexpected response code (%i).\n\n", response_code);
        }
    }
}

/*
    Converts an array of unsigned chars to their hexadecimal string representation.
*/
string Client::uchar_tohex(unsigned char *chars, int length) {
    char const hex_characters[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
    char c_str[length * 2];
    for (int i = 0, j = 0; i < length; i++, j += 2) {
        c_str[j] = hex_characters[(chars[i] & 0xF0) >> 4];
        c_str[j + 1] = hex_characters[chars[i] & 0x0F];
    }
    string str(c_str);
    return str;
}

int main(int argc, char *argv[]) {
    bool use_sha256;
    if (argc > 1) {
        char *arg = argv[1];
        if (arg[0] == '-' && arg[1] == 'e')
            use_sha256 = true;
        else
            use_sha256 = false;
    }
    else
        use_sha256 = false;
    Client client;
    bool authenticated = client.authenticate(use_sha256);
    while (!authenticated){
        authenticated = client.authenticate(use_sha256);
    }
    string room_code;
    string request_type;
    unsigned char in_buffer[MAXBUFLEN];
    int numbytes;
    while (1) {
        cout << "Please enter the room code: ";
        cin >> room_code;
        cout << "Would you like to search for the availability or make a reservation? ";
        cout << "(Enter “Availability” to search for the availability or Enter “Reservation” to make a reservation ): ";
        cin >> request_type;
        request_type = to_lower(request_type);
        if (request_type == "availability") {
            client.send_room_data(room_code, 1);
            printf("%s sent an availability request to the main server.\n", client.id.c_str());
            client.get_response_code(in_buffer, &numbytes);
            client.print_response_msg(in_buffer[0], true, room_code);
        }
        else if (request_type == "reservation") {
            client.send_room_data(room_code, 2);
            printf("%s sent a reservation request to the main server.\n", client.id.c_str());
            client.get_response_code(in_buffer, &numbytes);
            client.print_response_msg(in_buffer[0], false, room_code);
        }
    }
}