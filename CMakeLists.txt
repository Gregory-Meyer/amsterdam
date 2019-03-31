cmake_minimum_required(VERSION 3.8)

project(amsterdam)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(include src)

add_library(amsterdam STATIC src/amsterdam.cpp)

install(TARGETS amsterdam DESTINATION lib)
install(FILES include/amsterdam.h DESTINATION include)

option(AMSTERDAM_BUILD_TESTS "Build tests for libamsterdam." ON)
if(AMSTERDAM_BUILD_TESTS)
    find_package(Catch2 REQUIRED)
    find_package(Threads REQUIRED)

    add_executable(test_channel test/runner.cpp test/channel.spec.cpp)
    target_link_libraries(test_channel Threads::Threads Catch2::Catch2
                          amsterdam)

    include(CTest)
    include(Catch)
    catch_discover_tests(test_channel)
endif()

option(AMSTERDAM_BUILD_DOCS "Build documentation for libamsterdam." OFF)
if(AMSTERDAM_BUILD_DOCS)
    set(DOXYGEN_SKIP_DOT ON)
    find_package(Doxygen REQUIRED)

    set(DOXYGEN_IN "${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in")
    set(DOXYGEN_OUT "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile")

    configure_file("${DOXYGEN_IN}" "${DOXYGEN_OUT}" @ONLY)

    add_custom_target(doc ALL "${DOXYGEN_EXECUTABLE}" "${DOXYGEN_OUT}"
                      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                      COMMENT "Generating API documentation with Doxygen"
                      VERBATIM)

    install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/doc/" DESTINATION "share/doc/amsterdam")
endif()
