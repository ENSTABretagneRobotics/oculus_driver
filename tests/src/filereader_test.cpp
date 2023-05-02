#include <iostream>
using namespace std;

#include <oculus_driver/Recorder.h>
#include <oculus_driver/print_utils.h>
using namespace oculus;

int main(int argc, char** argv)
{
    if(argc < 2) {
        throw std::runtime_error("Must give a .oculus file as parameter");
    }
    cout << "Opening file : " << argv[1] << endl;

    FileReader file(argv[1]);
    //while(auto msg = file.read_next_ping()) {
    //    cout << "Got ping item : " << msg->header() << endl;
    //    cout << "size : " <<  msg->bearing_count() << 'x' << msg->range_count() << endl;
    //}
    auto msg = file.read_next_ping();
    cout << "Got ping    : " << msg->header() << endl;
    cout << "size        : " <<  msg->bearing_count() << 'x' << msg->range_count() << endl;
    cout << "master mode : " << (unsigned int)msg->master_mode() << endl;
    cout << "bearings :\n";
    auto bearingData = msg->bearing_data();
    for(int i = 0; i < msg->bearing_count(); i++) {
        cout << ' ' << 0.01 * bearingData[i];
    }
    cout << endl;

    return 0;
}
