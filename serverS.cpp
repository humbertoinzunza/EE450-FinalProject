#include "serverS.h";
#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>

/*
    Default constructor.
*/
ServerS::ServerS() {
    load_data();
}

/*
    Loads the data from the input file into the room_status map.
*/
void ServerS::load_data() {
    ifstream data_file;
    data_file.open(data_filename);
    for (string line; getline(data_file, line);) {
        string buffer = "";
        short number;
        short status;
        bool room_number_found = false;
        // Start at 1 to skip the first letter
        for (int i = 1; i < line.length(); i++) {
            if (!room_number_found)
            {
                if (line[i] == ',') {
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
        room_status.insert(number, status);
    }
}

int main(int argc, char* argv[]) {

}   