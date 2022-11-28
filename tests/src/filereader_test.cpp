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
    while(auto msg = file.read_next_message()) {
        cout << "Got item : " << msg->header() << endl;
    }

    return 0;
}
