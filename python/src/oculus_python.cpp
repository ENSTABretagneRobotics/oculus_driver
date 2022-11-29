#include <pybind11/pybind11.h>
namespace py = pybind11;

#include <oculus_driver/Oculus.h>
#include <oculus_driver/OculusMessage.h>
#include <oculus_driver/AsyncService.h>
#include <oculus_driver/SonarDriver.h>
#include <oculus_driver/Recorder.h>

#include "oculus_message.h"
#include "oculus_files.h"

inline void message_callback_wrapper(py::object callback, const oculus::Message::ConstPtr& msg)
{
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();
    
    callback(py::cast(msg));

    PyGILState_Release(gstate);
}

inline void ping_callback_wrapper(py::object callback, 
                                  const OculusSimplePingResult& metadata,
                                  const std::vector<uint8_t>& data)
{
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();
    
    callback(py::cast(&metadata),
             py::memoryview(py::buffer_info(const_cast<uint8_t*>(data.data()),
                                             sizeof(uint8_t),
                                             py::format_descriptor<uint8_t>::format(),
                                             1, {data.size()}, {1})));
    
    PyGILState_Release(gstate);
}

inline void status_callback_wrapper(py::object callback, 
                                    const OculusStatusMsg& status)
{
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();
    
    callback(py::cast(&status));
    
    PyGILState_Release(gstate);
}

inline void config_callback_wrapper(py::object callback, 
                                    const OculusSimpleFireMessage& lastConfig,
                                    const OculusSimpleFireMessage& newConfig)
{
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();
    
    callback(py::cast(&lastConfig), py::cast(&newConfig));
    
    PyGILState_Release(gstate);
}

struct OculusPythonHandle
{
    oculus::AsyncService service_;
    oculus::SonarDriver  sonar_;
    oculus::Recorder     recorder_;
    int recorderCallbackId_;

    OculusPythonHandle() :
        sonar_(service_.io_service()),
        recorderCallbackId_(-1)
    {}
    ~OculusPythonHandle() { this->stop(); }

    void start() { service_.start(); }
    void stop()  { service_.stop();  }

    bool send_config(py::object obj) {
        return sonar_.send_ping_config(*obj.cast<const OculusSimpleFireMessage*>());
    }

    py::object current_config() const {
        return py::cast(sonar_.last_ping_config());
    }

    py::object request_config(py::object obj) {
        return py::cast(sonar_.request_ping_config(*obj.cast<const OculusSimpleFireMessage*>()));
    }

    void add_message_callback(py::object obj) {
        sonar_.add_message_callback(std::bind(message_callback_wrapper, obj,
                                              std::placeholders::_1));
    }
    void add_ping_callback(py::object obj) {
        sonar_.add_ping_callback(std::bind(ping_callback_wrapper, obj,
                                           std::placeholders::_1, 
                                           std::placeholders::_2));
    }
    void add_status_callback(py::object obj) {
        sonar_.add_status_callback(std::bind(status_callback_wrapper, obj,
                                             std::placeholders::_1));
    }
    void add_config_callback(py::object obj) {
        sonar_.add_config_callback(std::bind(config_callback_wrapper, obj,
                                             std::placeholders::_1,
                                             std::placeholders::_2));
    }

    void recorder_start(const std::string& filename, bool overwrite) {
        if(recorder_.is_open()) {
            return;
        }
        recorder_.open(filename, overwrite);
        recorderCallbackId_ = sonar_.add_message_callback(
            std::bind(&OculusPythonHandle::recorder_callback, this,
                      std::placeholders::_1));
    }
    void recorder_stop() {
        recorder_.close();
        if(recorderCallbackId_ > 0) {
            sonar_.remove_message_callback(recorderCallbackId_);
        }
    }
    void recorder_callback(const oculus::Message::ConstPtr& msg) const {
        recorder_.write(msg);
    }
    bool is_recording() const {
        return recorder_.is_open();
    }
};

