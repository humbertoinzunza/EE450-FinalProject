#include <string>

using namespace std;

class Client
{
    private:
        void get_addrinfos();
        void create_sockets();

    public:
        Client();
        ~Client();
        string basic_encrypt(string);
};