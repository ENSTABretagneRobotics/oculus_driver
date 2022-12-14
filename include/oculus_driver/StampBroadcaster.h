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

#ifndef _DEF_OCULUS_DRIVER_STAMP_BROADCASTER_H_
#define _DEF_OCULUS_DRIVER_STAMP_BROADCASTER_H_

#include <oculus_driver/SonarDriver.h>

namespace oculus {

class StampBroadcaster
{
    public:

    using Socket   = boost::asio::ip::udp::socket;
    using EndPoint = boost::asio::ip::udp::endpoint;

    protected:

    mutable Socket socket_;
    EndPoint       broadcastEndpoint_;

    public:

    StampBroadcaster(const SonarDriver& sonar, uint16_t port);
    ~StampBroadcaster();

    bool is_open() const { return socket_.is_open(); }

    void send(const Message::ConstPtr& msg) const;
};

} //namespace oculus


#endif //_DEF_OCULUS_DRIVER_STAMP_BROADCASTER_H_
