set(CMAKE_FOLDER "tests")
add_executable(registry_tests registry_tests.cpp ../registry.cpp)
target_link_libraries(registry_tests PRIVATE GTest::gtest_main logger)
target_include_directories(registry_tests PRIVATE ..)

# Inject malloc failures only into the test executable, without adding an
# allocator override to the production API. Other platforms run the normal cases.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_compile_definitions(registry_tests PRIVATE REGISTRY_TEST_WRAP_MALLOC)
  target_link_options(registry_tests PRIVATE "LINKER:--wrap=malloc")
  target_compile_options(registry_tests PRIVATE -fno-builtin-malloc)
endif()
gtest_discover_tests(registry_tests PROPERTIES TIMEOUT 15)

add_executable(raw_console_tests raw_console_tests.cpp ../lnxcon_raw.cpp)
target_link_libraries(raw_console_tests PRIVATE GTest::gtest_main)
target_include_directories(raw_console_tests PRIVATE ${PROJECT_SOURCE_DIR}/ddio)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_compile_definitions(raw_console_tests PRIVATE RAW_CONSOLE_TEST_WRAP_ALLOCATIONS)
  target_compile_options(raw_console_tests PRIVATE -fno-builtin-malloc)
  target_link_options(raw_console_tests PRIVATE "LINKER:--wrap=malloc" "LINKER:--wrap=free")
endif()
gtest_discover_tests(raw_console_tests PROPERTIES TIMEOUT 15)
