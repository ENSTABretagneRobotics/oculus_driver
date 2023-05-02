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

#include <oculus_driver/SonarDriver.h>

namespace oculus {

SonarDriver::SonarDriver(const IoServicePtr& service,
                         const Duration& checkerPeriod) :
    SonarClient(service, checkerPeriod),
    lastConfig_(default_ping_config()),
    lastPingRate_(pingRateNormal)
{}

bool SonarDriver::send_ping_config(PingConfig config)
{
    config.head.oculusId    = OCULUS_CHECK_ID;
    config.head.msgId       = messageSimpleFire;
    config.head.srcDeviceId = 0;
    config.head.dstDeviceId = sonarId_;
    config.head.payloadSize = sizeof(PingConfig) - sizeof(OculusMessageHeader);

    // Other non runtime-configurable parameters (TODO : make then launch parameters)
    config.networkSpeed = 0xff;

    boost::asio::streambuf buf;
    buf.sputn(reinterpret_cast<const char*>(&config), sizeof(config));
    
    auto bytesSent = this->send(buf);
    if(bytesSent != sizeof(config)) {
        std::cerr << "Could not send whole fire message(" << bytesSent
                  << "/" << sizeof(config) << ")" << std::endl;
        return false;
    }

    // BUG IN THE SONAR FIRMWARE : the sonar never sets the
    // config.pingRate field in the SimplePing message -> there is no
    // feedback saying if this parameter is effectively set by the sonar. The
    // line below allows to keep a trace of the requested ping rate but there
    // is no clean way to check.
    lastConfig_.pingRate = config.pingRate;
    
    // Also saving the last pingRate which is not standby to be able to resume
    // the sonar to the last ping rate in the resume() method.
    if(lastConfig_.pingRate != pingRateStandby) {
        lastPingRate_ = lastConfig_.pingRate;
    }
    return true;
}

SonarDriver::PingConfig SonarDriver::last_ping_config() const
{
    return lastConfig_;
}

SonarDriver::PingConfig SonarDriver::current_ping_config()
{
    using Timeout = CallbackQueue<const Message::ConstPtr&>::TimeoutReached;

    PingConfig config;

    auto configSetter = [&](const Message::ConstPtr& message) {
        // lastConfig_ is ALWAYS updated before the callbacks are called.
        // We only need to wait for the next message to get the current ping
        // configuration.
        config = lastConfig_;
        config.head = message->header();
    };
    if(!this->on_next_message(configSetter)) {
        throw Timeout();
    }
    return config;
}

SonarDriver::PingConfig SonarDriver::request_ping_config(PingConfig request)
{
    request.flags |= 0x4; // forcing sonar sending gains to true
    using Timeout = CallbackQueue<const Message::ConstPtr&>::TimeoutReached;

    // Waiting for a ping or a dummy message to have a feedback on the config changes.
    PingConfig feedback;
    int count = 0;
    const int maxCount = 100; // TODO make a parameter out of this
    do {
        if(this->send_ping_config(request)) {
            try {
                feedback = this->current_ping_config();
                if(check_config_feedback(request, feedback))
                    break;
            }
            catch(const Timeout& e) {
                std::cerr << "Timeout reached while requesting config" << std::endl;
                continue;
            }
        }
        count++;
    } while(count < maxCount);
    //std::cout << "Count is : " << count << std::endl << std::flush;

    if(count >= maxCount) {
        std::cerr << "Could not get a proper feedback from the sonar."
                  << "Assuming the configuration is ok (fix this)" << std::endl;
        feedback = request;
        feedback.head.msgId = 0; // invalid, will be checkable.
    }
    
    return feedback;
}

void SonarDriver::standby()
{
    auto request = lastConfig_;

    request.pingRate = pingRateStandby;
    
    this->send_ping_config(request);
}

void SonarDriver::resume()
{
    auto request = lastConfig_;

    request.pingRate = lastPingRate_;
    
    this->send_ping_config(request);
}

/**
 * Called when the driver first connection with the sonar.
 *
 * Will be called again if a new connection append.
 */
void SonarDriver::on_connect()
{
    // This makes the oculus fire right away.
    // On first connection lastConfig_ is equal to default_ping_config().
    this->send_ping_config(lastConfig_);
}

/**
 * Called when a new complete message is received (any type).
 */
void SonarDriver::handle_message(const Message::ConstPtr& message)
{
    const auto& header = message->header();
    const auto& data   = message->data();
    OculusSimpleFireMessage newConfig = lastConfig_;
    switch(header.msgId) {
        case messageSimplePingResult:
            newConfig = reinterpret_cast<const OculusSimplePingResult*>(data.data())->fireMessage;
            newConfig.pingRate = lastConfig_.pingRate; // feedback is broken on pingRate
            // When masterMode = 2, the sonar force gainPercent between 40& and
            // 100%, BUT still needs resquested gainPercent to be between 0%
            // and 100%. (If you request a gainPercent=0 in masterMode=2, the
            // fireMessage in the ping results will be 40%). The gainPercent is
            // rescaled here to ensure consistent parameter handling on client
            // side).
            if(newConfig.masterMode == 2) {
                newConfig.gainPercent = (newConfig.gainPercent - 40.0) * 100.0 / 60.0;
            }
            break;
        case messageDummy:
            newConfig.pingRate = pingRateStandby;
            break;
        default:
            break;
    };

    if(config_changed(lastConfig_, newConfig)) {
        configCallbacks_.call(lastConfig_, newConfig);
    }
    lastConfig_ = newConfig;

    // Calling generic message callbacks first (in case we want to do something
    // before calling the specialized callbacks).
    messageCallbacks_.call(message);
    switch(header.msgId) {
        case messageSimplePingResult:
            pingCallbacks_.call(PingMessage::Create(message));
            break;
        case messageDummy:
            dummyCallbacks_.call(header);
            break;
        case messageSimpleFire:
            std::cerr << "messageSimpleFire parsing not implemented." << std::endl;
            break;
        case messagePingResult:
            std::cerr << "messagePingResult parsing not implemented." << std::endl;
            break;
        case messageUserConfig:
            std::cerr << "messageUserConfig parsing not implemented." << std::endl;
            break;
        default:
            break;
    }
}

// message callbacks
unsigned int SonarDriver::add_message_callback(const MessageCallback& callback)
{
    return messageCallbacks_.add_callback(callback);
}

bool SonarDriver::remove_message_callback(unsigned int callbackId)
{
    return messageCallbacks_.remove_callback(callbackId);
}

bool SonarDriver::on_next_message(const MessageCallback& callback)
{
    return messageCallbacks_.add_single_shot(callback);
}

// status callbacks
unsigned int SonarDriver::add_status_callback(const StatusCallback& callback)
{
    return statusListener_.add_callback(callback);
}

bool SonarDriver::remove_status_callback(unsigned int callbackId)
{
    return statusListener_.remove_callback(callbackId);
}

bool SonarDriver::on_next_status(const StatusCallback& callback)
{
    return statusListener_.on_next_status(callback);
}

// ping callbacks
unsigned int SonarDriver::add_ping_callback(const PingCallback& callback)
{
    return pingCallbacks_.add_callback(callback);
}

bool SonarDriver::remove_ping_callback(unsigned int callbackId)
{
    return pingCallbacks_.remove_callback(callbackId);
}

bool SonarDriver::on_next_ping(const PingCallback& callback)
{
    return pingCallbacks_.add_single_shot(callback);
}

// dummy callbacks
unsigned int SonarDriver::add_dummy_callback(const DummyCallback& callback)
{
    return dummyCallbacks_.add_callback(callback);
}

bool SonarDriver::remove_dummy_callback(unsigned int callbackId)
{
    return dummyCallbacks_.remove_callback(callbackId);
}

bool SonarDriver::on_next_dummy(const DummyCallback& callback)
{
    return dummyCallbacks_.add_single_shot(callback);
}

/**
 * This is a synchronization primitive allowing for waiting for the sonar to be
 * ready for example.
 */
bool SonarDriver::wait_next_message()
{
    auto dummy = [](const Message::ConstPtr&) {};
    return this->on_next_message(dummy);
}

unsigned int SonarDriver::add_config_callback(const ConfigCallback& callback)
{
    return configCallbacks_.add_callback(callback);
}

} //namespace oculus

