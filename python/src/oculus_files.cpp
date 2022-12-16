#include <pybind11/pybind11.h>
namespace py = pybind11;

#include <oculus_driver/Oculus.h>
#include <oculus_driver/Recorder.h>

#include "oculus_message.h"


struct OculusFileReader
{
    oculus::FileReader           file_;
    oculus::blueprint::LogItem   msgHeader_;
    std::vector<uint8_t>         data_;

    OculusFileReader(const std::string& path) : file_(path) {}

    const oculus::blueprint::LogHeader file_header() const { return file_.file_header(); }
    py::object read_next_message() const {
        auto msg = file_.read_next_message();
        if(msg)
            return py::cast(msg.get());
        else
            return py::none();
    }

    py::object read_next_ping() const {
        auto msg = file_.read_next_ping();
        if(msg) {
            return py::cast(msg);
        }
        else
            return py::none();
    }

    void rewind() {
        file_.rewind();
    }
};

void init_oculus_python_files(py::module& parentModule)
{
    py::module m_ = parentModule.def_submodule("files", "Submodule to read .oculus files.");

    py::class_<oculus::blueprint::LogHeader>(m_, "FileHeader")
        .def(py::init<>())
        .def_readonly("fileHeader", &oculus::blueprint::LogHeader::fileHeader)
        .def_readonly("sizeHeader", &oculus::blueprint::LogHeader::sizeHeader)
        .def_readonly("source",     &oculus::blueprint::LogHeader::source)
        .def_readonly("version",    &oculus::blueprint::LogHeader::version)
        .def_readonly("encryption", &oculus::blueprint::LogHeader::encryption)
        .def_readonly("key",        &oculus::blueprint::LogHeader::key)
        .def_readonly("time",       &oculus::blueprint::LogHeader::time);
    py::class_<oculus::blueprint::LogItem>(m_, "ItemHeader")
        .def(py::init<>())
        .def_readonly("itemHeader",   &oculus::blueprint::LogItem::itemHeader)
        .def_readonly("sizeHeader",   &oculus::blueprint::LogItem::sizeHeader)
        .def_readonly("type",         &oculus::blueprint::LogItem::type)
        .def_readonly("version",      &oculus::blueprint::LogItem::version)
        .def_readonly("time",         &oculus::blueprint::LogItem::time)
        .def_readonly("compression",  &oculus::blueprint::LogItem::compression)
        .def_readonly("originalSize", &oculus::blueprint::LogItem::originalSize)
        .def_readonly("payloadSize",  &oculus::blueprint::LogItem::payloadSize);
    py::enum_<oculus::blueprint::RecordTypes>(m_, "RecordTypes")
        .value("rt_settings",          oculus::blueprint::rt_settings)
        .value("rt_serialPort",        oculus::blueprint::rt_serialPort)
        .value("rt_oculusSonar",       oculus::blueprint::rt_oculusSonar)
        .value("rt_blueviewSonar",     oculus::blueprint::rt_blueviewSonar)
        .value("rt_rawVideo",          oculus::blueprint::rt_rawVideo)
        .value("rt_h264Video",         oculus::blueprint::rt_h264Video)
        .value("rt_apBattery",         oculus::blueprint::rt_apBattery)
        .value("rt_apMissionProgress", oculus::blueprint::rt_apMissionProgress)
        .value("rt_nortekDVL",         oculus::blueprint::rt_nortekDVL)
        .value("rt_apNavData",         oculus::blueprint::rt_apNavData)
        .value("rt_apDvlData",         oculus::blueprint::rt_apDvlData)
        .value("rt_apAhrsData",        oculus::blueprint::rt_apAhrsData)
        .value("rt_apSonarHeader",     oculus::blueprint::rt_apSonarHeader)
        .value("rt_rawSonarImage",     oculus::blueprint::rt_rawSonarImage)
        .value("rt_ahrsMtData2",       oculus::blueprint::rt_ahrsMtData2)
        .value("rt_apVehicleInfo",     oculus::blueprint::rt_apVehicleInfo)
        .value("rt_apMarker",          oculus::blueprint::rt_apMarker)
        .value("rt_apGeoImageHeader",  oculus::blueprint::rt_apGeoImageHeader)
        .value("rt_apGeoImageData",    oculus::blueprint::rt_apGeoImageData)
        .value("rt_sbgData",           oculus::blueprint::rt_sbgData)
        .value("rt_ocViewInfo",        oculus::blueprint::rt_ocViewInfo)
        .value("rt_oculusSonarStamp",  oculus::blueprint::rt_oculusSonarStamp)
        .export_values();

    py::class_<OculusFileReader>(m_, "OculusFileReader")
        .def(py::init<const std::string&>())
        .def("file_header",       &OculusFileReader::file_header)
        .def("read_next_message", &OculusFileReader::read_next_message)
        .def("read_next_ping",    &OculusFileReader::read_next_ping)
        .def("rewind",            &OculusFileReader::rewind);
}


