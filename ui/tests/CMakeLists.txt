set(CMAKE_FOLDER "tests")
add_executable(ui_scale_tests window_scale_tests.cpp)
target_include_directories(ui_scale_tests PRIVATE ${PROJECT_SOURCE_DIR}/ui ${PROJECT_SOURCE_DIR}/lib)
target_link_libraries(ui_scale_tests PRIVATE GTest::gtest_main)
gtest_discover_tests(ui_scale_tests)

add_executable(ui_text_item_tests text_item_tests.cpp ../UIRes.cpp)
target_include_directories(ui_text_item_tests PRIVATE
  ${PROJECT_SOURCE_DIR}/ui ${PROJECT_SOURCE_DIR}/lib ${PROJECT_SOURCE_DIR}/ddebug ${PROJECT_SOURCE_DIR}/grtext
  ${PROJECT_SOURCE_DIR}/ddio)
target_link_libraries(ui_text_item_tests PRIVATE GTest::gtest_main logger mem)
gtest_discover_tests(ui_text_item_tests)
