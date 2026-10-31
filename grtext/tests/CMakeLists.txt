set(CMAKE_FOLDER "tests")
add_executable(scalable_font_tests scalable_font_tests.cpp)
target_link_libraries(scalable_font_tests PRIVATE popup_font GTest::gtest_main)
gtest_discover_tests(scalable_font_tests)
