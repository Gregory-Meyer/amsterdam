cmake_minimum_required(VERSION 3.8)

project(amsterdam)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(include src)

add_library(amsterdam STATIC src/amsterdam.cpp)

option(CHANNELS_BUILD_TESTS "Build tests for libamsterdam." ON)
if(CHANNELS_BUILD_TESTS)
    find_package(Catch2 REQUIRED)
    find_package(Threads REQUIRED)

    add_executable(test_channel test/runner.cpp test/channel.spec.cpp)
    target_link_libraries(test_channel Threads::Threads Catch2::Catch2
                          amsterdam)

    include(CTest)
    include(Catch)
    catch_discover_tests(test_channel)
endif()
