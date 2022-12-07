#ifndef _DEF_OCULUS_DRIVER_PYBIND11_UTILS_H_
#define _DEF_OCULUS_DRIVER_PYBIND11_UTILS_H_

#include <pybind11/pybind11.h>
namespace py = pybind11;

#include <vector>

#include <oculus_driver/OculusMessage.h>

#include "oculus_message.h"

template <typename T>
inline py::memoryview make_memory_view(std::size_t size, const T* data)
{
    return py::memoryview::from_buffer(const_cast<T*>(data), {size}, {sizeof(T)});
}

template <typename T>
inline py::memoryview make_memory_view(std::size_t width, std::size_t height, const T* data)
{
    return py::memoryview::from_buffer(const_cast<T*>(data), {height, width}, {sizeof(T)*width, sizeof(T)});
}

template <typename T>
inline py::memoryview make_memory_view(const std::vector<T>& data)
{
    return make_memory_view(data.size(), data.data());
}

inline py::memoryview make_memory_view(const oculus::Message& msg)
{
    return make_memory_view(msg.data());
}

inline py::memoryview make_raw_ping_data_view(const oculus::PingMessage& msg)
{
    auto width = msg.bearing_count();
    switch(msg.sample_size()) {
        case 1:
            if(msg.has_gains()) width += 4;
            return make_memory_view(width, msg.range_count(), msg.ping_data());
            break;
        case 2:
            if(msg.has_gains()) width += 2;
            return make_memory_view(width, msg.range_count(), (const uint16_t*)msg.ping_data());
            break;
        case 4:
            if(msg.has_gains()) width += 1;
            return make_memory_view(width, msg.range_count(), (const uint32_t*)msg.ping_data());
            break;
        default:
            std::cerr << "Unhandled sample_size ("
                      << 3*msg.sample_size() << "bits)." << std::endl;
            return py::none();
            break;
    }
}

inline py::memoryview make_gains_view(const oculus::PingMessage& msg)
{
    if(!msg.has_gains()) {
        return py::none();
    }

    auto step = msg.bearing_count()*msg.sample_size() + 4;
    return py::memoryview::from_buffer((uint32_t*)const_cast<uint8_t*>(msg.ping_data()),
                                       {msg.range_count()}, {step});
}

inline py::memoryview make_ping_data_view(const oculus::PingMessage& msg)
{
    std::size_t step   = msg.sample_size()*msg.bearing_count();
    std::size_t offset = 0;
    if(msg.has_gains()) {
        step   += 4;
        offset  = 4;
    }
    switch(msg.sample_size()) {
        case 1:
            return py::memoryview::from_buffer(const_cast<uint8_t*>(msg.ping_data() + offset),
                                  {msg.range_count(), msg.bearing_count()},
                                  {step, sizeof(uint8_t)});
            break;
        case 2:
            return py::memoryview::from_buffer((uint16_t*)const_cast<uint8_t*>(msg.ping_data() + offset),
                                  {msg.range_count(), msg.bearing_count()},
                                  {step, sizeof(uint16_t)});
            break;
        case 4:
            return py::memoryview::from_buffer((uint32_t*)const_cast<uint8_t*>(msg.ping_data() + offset),
                                  {msg.range_count(), msg.bearing_count()},
                                  {step, sizeof(uint32_t)});
            break;
        default:
            std::cerr << "Unhandled sample_size ("
                      << 3*msg.sample_size() << "bits)." << std::endl;
            return py::none();
            break;
    }
}

#endif //_DEF_OCULUS_DRIVER_PYBIND11_UTILS_H_





