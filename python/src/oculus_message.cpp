#include <pybind11/pybind11.h>
#include <pybind11/chrono.h>
namespace py = pybind11;

#include <iostream>
#include <sstream>
#include <chrono>

#include <oculus_driver/Oculus.h>
#include <oculus_driver/OculusMessage.h>
#include <oculus_driver/print_utils.h>

#include "message_utils.h"

void init_oculus_message(py::module& m_)
{
    py::class_<oculus::Message, oculus::Message::Ptr>(m_, "OculusMessage")
        .def(py::init<>())
        .def("header", &oculus::Message::header)
        .def("timestamp", &oculus::Message::timestamp)
        .def("data",   [](const oculus::Message::ConstPtr& msg) {
            return make_memory_view(msg->data());
         })
        .def("__str__", [](const oculus::Message::ConstPtr& msg) {
            std::ostringstream oss;
            oss << "OculusMessage :\n" << msg->header();
            return oss.str();
        })
        .def("timestamp_micros", [](const oculus::Message::ConstPtr& msg) {
            return std::chrono::duration_cast<std::chrono::microseconds>(
                msg->timestamp().time_since_epoch()).count();
        });

    py::class_<oculus::PingMessage, oculus::PingMessage::Ptr>(m_, "PingMessage")
        .def(py::init<oculus::Message::ConstPtr>())
        .def("message",   &oculus::PingMessage::message)
        .def("timestamp", &oculus::PingMessage::timestamp)
        .def("data",          [](const oculus::PingMessage::ConstPtr& msg) {
            return make_memory_view(msg->data());
        })
        .def("metadata", [](const oculus::PingMessage::ConstPtr& msg) {
            if(msg->message()->message_version() == 2) {
                return py::cast(*reinterpret_cast<const OculusSimplePingResult2*>(msg->data().data()));
            }
            else {
                return py::cast(*reinterpret_cast<const OculusSimplePingResult*>(msg->data().data()));
            }
        })

        .def("range_count",   &oculus::PingMessage::range_count)
        .def("bearing_count", &oculus::PingMessage::bearing_count)
        .def("has_gains",     &oculus::PingMessage::has_gains)
        .def("master_mode",   &oculus::PingMessage::master_mode)
        .def("sample_size",   &oculus::PingMessage::sample_size)

        .def("bearing_data",  [](const oculus::PingMessage::ConstPtr& msg) {
            return make_memory_view(msg->bearing_count(), msg->bearing_data());
        })
        .def("raw_ping_data", [](const oculus::PingMessage::ConstPtr& msg) {
            return make_raw_ping_data_view(*msg);
        })
        .def("gains", [](const oculus::PingMessage::ConstPtr& msg) {
            return make_gains_view(*msg);
        })
        .def("ping_data", [](const oculus::PingMessage::ConstPtr& msg) {
            return make_ping_data_view(*msg);
        })
        .def("ping_index",          &oculus::PingMessage::ping_index)
        //.def("ping_firing_date",    &oculus::PingMessage::ping_firing_date) // broken on hardware side ?
        .def("range",               &oculus::PingMessage::range)
        .def("gain_percent",        &oculus::PingMessage::gain_percent)
        .def("frequency",           &oculus::PingMessage::frequency)
        .def("speed_of_sound_used", &oculus::PingMessage::speed_of_sound_used)
        .def("range_resolution",    &oculus::PingMessage::range_resolution)
        .def("temperature",         &oculus::PingMessage::temperature)
        .def("pressure",            &oculus::PingMessage::pressure)
        .def("timestamp_micros", [](const oculus::PingMessage::ConstPtr& msg) {
            return std::chrono::duration_cast<std::chrono::microseconds>(
                msg->timestamp().time_since_epoch()).count();
        });

        //m_.def("ping_message_from_bytes", [](py::bytes data) {
        m_.def("ping_message_from_bytes", [](py::bytes data,
                const oculus::Message::TimePoint& stamp)
        {
            auto view = (std::string_view)data;
            return oculus::PingMessage::Create(view.size(),
                                               (const uint8_t*)view.data());
        }, py::arg("data"), py::arg("stamp") = oculus::Message::TimePoint());
}




