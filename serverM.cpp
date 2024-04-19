#include <serverM.h>
#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define BACKLOG 10

ServerM::ServerM() {

}

ServerM::~ServerM() {

}

void ServerM::get_addrinfo() {
    memset(&hints, 0, sizeof hints); // Set the struct to empty
    hints.
}
