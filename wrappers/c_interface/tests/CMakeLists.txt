

list(APPEND test_files
    src/opaque_handle_test.c
)

foreach(filename ${test_files})
    get_filename_component(name ${filename} NAME_WE)
    set(target_name ${name}_${PROJECT_NAME})
    add_executable(${target_name} ${filename})
    target_link_libraries(${target_name} PRIVATE oculus_driver_c)
endforeach()

