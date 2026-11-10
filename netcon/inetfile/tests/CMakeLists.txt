set(CMAKE_FOLDER "tests")

add_executable(httpclient_timeout_tests httpclient_timeout_tests.cpp)
target_link_libraries(httpclient_timeout_tests PRIVATE GTest::gtest_main inetfile)
gtest_discover_tests(httpclient_timeout_tests PROPERTIES TIMEOUT 30)
