cmake_minimum_required(VERSION 3.16)
project(oculus_driver_python VERSION 0.1 LANGUAGES CXX)

find_package(pybind11 REQUIRED)

if(NOT TARGET oculus_driver)
    find_package(oculus_driver QUIET)
    if(NOT TARGET oculus_driver)
    include(FetchContent)
    FetchContent_Declare(oculus_driver
        GIT_REPOSITORY https://github.com/ENSTABretagneRobotics/oculus_driver.git
        GIT_TAG        v1.1
    )
    FetchContent_MakeAvailable(oculus_driver)
    endif()
endif()


pybind11_add_module(oculus_python
    src/oculus_files.cpp
    src/oculus_python.cpp
)
target_link_libraries(oculus_python PUBLIC
    oculus_driver
)
