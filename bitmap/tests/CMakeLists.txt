set(CMAKE_FOLDER "tests")

add_executable(pcx_loader_tests pcx_loader_tests.cpp ../pcx.cpp)
target_include_directories(pcx_loader_tests PRIVATE ${PROJECT_SOURCE_DIR}/lib ${PROJECT_SOURCE_DIR}/bitmap)
target_link_libraries(pcx_loader_tests PRIVATE GTest::gtest_main cfile logger mem)
gtest_discover_tests(pcx_loader_tests PROPERTIES TIMEOUT 15)
