/******************************************************************************
 * oculus_driver driver library for Blueprint Subsea Oculus sonar.
 * Copyright (C) 2020 ENSTA-Bretagne
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *****************************************************************************/

#include <iostream>
#include <sstream>
using namespace std;

#include <oculus_driver/AsyncService.h>
#include <oculus_driver/SonarDriver.h>
#include <oculus_driver/Recorder.h>
using namespace oculus;

void print_ping(const OculusSimplePingResult& pingMetadata,
                const std::vector<uint8_t>& pingData)
{
    cout << "=============== Got Ping :" << endl;
    //cout << pingMetadata << endl;
}

void print_dummy(const OculusMessageHeader& msg)
{
    cout << "=============== Got dummy :" << endl;
    //cout << msg << endl;
}

void print_all(const Message::ConstPtr& msg)
{
    switch(msg->header().msgId) {
        case messageSimplePingResult:
            std::cout << "Got messageSimplePingResult" << endl;
            break;
        case messageDummy:
            std::cout << "Got messageDummy" << endl;
            break;
        case messageSimpleFire:
            std::cout << "Got messageSimpleFire" << endl;
            break;
        case messagePingResult:
            std::cout << "Got messagePingResult" << endl;
            break;
        case messageUserConfig:
            std::cout << "Got messageUserConfig" << endl;
            break;
        default:
            break;
    }

}

void recorder_callback(const Recorder* recorder,
                       const Message::ConstPtr& msg)
{
    recorder->write(msg);
}

int main()
{
    //Sonar sonar;
    AsyncService ioService;
    SonarDriver sonar(ioService.io_service());
    
    //sonar.add_ping_callback(&print_ping);
    //sonar.add_dummy_callback(&print_dummy);
    sonar.add_message_callback(&print_all);

    ioService.start();

    Recorder recorder;
    recorder.open("output.oculus", true);

    sonar.add_message_callback(std::bind(recorder_callback, &recorder, std::placeholders::_1));

    getchar();

    recorder.close();

    ioService.stop();

    return 0;
}


