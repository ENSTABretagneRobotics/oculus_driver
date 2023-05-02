#include <iostream>
using namespace std;

#include <oculus_driver/Recorder.h>
#include <oculus_driver/helpers.h>
#include <oculus_driver/print_utils.h>
using namespace oculus;

void handle_oculus_message(const std::vector<uint8_t>& data)
{
    auto header = *reinterpret_cast<const OculusMessageHeader*>(data.data());
    if(header.msgId != messageSimplePingResult) {
        cout << "Got non-ping message" << endl;
    }
    else {
        if(header.msgVersion != 2) {
            cout << *reinterpret_cast<const OculusSimplePingResult*>(data.data()) << endl;
            cout << "PingResultV1" << endl;
        }
        else {
            cout << *reinterpret_cast<const OculusSimplePingResult2*>(data.data()) << endl;
            cout << "PingResultV2" << endl;
        }
    }
}

int main(int argc, char** argv)
{
    if(argc < 2) {
        throw std::runtime_error("Must give a .oculus file as parameter");
    }
    cout << "Opening file : " << argv[1] << endl;

    FileReader file(argv[1]);
    unsigned int count = 0;
    std::shared_ptr<const Message> msg;
    do {
        msg = file.read_next_message();
        if(!msg) break;
        if(!msg->is_ping_message()) continue;
        count++;
    } while(count < 100);

    if(!msg || !msg->is_ping_message()) {
        std::cerr << "Could not read an oculus ping message from " << argv[1] << std::endl;
        return -1;
    }

    auto bearings = get_ping_bearings(msg->data());
    cout << "bearings :\n";
    for(auto b : bearings) {
        cout << " " << 180.0*b / M_PI;
    }
    cout << endl;

    auto pingData  = get_ping_acoustic_data(msg->data());
    auto msgHeader = msg->header();
    if(msgHeader.msgVersion != 2) {
        auto metadata = *reinterpret_cast<const OculusSimplePingResult*>(msg->data().data());
        write_pgm("output_polar.pgm", metadata.nBeams, metadata.nRanges, pingData.data());
        cout << metadata << endl;
        std::vector<float> cartesianImage;
        auto imageShape = image_from_ping_data(metadata, msg->data(), cartesianImage);
        write_pgm("output_cartesian.pgm",
                  imageShape.first, imageShape.second, cartesianImage.data());

    }
    else {
        auto metadata = *reinterpret_cast<const OculusSimplePingResult2*>(msg->data().data());
        cout << metadata << endl;
        write_pgm("output_polar.pgm", metadata.nBeams, metadata.nRanges, pingData.data());
        std::vector<float> cartesianImage;
        auto imageShape = image_from_ping_data(metadata, msg->data(), cartesianImage);
        write_pgm("output_cartesian.pgm",
                  imageShape.first, imageShape.second, cartesianImage.data());
    }

    return 0;
}


