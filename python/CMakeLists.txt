cmake_minimum_required(VERSION 3.16)
project(oculus_driver_python VERSION 1.2 LANGUAGES CXX)

find_package(pybind11 REQUIRED)

if(NOT TARGET oculus_driver)
    # Creating a symlink to top directory (works on windows since cmake 3.13)
    execute_process(COMMAND ${CMAKE_COMMAND} -E 
                    create_symlink
                    ${CMAKE_CURRENT_SOURCE_DIR}/../
                    ${CMAKE_CURRENT_BINARY_DIR}/oculus_driver_simlink)
    add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/oculus_driver_simlink)
endif() 


pybind11_add_module(_oculus_python
    src/oculus_message.cpp
    src/oculus_files.cpp
    src/oculus_python.cpp
)
target_link_libraries(_oculus_python PUBLIC
    oculus_driver
)
