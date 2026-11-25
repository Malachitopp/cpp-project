cmake_minimum_required(VERSION 3.20)
project(airtrack LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Wpedantic)

add_executable(udp_sender src/udp_sender.cpp)
add_executable(udp_receiver src/udp_receiver.cpp)

# --- Tests ---------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(googletest)

enable_testing()
include(GoogleTest)

add_executable(airtrack_tests tests/smoke_test.cpp)
target_link_libraries(airtrack_tests PRIVATE GTest::gtest_main)
gtest_discover_tests(airtrack_tests)