PYBIND11_MODULE(oculus_python, m_)
{
    py::class_<OculusMessageHeader>(m_, "OculusMessageHeader")
        .def(py::init<>())
        .def_readwrite("oculusId",    &OculusMessageHeader::oculusId)
        .def_readwrite("srcDeviceId", &OculusMessageHeader::srcDeviceId)
        .def_readwrite("dstDeviceId", &OculusMessageHeader::dstDeviceId)
        .def_readwrite("msgId",       &OculusMessageHeader::msgId)
        .def_readwrite("msgVersion",  &OculusMessageHeader::msgVersion)
        .def_readwrite("payloadSize", &OculusMessageHeader::payloadSize)
        .def_readwrite("spare2",      &OculusMessageHeader::spare2)
        .def("__str__", [](const OculusMessageHeader& header) {
            std::ostringstream oss;
            oss << header;
            return oss.str();
        });

    py::class_<OculusSimpleFireMessage>(m_, "OculusSimpleFireMessage")
        .def(py::init<>())
        .def_readwrite("head",            &OculusSimpleFireMessage::head)
        .def_readwrite("masterMode",      &OculusSimpleFireMessage::masterMode)
        .def_readwrite("pingRate",        &OculusSimpleFireMessage::pingRate)
        .def_readwrite("networkSpeed",    &OculusSimpleFireMessage::networkSpeed)
        .def_readwrite("gammaCorrection", &OculusSimpleFireMessage::gammaCorrection)
        .def_readwrite("flags",           &OculusSimpleFireMessage::flags)
        .def_readwrite("range",           &OculusSimpleFireMessage::range)
        .def_readwrite("gainPercent",     &OculusSimpleFireMessage::gainPercent)
        .def_readwrite("speedOfSound",    &OculusSimpleFireMessage::speedOfSound)
        .def_readwrite("salinity",        &OculusSimpleFireMessage::salinity)
        .def("__str__", [](const OculusSimpleFireMessage& msg) {
            std::ostringstream oss;
            oss << msg;
            return oss.str();
        });

    py::class_<OculusSimplePingResult>(m_, "OculusSimplePingResult")
        .def(py::init<>())
        .def_readonly("fireMessage",       &OculusSimplePingResult::fireMessage)
        .def_readonly("pingId",            &OculusSimplePingResult::pingId)
        .def_readonly("status",            &OculusSimplePingResult::status)
        .def_readonly("frequency",         &OculusSimplePingResult::frequency)
        .def_readonly("temperature",       &OculusSimplePingResult::temperature)
        .def_readonly("pressure",          &OculusSimplePingResult::pressure)
        .def_readonly("speeedOfSoundUsed", &OculusSimplePingResult::speeedOfSoundUsed)
        .def_readonly("pingStartTime",     &OculusSimplePingResult::pingStartTime)
        .def_readonly("dataSize",          &OculusSimplePingResult::dataSize)
        .def_readonly("rangeResolution",   &OculusSimplePingResult::rangeResolution)
        .def_readonly("nRanges",           &OculusSimplePingResult::nRanges)
        .def_readonly("nBeams",            &OculusSimplePingResult::nBeams)
        .def_readonly("imageOffset",       &OculusSimplePingResult::imageOffset)
        .def_readonly("imageSize",         &OculusSimplePingResult::imageSize)
        .def_readonly("messageSize",       &OculusSimplePingResult::messageSize)
        .def("__str__", [](const OculusSimplePingResult& msg) {
            std::ostringstream oss;
            oss << msg;
            return oss.str();
        });

    py::class_<OculusVersionInfo>(m_, "OculusVersionInfo")
        .def(py::init<>())
        .def_readonly("firmwareVersion0", &OculusVersionInfo::firmwareVersion0)
        .def_readonly("firmwareDate0",    &OculusVersionInfo::firmwareDate0)
        .def_readonly("firmwareVersion1", &OculusVersionInfo::firmwareVersion1)
        .def_readonly("firmwareDate1",    &OculusVersionInfo::firmwareDate1)
        .def_readonly("firmwareVersion2", &OculusVersionInfo::firmwareVersion2)
        .def_readonly("firmwareDate2",    &OculusVersionInfo::firmwareDate2);
        //.def("__str__", [](const OculusVersionInfo& version) {
        //    std::ostringstream oss;
        //    oss << version;
        //    return oss.str();
        //});

    py::class_<OculusStatusMsg>(m_, "OculusStatusMsg")
        .def(py::init<>())
        .def_readonly("hdr",             &OculusStatusMsg::hdr)
        .def_readonly("deviceId",        &OculusStatusMsg::deviceId)
        .def_readonly("deviceType",      &OculusStatusMsg::deviceType)
        .def_readonly("partNumber",      &OculusStatusMsg::partNumber)
        .def_readonly("status",          &OculusStatusMsg::status)
        .def_readonly("versinInfo",      &OculusStatusMsg::versinInfo)
        .def_readonly("ipAddr",          &OculusStatusMsg::ipAddr)
        .def_readonly("ipMask",          &OculusStatusMsg::ipMask)
        .def_readonly("connectedIpAddr", &OculusStatusMsg::connectedIpAddr)
        .def_readonly("macAddr0",        &OculusStatusMsg::macAddr0)
        .def_readonly("macAddr1",        &OculusStatusMsg::macAddr1)
        .def_readonly("macAddr2",        &OculusStatusMsg::macAddr2)
        .def_readonly("macAddr3",        &OculusStatusMsg::macAddr3)
        .def_readonly("macAddr4",        &OculusStatusMsg::macAddr4)
        .def_readonly("macAddr5",        &OculusStatusMsg::macAddr5)
        .def_readonly("temperature0",    &OculusStatusMsg::temperature0)
        .def_readonly("temperature1",    &OculusStatusMsg::temperature1)
        .def_readonly("temperature2",    &OculusStatusMsg::temperature2)
        .def_readonly("temperature3",    &OculusStatusMsg::temperature3)
        .def_readonly("temperature4",    &OculusStatusMsg::temperature4)
        .def_readonly("temperature5",    &OculusStatusMsg::temperature5)
        .def_readonly("temperature6",    &OculusStatusMsg::temperature6)
        .def_readonly("temperature7",    &OculusStatusMsg::temperature7)
        .def_readonly("pressure",        &OculusStatusMsg::pressure)
        .def("__str__", [](const OculusStatusMsg& msg) {
            std::ostringstream oss;
            oss << msg;
            return oss.str();
        });

    //py::class_<oculus::Message, oculus::Message::Ptr>(m_, "OculusMessage")
    //    .def(py::init<>())
    //    .def("header", &oculus::Message::header)
    //    .def("data",   [](const oculus::Message::ConstPtr& msg) {
    //        return make_memory_view(msg->data());
    //     })
    //    .def("__str__", [](const oculus::Message::ConstPtr& msg) {
    //        std::ostringstream oss;
    //        oss << "OculusMessage :\n" << msg->header();
    //        return oss.str();
    //    });

    //py::class_<oculus::PingMessage1, oculus::PingMessage1::Ptr>(m_, "PingMessage1")
    //    .def(py::init<const oculus::Message::ConstPtr&>())
    //    .def("header",        &oculus::PingMessage1::header)
    //    .def("data",          [](const oculus::PingMessage1::ConstPtr& msg) {
    //        return make_memory_view(msg->data());
    //    })
    //    .def("range_count",   &oculus::PingMessage1::range_count)
    //    .def("bearing_count", &oculus::PingMessage1::bearing_count)
    //    .def("bearing_data",  [](const oculus::PingMessage1::ConstPtr& msg) {
    //        return make_memory_view(msg->bearing_count(), msg->bearing_data());
    //    })
    //    .def("raw_ping_data", [](const oculus::PingMessage1::ConstPtr& msg) {
    //        return make_raw_ping_data_view(*msg);
    //    })
    //    .def("gains", [](const oculus::PingMessage1::ConstPtr& msg) {
    //        return make_gains_view(*msg);
    //    })
    //    .def("ping_data", [](const oculus::PingMessage1::ConstPtr& msg) {
    //        return make_ping_data_view(*msg);
    //    });

    py::class_<OculusPythonHandle>(m_, "OculusSonar")
        .def(py::init<>())
        .def("start",       &OculusPythonHandle::start)
        .def("stop",        &OculusPythonHandle::stop)

        .def("send_config",    &OculusPythonHandle::send_config)
        .def("request_config", &OculusPythonHandle::request_config)
        .def("current_config", &OculusPythonHandle::current_config)

        .def("add_message_callback", &OculusPythonHandle::add_message_callback)
        .def("add_ping_callback",    &OculusPythonHandle::add_ping_callback)
        .def("add_status_callback",  &OculusPythonHandle::add_status_callback)
        .def("add_config_callback",  &OculusPythonHandle::add_config_callback)

        .def("recorder_start", &OculusPythonHandle::recorder_start)
        .def("recorder_stop",  &OculusPythonHandle::recorder_stop)
        .def("is_recording",   &OculusPythonHandle::is_recording);

    init_oculus_message(m_);
    init_oculus_python_files(m_);
}




