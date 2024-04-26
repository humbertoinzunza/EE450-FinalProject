#include "serverU.h"

ServerU::ServerU() : BackendServer("U", "43599", "suite.txt") { } 

int main(int argc, char* argv[]) {
    ServerU server;
    server.receive_loop();
}   