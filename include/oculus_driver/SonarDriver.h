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

#ifndef _DEF_OCULUS_DRIVER_SONAR_DRIVER_H_
#define _DEF_OCULUS_DRIVER_SONAR_DRIVER_H_

#include <oculus_driver/Oculus.h>
#include <oculus_driver/utils.h>
#include <oculus_driver/print_utils.h>
#include <oculus_driver/CallbackQueue.h>
#include <oculus_driver/SonarClient.h>

namespace oculus {

class SonarDriver : public SonarClient
{
    public:

    using Duration     = SonarClient::Duration;

    using IoService    = boost::asio::io_service;
    using IoServicePtr = std::shared_ptr<IoService>;

    using PingConfig    = OculusSimpleFireMessage;
    using PingResult    = OculusSimplePingResult;

    using MessageCallback = std::function<void(const Message::ConstPtr&)>;
    using StatusCallback  = std::function<void(const OculusStatusMsg&)>;
    using PingCallback    = std::function<void(const PingMessage::ConstPtr)>;
    using DummyCallback   = std::function<void(const OculusMessageHeader&)>;
    using ConfigCallback  = std::function<void(const PingConfig&, const PingConfig&)>;

    using TimeSource = SonarClient::TimeSource;
    using TimePoint  = typename std::invoke_result<decltype(&TimeSource::now)>::type;

    protected:

    PingConfig lastConfig_;
    uint8_t    lastPingRate_;

    // message callbacks will be called on every received message.
    // config callbacks will be called on (detectable) configuration changes.
    CallbackQueue<const Message::ConstPtr&>             messageCallbacks_;
    CallbackQueue<const PingMessage::ConstPtr>          pingCallbacks_;
    CallbackQueue<const OculusMessageHeader&>           dummyCallbacks_;
    CallbackQueue<const PingConfig&, const PingConfig&> configCallbacks_;

    public:

    SonarDriver(const IoServicePtr& service,
                const Duration& checkerPeriod = boost::posix_time::seconds(1));

    bool send_ping_config(PingConfig config);
    PingConfig current_ping_config();
    PingConfig request_ping_config(PingConfig request);
    PingConfig last_ping_config() const;

    // Stanby mode (saves current ping rate and set it to 0 on the sonar
    void standby();
    void resume();
    
    virtual void on_connect();
    virtual void handle_message(const Message::ConstPtr& message);

    /////////////////////////////////////////////
    // All remaining member function are related to callbacks and are merely
    // helpers to add callbacks.

    unsigned int add_message_callback(const MessageCallback& callback);
    unsigned int add_status_callback (const StatusCallback&  callback);
    unsigned int add_ping_callback   (const PingCallback&    callback);
    unsigned int add_dummy_callback  (const DummyCallback&   callback);
    unsigned int add_config_callback (const ConfigCallback&  callback);

    bool remove_message_callback(unsigned int callbackId);
    bool remove_status_callback (unsigned int callbackId);
    bool remove_ping_callback   (unsigned int callbackId);
    bool remove_dummy_callback  (unsigned int callbackId);

    // these are synchronous function which will wait for the next message
    bool wait_next_message();
    bool on_next_message(const MessageCallback& callback);
    bool on_next_status (const StatusCallback&  callback);
    bool on_next_ping   (const PingCallback&    callback);
    bool on_next_dummy  (const DummyCallback&   callback);
};

} //namespace oculus

#endif //_DEF_OCULUS_DRIVER_SONAR_DRIVER_H_


