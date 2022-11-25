cmake_minimum_required(VERSION 3.16)
project(oculus_driver_tests VERSION 0.1)

list(APPEND test_files
    src/statusraw_test.cpp
    src/status_display.cpp
    src/client_test.cpp
    src/async_client_test.cpp
    src/config_test.cpp
    src/recorder_test.cpp
    src/filereader_test.cpp
    src/helpers_test.cpp
)

foreach(filename ${test_files})
    get_filename_component(test_name ${filename} NAME_WE)
    set(target_name ${test_name}_${PROJECT_NAME})
    add_executable(${target_name} ${filename})
    target_link_libraries(${target_name} oculus_driver)
endforeach()



