cmake_minimum_required(VERSION 3.20)
project(airtrack LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Wpedantic)

add_library(airtrack_core src/sbs.cpp)
target_include_directories(airtrack_core PUBLIC src)

add_executable(udp_sender src/udp_sender.cpp)
add_executable(udp_receiver src/udp_receiver.cpp)

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(googletest)

enable_testing()
include(GoogleTest)

add_executable(airtrack_tests tests/smoke_test.cpp tests/sbs_test.cpp)
target_link_libraries(airtrack_tests PRIVATE airtrack_core GTest::gtest_main)
target_compile_definitions(airtrack_tests PRIVATE
    SBS_SAMPLE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/tests/data/sample.sbs")
gtest_discover_tests(airtrack_tests)
