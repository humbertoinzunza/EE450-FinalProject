#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

class ServerM
{
    private:
        struct addrinfo hintsS;
        struct addrinfo *servinfoS;
        struct addrinfo *pS;
        int socketfdS;
        struct addrinfo hintsD;
        struct addrinfo *servinfoD;
        struct addrinfo *pD;
        int socketfdD;
        struct addrinfo hintsU;
        struct addrinfo *servinfoU;
        struct addrinfo *pU;
        int socketfdU;
    public:
        ServerM();
        ~ServerM();
        void get_addrinfos();
        void create_sockets(bool);
};