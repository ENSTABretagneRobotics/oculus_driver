cmake_minimum_required(VERSION 3.16)
project(oculus_driver_c VERSION 0.1)

option(BUILD_TESTS "Build unit tests" ON)

if(NOT TARGET oculus_driver)
    find_package(oculus_driver REQUIRED)
endif()

add_library(oculus_driver_c
    src/oculus_driver.cpp
)
target_include_directories(oculus_driver_c PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(oculus_driver_c PUBLIC
    oculus_driver
)

#############
## Install ##
#############
include(GNUInstallDirs)

install(DIRECTORY include/oculus_driver_c
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(TARGETS ${PROJECT_NAME}
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
        EXPORT ${PROJECT_NAME}Targets)

# install the exported targets
install(EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Targets.cmake
    # NAMESPACE ${PROJECT_NAME}:: # namespacing does not play well with fetch content
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

############
## Config ##
############
include(CMakePackageConfigHelpers)
configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
    PATH_VARS CMAKE_INSTALL_INCLUDEDIR CMAKE_INSTALL_LIBDIR
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    VERSION "${CMAKE_PROJECT_VERSION}"
    COMPATIBILITY AnyNewerVersion
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    DESTINATION lib/cmake/${PROJECT_NAME}
)
export(EXPORT ${PROJECT_NAME}Targets
    # NAMESPACE ${PROJECT_NAME}:: # namespacing does not play well with fetch content
    FILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Targets.cmake"
)

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()


