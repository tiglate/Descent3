set(CMAKE_FOLDER "tests")

add_executable(cfile_tests
  cfile_tests.cpp
  inffile_tests.cpp
  file_length_tests.cpp
)
target_link_libraries(cfile_tests PRIVATE
  GTest::gtest_main
  cfile
)
target_include_directories(cfile_tests PRIVATE ${PROJECT_SOURCE_DIR}/lib)

gtest_discover_tests(cfile_tests
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/cfile/tests
)

# Exercise the real CFILE library with a returning Error() handler, matching
# Debug's Ignore/Cancel path. Fault injection stays out of the production API.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND ENABLE_MEM_RTL)
  add_executable(cfile_allocation_tests allocation_failure_tests.cpp)
  target_link_libraries(cfile_allocation_tests PRIVATE GTest::gtest_main cfile)
  target_link_options(cfile_allocation_tests PRIVATE
    "LINKER:--wrap=malloc" "LINKER:--wrap=strdup" "LINKER:--wrap=free"
    "LINKER:--wrap=fopen" "LINKER:--wrap=fclose" "LINKER:--wrap=_Z5ErrorPKcz"
  )
  gtest_discover_tests(cfile_allocation_tests PROPERTIES TIMEOUT 15)
endif()
