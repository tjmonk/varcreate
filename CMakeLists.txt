cmake_minimum_required(VERSION 3.10)

project(VARCREATE
    VERSION 0.1
    DESCRIPTION "Variable Creation Components"
)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(libvarcreate)
add_subdirectory(varcreate)
