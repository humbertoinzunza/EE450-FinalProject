#include <map>

using namespace std;

class ServerS
{
    private:
        map<short, short> room_status;
        const char* data_filename = "suite.txt";
        void load_data();
    public:
        ServerS();   
};

