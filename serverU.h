#include <iostream>
#include <fstream>
#include <map>
#include <string>

using namespace std;

class ServerU
{
    private:
        map<short, short> room_status;
        const char* data_filename = "suite.txt";
        void load_data();
    public:
        ServerU();   
};

