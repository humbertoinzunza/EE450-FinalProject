#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <map>
#include <string>

#define LOOPBACKIP  "127.0.0.1"
#define SERVERMPORT "44599"

using namespace std;

class BackendServer
{
    private:
        const char *SERVER_ID;
        const char *MY_UDP_PORT;
        const char *MY_DATA_FNAME;
        map<unsigned short, unsigned short> room_status;
        struct addrinfo hints;
        struct addrinfo *servinfo;
        struct addrinfo *p;
        int socketfd;
        struct addrinfo hintsM;
        struct addrinfo *servinfoM;
        struct addrinfo *pM;
        int socketfdM;
        
        void load_data();
        void get_addrinfos();
        void create_sockets();
        void send_initial_data();
        void receive_udp(unsigned char *, int *);
        void query(unsigned char *, unsigned char *);
        void reservation(unsigned char *, unsigned char *);
        void send_response(unsigned char);
    
    public:
        BackendServer(const char *, const char *, const char *);
        ~BackendServer();
        void receive_loop();
};

