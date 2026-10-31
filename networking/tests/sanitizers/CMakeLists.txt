cmake_minimum_required(VERSION 3.20)
project(AsyncDnsSanitizers LANGUAGES CXX)

# A small standalone build for sanitizer runs without instrumenting the engine.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
get_filename_component(D3_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
find_package(SDL3 CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)
find_package(Threads REQUIRED)

add_executable(async_dns_tests ../async_dns_tests.cpp ../../async_dns.cpp)
target_include_directories(async_dns_tests PRIVATE "${D3_ROOT}/networking")
target_link_libraries(async_dns_tests PRIVATE GTest::gtest_main SDL3::SDL3 Threads::Threads
  $<$<PLATFORM_ID:Windows>:ws2_32>)
