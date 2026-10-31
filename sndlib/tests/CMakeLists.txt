set(CMAKE_FOLDER "tests")

# Compile the production loader directly so the fixtures do not need a game or
# an audio device. Only the paging counters below are supplied by the tests.
add_executable(wave_loader_tests wave_loader_tests.cpp ../ddsoundload.cpp)
target_include_directories(wave_loader_tests PRIVATE ..)
target_link_libraries(wave_loader_tests PRIVATE GTest::gtest_main cfile logger mem)
gtest_discover_tests(wave_loader_tests PROPERTIES TIMEOUT 15)
