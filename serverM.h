#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

class ServerM
{
    private:
        struct addrinfo hints;
        struct addrinfo *servinfo;
    public:
        ServerM();
        ~ServerM();
        void get_addrinfo();
};
