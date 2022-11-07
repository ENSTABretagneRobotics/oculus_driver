#include <oculus_driver_c/oculus_driver.h>

#include <oculus_driver/AsyncService.h>
#include <oculus_driver/SonarDriver.h>

#include <functional>

struct oculus_handle {
    void* handle;
};

struct OculusHandle
{
    oculus::AsyncService ioService_;
    oculus::SonarDriver  sonar_;

    OculusHandle() :
        sonar_(ioService_.io_service())
    {}
};

OculusHandle* get(oculus_handle_t* handle)
{
    return static_cast<OculusHandle*>(handle->handle);
}

oculus_handle_t* oculus_handle_create()
{
    oculus_handle_t* handle;

    handle = (oculus_handle_t*)malloc(sizeof(oculus_handle_t));
    handle->handle = new OculusHandle();

    return handle;
}

void oculus_handle_destroy(oculus_handle_t* handle)
{
    if(handle == NULL) {
        return;
    }

    delete get(handle);
    free(handle);
}

void oculus_start(oculus_handle_t* handle)
{
    get(handle)->ioService_.start();
}

void oculus_stop(oculus_handle_t* handle)
{
    get(handle)->ioService_.stop();
}

void message_callback_wrapper(void (*callback)(OculusMessageHeader, uint64_t, const uint8_t*),
                              const OculusMessageHeader& header,
                              const std::vector<uint8_t>& data)
{
    callback(header, data.size(), data.data());
}

void oculus_add_message_callback(oculus_handle_t* handle,
    void (*callback)(OculusMessageHeader, uint64_t, const uint8_t*))
{
    get(handle)->sonar_.add_message_callback(std::bind(message_callback_wrapper,
                                                       callback, 
                                                       std::placeholders::_1,
                                                       std::placeholders::_2));
}

void ping_callback_wrapper(void (*callback)(OculusSimplePingResult, size_t, const uint8_t*),
                           const OculusSimplePingResult& metadata,
                           const std::vector<uint8_t>& data)
{
    callback(metadata, data.size(), data.data());
}

void oculus_add_ping_callback(oculus_handle_t* handle,
    void (*callback)(OculusSimplePingResult, size_t, const uint8_t*))
{
    get(handle)->sonar_.add_ping_callback(std::bind(ping_callback_wrapper,
                                                    callback, 
                                                    std::placeholders::_1,
                                                    std::placeholders::_2));
}


