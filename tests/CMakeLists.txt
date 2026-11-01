set(CMAKE_FOLDER "tests")

add_executable(
  byteswap_tests
  byteswap_tests.cpp
)
target_link_libraries(
  byteswap_tests
  GTest::gtest_main
)
target_include_directories(byteswap_tests PRIVATE ${PROJECT_SOURCE_DIR}/lib)

gtest_discover_tests(byteswap_tests)

add_executable(control_bindings_tests control_bindings_tests.cpp)
target_include_directories(control_bindings_tests PRIVATE
  ${PROJECT_SOURCE_DIR}/lib ${PROJECT_SOURCE_DIR}/ddio ${PROJECT_SOURCE_DIR}/Descent3
  ${PROJECT_SOURCE_DIR}/linux ${PROJECT_SOURCE_DIR}/win32)
target_link_libraries(control_bindings_tests PRIVATE GTest::gtest_main)
gtest_discover_tests(control_bindings_tests)

add_executable(game_assets_tests game_assets_tests.cpp ../Descent3/game_assets.cpp)
target_include_directories(game_assets_tests PRIVATE ${PROJECT_SOURCE_DIR}/Descent3)
target_link_libraries(game_assets_tests PRIVATE GTest::gtest_main)
gtest_discover_tests(game_assets_tests)

add_executable(settings_defaults_tests settings_defaults_tests.cpp)
target_include_directories(settings_defaults_tests PRIVATE ${PROJECT_SOURCE_DIR}/Descent3)
target_link_libraries(settings_defaults_tests PRIVATE GTest::gtest_main)
gtest_discover_tests(settings_defaults_tests)

find_package(PNG REQUIRED)
add_executable(menu_image_tests menu_image_tests.cpp ../Descent3/menu_image_decode.cpp)
target_include_directories(menu_image_tests PRIVATE ${PROJECT_SOURCE_DIR}/Descent3)
target_compile_definitions(menu_image_tests PRIVATE D3_MENU_IMAGE_PATH="${PROJECT_SOURCE_DIR}/images/mainmenu_new.png")
target_link_libraries(menu_image_tests PRIVATE GTest::gtest_main PNG::PNG)
gtest_discover_tests(menu_image_tests)
