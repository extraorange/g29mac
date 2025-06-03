cmake_minimum_required(VERSION 3.22)
project(g29mac VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Werror)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

find_library(IOKIT IOKit)
find_library(COREFOUNDATION CoreFoundation)
add_executable(g29mac main.cpp)

target_link_libraries(g29mac ${IOKIT} ${COREFOUNDATION})
