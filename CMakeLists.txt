cmake_minimum_required(VERSION 3.16)
project(oculus_driver VERSION 0.1)

find_package(Boost COMPONENTS system thread REQUIRED)
message(STATUS "Boost version  : ${Boost_VERSION}")
message(STATUS "Boost location : ${Boost_INCLUDE_DIRS}")

list(APPEND oculus_driver_headers
    include/oculus_driver/Oculus.h
    include/oculus_driver/utils.h
    include/oculus_driver/print_utils.h
    include/oculus_driver/Clock.h
    include/oculus_driver/CallbackQueue.h
    include/oculus_driver/StatusListener.h
    include/oculus_driver/SonarClient.h
    include/oculus_driver/SonarDriver.h
    include/oculus_driver/AsyncService.h
)

add_library(oculus_driver SHARED
    src/print_utils.cpp
    src/StatusListener.cpp
    src/SonarClient.cpp
    src/SonarDriver.cpp
    src/AsyncService.cpp
)
set_target_properties(oculus_driver PROPERTIES
    PUBLIC_HEADER "${oculus_driver_headers}"
    INSTALL_RPATH_USE_LINK_PATH True
)
target_include_directories(oculus_driver PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(oculus_driver PUBLIC
    Boost::system
    Boost::thread
)
target_compile_features(oculus_driver PUBLIC cxx_std_17)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/narval_installation.cmake)
narval_install_target(oculus_driver)

# add_subdirectory(tools)
add_subdirectory(tests)


