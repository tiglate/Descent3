set(CMAKE_FOLDER "tests")

find_package(Threads REQUIRED)
add_executable(async_dns_tests async_dns_tests.cpp)
target_link_libraries(async_dns_tests PRIVATE GTest::gtest_main networking SDL3::SDL3 Threads::Threads)
target_include_directories(async_dns_tests PRIVATE ${PROJECT_SOURCE_DIR}/networking)
gtest_discover_tests(async_dns_tests PROPERTIES TIMEOUT 15)
