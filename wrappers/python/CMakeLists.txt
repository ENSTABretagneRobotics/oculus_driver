cmake_minimum_required(VERSION 3.16)
project(oculus_driver_python VERSION 0.1 LANGUAGES CXX)

find_package(pybind11 REQUIRED)

find_package(oculus_driver_c REQUIRED)

pybind11_add_module(oculus_python src/oculus_python.cpp)
target_link_libraries(oculus_python PUBLIC
    oculus_driver_c
)


get_cmake_property(_variableNames VARIABLES)
list (SORT _variableNames)
foreach (_variableName ${_variableNames})
    message(STATUS "${_variableName}=${${_variableName}}")
endforeach()
