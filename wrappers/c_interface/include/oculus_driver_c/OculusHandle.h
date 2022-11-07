#ifndef _OCULUS_DRIVER_C_OCULUS_HANDLE_H_
#define _OCULUS_DRIVER_C_OCULUS_HANDLE_H_

#include <vector>

#include <oculus_driver/Oculus.h>
#include <oculus_driver/AsyncService.h>
#include <oculus_driver/SonarDriver.h>

namespace oculus 
{

struct OculusHandle
{
    using MessageCallback = void(const OculusMessageHeader&,
                                 const std::vector<uint8_t>& data);
    using PingCallback    = void(const OculusSimplePingResult&,
                                 const std::vector<uint8_t>& data);
    using StatusCallback  = void(const OculusStatusMsg&);

    oculus::AsyncService ioService_;
    oculus::SonarDriver  sonar_;

    OculusHandle() :
        sonar_(ioService_.io_service())
    {}

    void start() { ioService_.start(); }
    void stop()  { ioService_.stop();  }

    void add_message_callback(const std::function<MessageCallback>& callback) {
        sonar_.add_message_callback(callback);
    }
    void add_ping_callback(const std::function<PingCallback>& callback) {
        sonar_.add_ping_callback(callback);
    }
    void add_status_callback(const std::function<StatusCallback>& callback) {
        sonar_.add_status_callback(callback);
    }
};

} //namespace oculus

#endif //_OCULUS_DRIVER_C_OCULUS_HANDLE_H_
