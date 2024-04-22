#include "serverD.h"

ServerD::ServerD() : BackendServer("D", "42599", "double.txt") { }

int main(int argc, char* argv[]) {
    ServerD server;
}   