#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <map>
#include <string>

using namespace std;

class ServerM
{
    private:
        map<unsigned short, bool> connected_clients;
        map<unsigned short, unsigned short> rooms_S;
        map<unsigned short, unsigned short> rooms_D;
        map<unsigned short, unsigned short> rooms_U;
        struct addrinfo *servinfo_udp;
        struct addrinfo *p_udp;
        int socketfd_udp;
        struct addrinfo *servinfoS;
        struct addrinfo *pS;
        int socketfdS;
        struct addrinfo *servinfoD;
        struct addrinfo *pD;
        int socketfdD;
        struct addrinfo *servinfoU;
        struct addrinfo *pU;
        int socketfdU;
        struct addrinfo *servinfo_tcp;
        struct addrinfo *p_tcp;
        int socketfd_tcp;

        void get_addrinfos();
        void create_sockets();
        void receive_initial_data();
        char get_sender_id(uint16_t);
        void fill_room_data(char, unsigned char *, int);
        void listen_for_connections();
        void lookup_credentials(string, string, unsigned char *);

    public:
        ServerM();
        ~ServerM();
        void print_room_data(char);
        unsigned char *receive_udp(int *, uint16_t *);
        unsigned char *receive_tcp(int, int *);
        int accept_connection(uint16_t *); 
        void close_parent_socket();
        void close_child_socket(int);
        bool authenticate(unsigned char *, int, unsigned char *);
        void send_response_code(unsigned char);
};