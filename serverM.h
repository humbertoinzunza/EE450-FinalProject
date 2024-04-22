#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <map>

using namespace std;

class ServerM
{
    private:
        map<unsigned short, unsigned short> rooms_S;
        map<unsigned short, unsigned short> rooms_D;
        map<unsigned short, unsigned short> rooms_U;
        struct addrinfo hintsM;
        struct addrinfo *servinfoM;
        struct addrinfo *pM;
        int socketfdM;
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

        void get_addrinfos();
        void create_sockets();
        void receive_initial_data();
        char get_sender_id(uint16_t);
        void fill_room_data(char, unsigned char *, int);

    public:
        ServerM();
        ~ServerM();
        void print_room_data(char);
        unsigned char *receive_udp(int *, uint16_t *);
        
};