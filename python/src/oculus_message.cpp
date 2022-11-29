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

    py::class_<PyPingMessage>(m_, "PingMessage")
        .def(py::init<const oculus::PingMessage::ConstPtr&>())
        .def("header",        &PyPingMessage::header)
        .def("data",          [](const PyPingMessage& msg) {
            return make_memory_view(msg.data());
        })
        .def("range_count",   &PyPingMessage::range_count)
        .def("bearing_count", &PyPingMessage::bearing_count)
        .def("bearing_data",  [](const PyPingMessage& msg) {
            return make_memory_view(msg.bearing_count(), msg.bearing_data());
        })
        .def("raw_ping_data", [](const PyPingMessage& msg) {
            return make_raw_ping_data_view(msg);
        })
        .def("gains", [](const PyPingMessage& msg) {
            return make_gains_view(msg);
        })
        .def("ping_data", [](const PyPingMessage& msg) {
            return make_ping_data_view(msg);
        });
}




