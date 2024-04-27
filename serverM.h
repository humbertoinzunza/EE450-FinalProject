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
        uint16_t my_tcp_port;
        uint16_t my_udp_port;
        struct addrinfo *servinfo_udp;
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
        int socketfd_tcp;

        void get_addrinfos();
        void create_sockets();
        void receive_initial_data();
        char get_sender_id(uint16_t);
        void fill_room_data(char, unsigned char *, int);
        void listen_for_connections();
        void lookup_credentials(string, string, unsigned char *);
        void lookup_sha256_credentials(unsigned char *, unsigned char *, unsigned char *);
        string uchar_tohex(unsigned char *, int);

    public:
        ServerM();
        ~ServerM();
        void print_room_data(char);
        void receive_udp(unsigned char *, int *, uint16_t *);
        void receive_tcp(unsigned char *, int *, int);
        int accept_connection(); 
        void close_parent_socket();
        void close_child_socket(int);
        bool authenticate(unsigned char *, int, unsigned char *, string *, bool);
        void send_response_code(int, unsigned char);
        void process_query_request(unsigned char *, int, unsigned char *, string);
        void process_reservation_request(unsigned char *, int, unsigned char *, string, bool);
};