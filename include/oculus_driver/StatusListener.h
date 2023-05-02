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

#ifndef _DEF_OCULUS_DRIVER_STATUS_LISTENER_H_
#define _DEF_OCULUS_DRIVER_STATUS_LISTENER_H_

#include <iostream>

#include <boost/asio.hpp>

#include <oculus_driver/Oculus.h>
#include <oculus_driver/CallbackQueue.h>
#include <oculus_driver/Clock.h>

namespace oculus {

class StatusListener
{
    public:

    using IoService    = boost::asio::io_service;
    using IoServicePtr = std::shared_ptr<IoService>;
    using Socket       = boost::asio::ip::udp::socket;
    using EndPoint     = boost::asio::ip::udp::endpoint;
    using Callbacks    = CallbackQueue<const OculusStatusMsg&>;
    using CallbackT    = Callbacks::CallbackT;
    using CallbackId   = Callbacks::CallbackId;

    protected:

    Socket          socket_;
    EndPoint        remote_;
    OculusStatusMsg msg_;
    Callbacks       callbacks_;
    Clock           clock_;

    public:

    StatusListener(const IoServicePtr& service, unsigned short listeningPort = 52102);
    
    unsigned int add_callback(const std::function<void(const OculusStatusMsg&)>& callback);
    bool remove_callback(unsigned int index);
    bool on_next_status(const std::function<void(const OculusStatusMsg&)>& callback);

    template <typename T = float>
    T time_since_last_status() const { return clock_.now<T>(); }
    
    private:
    
    void get_one_message();
    void message_callback(const boost::system::error_code& err, std::size_t bytesReceived);
};

} //namespace oculus

#endif //_DEF_OCULUS_DRIVER_STATUS_LISTENER_H_
