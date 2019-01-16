cmake_minimum_required(VERSION 3.12)

project(channels)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

include_directories(include/)

add_executable(test_channel test/channel.cpp src/channel.cpp)
target_link_libraries(test_channel Threads::Threads)
