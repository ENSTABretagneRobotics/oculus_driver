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

#include <oculus_driver/StampBroadcaster.h>
#include <chrono>

namespace oculus {

StampBroadcaster::StampBroadcaster(const SonarDriver& sonar, uint16_t port) :
    socket_(*sonar.service()),
    broadcastEndpoint_(boost::asio::ip::address_v4::broadcast(), port)
{
    // using no throw versions of boost functions. 
    boost::system::error_code err;
    socket_.open(boost::asio::ip::udp::v4(), err);
    if(err) {
        std::cerr << "StampBroadcaster : failed to open socket" << std::endl;
        return;
    }

    socket_.set_option(Socket::reuse_address(true), err);
    if(err) {
        std::cerr << "StampBroadcaster : failed to set reuse_address option." << std::endl;
        return;
    }
    socket_.set_option(Socket::broadcast(true), err);
    if(err) {
        std::cerr << "StampBroadcaster : failed to set broadcast option." << std::endl;
        return;
    }
}

StampBroadcaster::~StampBroadcaster()
{
    if(socket_.is_open()) {
        boost::system::error_code err;
        socket_.close(err);
        std::cerr << "Failed to close broadcast socket." << std::endl;
    }
}

void StampBroadcaster::send(const Message::ConstPtr& msg) const
{
    if(!this->is_open()) {
        return;
    }

    uint64_t micros = std::chrono::duration_cast<std::chrono::microseconds>(
        msg->timestamp().time_since_epoch()).count();

    // performing a crude json serialization.
    std::ostringstream oss;
    oss << "{\"oculusId\": "         << msg->header().oculusId
        << ", \"deviceId\": "        << msg->header().srcDeviceId
        << ", \"timestampMicros\": " << micros << '}';
    std::string data = oss.str();
    
    boost::system::error_code err;
    socket_.send_to(boost::asio::buffer(data.c_str(), data.size()), broadcastEndpoint_,
                    0, err);
    if(err) {
        std::cerr << "Failed to broadcast message info." << std::endl;
    }
}


} //namespace oculus
