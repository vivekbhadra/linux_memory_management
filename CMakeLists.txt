cmake_minimum_required(VERSION 3.16)
project(linux_systems_examples CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_compile_options(-Wall -Wextra -O0 -g)

find_package(Threads REQUIRED)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
     ${CMAKE_SOURCE_DIR}/memory_management/*.cpp
     ${CMAKE_SOURCE_DIR}/process_management/*.cpp
     ${CMAKE_SOURCE_DIR}/scheduling/*.cpp)

foreach(src ${SOURCES})
    get_filename_component(name ${src} NAME_WE)
    add_executable(${name} ${src})
    target_link_libraries(${name} PRIVATE Threads::Threads)
endforeach()
