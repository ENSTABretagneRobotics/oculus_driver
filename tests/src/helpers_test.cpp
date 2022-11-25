#include <iostream>
using namespace std;

#include <oculus_driver/Recorder.h>
#include <oculus_driver/helpers.h>
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
    blueprint::LogItem header;
    std::vector<uint8_t> data;
    unsigned int count = 0;
    while(file.read_next(header, data) && count < 100) {
        if(header.type == blueprint::rt_oculusSonar) {
            //handle_oculus_message(data);
            count++;
        }
    }

    auto bearings = get_ping_bearings(data);
    cout << "bearings :\n";
    for(auto b : bearings) {
        cout << " " << 180.0*b / M_PI;
    }
    cout << endl;

    auto pingData = get_ping_acoustic_data(data);
    auto msgHeader = *reinterpret_cast<const OculusMessageHeader*>(data.data());
    if(msgHeader.msgVersion != 2) {
        auto metadata = *reinterpret_cast<const OculusSimplePingResult*>(data.data());
        write_pgm("output_polar.pgm", metadata.nBeams, metadata.nRanges, pingData.data());
        cout << metadata << endl;
        std::vector<float> cartesianImage;
        auto imageShape = image_from_ping_data(metadata, data, cartesianImage);
        write_pgm("output_cartesian.pgm",
                  imageShape.first, imageShape.second, cartesianImage.data());

    }
    else {
        auto metadata = *reinterpret_cast<const OculusSimplePingResult2*>(data.data());
        cout << metadata << endl;
        write_pgm("output_polar.pgm", metadata.nBeams, metadata.nRanges, pingData.data());
        std::vector<float> cartesianImage;
        auto imageShape = image_from_ping_data(metadata, data, cartesianImage);
        write_pgm("output_cartesian.pgm",
                  imageShape.first, imageShape.second, cartesianImage.data());
    }

    return 0;
}


