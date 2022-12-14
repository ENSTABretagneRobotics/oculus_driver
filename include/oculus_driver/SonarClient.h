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

#ifndef _OCULUS_DRIVER_SONAR_CLIENT_H_
#define _OCULUS_DRIVER_SONAR_CLIENT_H_

#include <iostream>
#include <iomanip>
#include <cstring>
#include <cmath>
#include <memory>
#include <mutex>
#include <thread>
#include <chrono>
#include <type_traits>

#include <boost/asio.hpp>

#include <oculus_driver/Oculus.h>
#include <oculus_driver/utils.h>
#include <oculus_driver/print_utils.h>
#include <oculus_driver/StatusListener.h>

#include <oculus_driver/OculusMessage.h>

namespace oculus {

/**
 * This is the base class to handle an Oculus sonar.
 *
 * It solely handle the network connection to the sonar. Use a subclass such as
 * SonarDriver to control the sonar or receive data.
 *
 * About concurrency on the socket : socket creation, destruction and read all
 * append in the same thread. There is no need to protect the socket for
 * concurrency between these operations. Also, boost sockets allows to be
 * concurrently read and written to at the same time. The situation where a
 * protection is needed is concurrent write and creation/destruction on the
 * socket. Hence, the socket is only locked in the send(), close_connection()
 * and ???. 
 */
class SonarClient
{
    public:

    using IoService    = boost::asio::io_service;
    using IoServicePtr = std::shared_ptr<IoService>;
    using Socket       = boost::asio::ip::tcp::socket;
    using SocketPtr    = std::unique_ptr<Socket>;
    using EndPoint     = boost::asio::ip::tcp::endpoint;
    using Duration     = boost::posix_time::time_duration;

    enum ConnectionState { Initializing, Attempt, Connected, Lost };

    using TimeSource = Message::TimeSource;
    using TimePoint  = Message::TimePoint;

    protected:
    
    IoServicePtr       ioService_;
    SocketPtr          socket_;
    EndPoint           remote_;
    uint16_t           sonarId_;
    ConnectionState    connectionState_;
    mutable std::mutex socketMutex_;

    Duration                     checkerPeriod_;
    boost::asio::deadline_timer  checkerTimer_;
    Clock                        clock_;
    
    StatusListener statusListener_;
    unsigned int   statusCallbackId_;

    Message::Ptr message_;

    // helper stubs
    void checker_callback(const boost::system::error_code& err);
    void check_reception(const boost::system::error_code& err);

    public:

    SonarClient(const IoServicePtr& ioService,
                const Duration& checkerPeriod = boost::posix_time::seconds(1));

    IoServicePtr service() const { return ioService_; }

    bool is_valid(const OculusMessageHeader& header);
    bool connected() const;

    size_t send(const boost::asio::streambuf& buffer) const;

    // initialization states
    void reset_connection();
    void close_connection();
    void on_first_status(const OculusStatusMsg& msg);
    void connect_callback(const boost::system::error_code& err);
    virtual void on_connect();

    // main loop begin
    void initiate_receive();
    void header_received_callback(const boost::system::error_code err,
                                  std::size_t receivedByteCount);
    void data_received_callback(const boost::system::error_code err,
                                std::size_t receivedByteCount);
    
    // This is called regardless of the content of the message.
    // To be reimplemented in a subclass (does nothing by default).
    virtual void handle_message(const Message::ConstPtr& msg);

    template <typename TimeT = float>
    TimeT time_since_last_message() const { return clock_.now<TimeT>(); }

    TimePoint last_header_stamp() const { return message_->timestamp(); }
};

} //namespace oculus

#endif //_OCULUS_DRIVER_SONAR_CLIENT_H_
