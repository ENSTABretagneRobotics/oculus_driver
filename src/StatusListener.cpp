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

#include <oculus_driver/StatusListener.h>

namespace oculus {

using namespace std::placeholders;

StatusListener::StatusListener(const IoServicePtr& service,
                               unsigned short listeningPort) :
    socket_(*service),
    remote_(boost::asio::ip::address_v4::any(), listeningPort)
{
    boost::system::error_code err;
    socket_.open(boost::asio::ip::udp::v4(), err);

    //socket_.set_option(boost::asio::socket_base::broadcast(true));
    if(err)
        throw std::runtime_error("oculus::StatusListener : Error opening socket");

    socket_.bind(remote_);
    if(err)
        throw std::runtime_error("oculus::StatusListener : Socket remote error");

    std::cout << "oculus::StatusListener : listening to remote : " << remote_ << std::endl;
    this->get_one_message();
}

void StatusListener::get_one_message()
{
    socket_.async_receive(boost::asio::buffer(static_cast<void*>(&msg_), sizeof(msg_)),
                          std::bind(&StatusListener::message_callback, this, _1, _2));
}

unsigned int StatusListener::add_callback(
    const std::function<void(const OculusStatusMsg&)>& callback)
{
    return callbacks_.add_callback(callback);
}

bool StatusListener::remove_callback(unsigned int index)
{
    return callbacks_.remove_callback(index);
}

bool StatusListener::on_next_status(
    const std::function<void(const OculusStatusMsg&)>& callback)
{
    return callbacks_.add_single_shot(callback);
}


void StatusListener::message_callback(const boost::system::error_code& err,
                                      std::size_t bytesReceived)
{
    if(err) {
        std::cerr << "oculus::StatusListener::read_callback : Status reception error.\n";
        this->get_one_message();
        return;
    }

    if(bytesReceived != sizeof(OculusStatusMsg)) {
        std::cerr << "oculus::StatusListener::read_callback : not enough bytes.\n";
        this->get_one_message();
        return;
    }
    
    // we are clean here
    clock_.reset();
    //std::cout << msg_ << std::endl;
    callbacks_.call(msg_);
    this->get_one_message();
}

} //namespace oculus
