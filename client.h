#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string>

using namespace std;

class Client
{
    private:
        uint16_t my_tcp_port;
        struct addrinfo *servinfo;
        struct addrinfo *p;
        int socketfd;
        void get_addrinfos();
        void create_sockets();
        void get_user_credentials(string *, string *);
        void print_authentication_error(string, bool, unsigned char);
        
    public:
        string id;
        Client();
        ~Client();
        string basic_encrypt(string);
        unsigned int *sha256(string);
        bool authenticate();
        void send_room_data(string, unsigned char);
        void get_response_code(unsigned char *, int *);
        void print_response_msg(unsigned char, bool, string);
};