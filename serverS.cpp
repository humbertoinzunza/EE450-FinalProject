#include "serverS.h"

ServerS::ServerS() : BackendServer("S", "41599", "single.txt") { } 

int main(int argc, char* argv[]) {
    ServerS server;
}   