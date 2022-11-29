#include <pybind11/pybind11.h>
namespace py = pybind11;

#include <iostream>
#include <sstream>

#include <oculus_driver/Oculus.h>
#include <oculus_driver/OculusMessage.h>
#include <oculus_driver/print_utils.h>

#include "message_utils.h"

void init_oculus_message(py::module& m_)
{
    py::class_<oculus::Message, oculus::Message::Ptr>(m_, "OculusMessage")
        .def(py::init<>())
        .def("header", &oculus::Message::header)
        .def("data",   [](const oculus::Message::ConstPtr& msg) {
            return make_memory_view(msg->data());
         })
        .def("__str__", [](const oculus::Message::ConstPtr& msg) {
            std::ostringstream oss;
            oss << "OculusMessage :\n" << msg->header();
            return oss.str();
        });

    py::class_<oculus::PingMessage, oculus::PingMessage::Ptr>(m_, "PingMessage")
        .def(py::init<oculus::Message::ConstPtr>())
        .def("message", &oculus::PingMessage::message)
        .def("data",          [](const oculus::PingMessage::ConstPtr& msg) {
            return make_memory_view(msg->data());
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
        });
}




