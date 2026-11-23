cmake_minimum_required(VERSION 3.21)
project(islamic-times VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core)

enable_testing()
add_subdirectory(src)
add_subdirectory(tests)

option(ISLAMIC_TIMES_TOOLS "Build the report tool" ON)
if(ISLAMIC_TIMES_TOOLS)
    add_subdirectory(tools/report)
    add_subdirectory(tools/smoke)
endif()
