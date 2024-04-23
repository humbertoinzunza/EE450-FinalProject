#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string>

using namespace std;

class Client
{
    private:
        struct addrinfo *servinfo;
        struct addrinfo *p;
        int socketfd;
        void get_addrinfos();
        void create_sockets();
        void receive(unsigned char *, int *);
        void get_user_credentials(string *, string *);
        void print_authentication_error(string, unsigned char);
    public:
        Client();
        ~Client();
        string basic_encrypt(string);
        bool authenticate();
};