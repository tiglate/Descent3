set(HEADERS
  grtext.h
  grtextlib.h
)
set(CPPS
  grfont.cpp
  grtext.cpp
  textaux.cpp
)

set(font_source "${CMAKE_CURRENT_BINARY_DIR}/popup_font_data.cpp")
add_custom_command(
  OUTPUT "${font_source}"
  COMMAND ${CMAKE_COMMAND}
    "-DINPUT=${PROJECT_SOURCE_DIR}/third_party/fonts/Arimo-Bold.ttf"
    "-DOUTPUT=${font_source}"
    -P "${PROJECT_SOURCE_DIR}/cmake/embed_font.cmake"
  DEPENDS "${PROJECT_SOURCE_DIR}/third_party/fonts/Arimo-Bold.ttf"
          "${PROJECT_SOURCE_DIR}/cmake/embed_font.cmake"
  VERBATIM
)
set(menu_font_source "${CMAKE_CURRENT_BINARY_DIR}/orbitron_font_data.cpp")
add_custom_command(
  OUTPUT "${menu_font_source}"
  COMMAND ${CMAKE_COMMAND}
    "-DINPUT=${PROJECT_SOURCE_DIR}/third_party/fonts/Orbitron.ttf"
    "-DOUTPUT=${menu_font_source}"
    "-DSYMBOL=OrbitronData"
    -P "${PROJECT_SOURCE_DIR}/cmake/embed_font.cmake"
  DEPENDS "${PROJECT_SOURCE_DIR}/third_party/fonts/Orbitron.ttf"
          "${PROJECT_SOURCE_DIR}/cmake/embed_font.cmake"
  VERBATIM
)
add_library(popup_font STATIC scalable_font.cpp scalable_font.h "${font_source}" "${menu_font_source}")
target_link_libraries(popup_font PRIVATE stb)
target_include_directories(popup_font PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

install(FILES "${PROJECT_SOURCE_DIR}/third_party/fonts/OFL.txt"
  DESTINATION "${CMAKE_INSTALL_DOCDIR}" RENAME "popup-font-OFL.txt")
install(FILES "${PROJECT_SOURCE_DIR}/third_party/fonts/Orbitron-OFL.txt"
  DESTINATION "${CMAKE_INSTALL_DOCDIR}" RENAME "orbitron-font-OFL.txt")
install(FILES "${PROJECT_SOURCE_DIR}/third_party/fonts/README.md"
  DESTINATION "${CMAKE_INSTALL_DOCDIR}" RENAME "bundled-fonts.md")

add_library(grtext STATIC ${HEADERS} ${CPPS})
target_link_libraries(grtext PRIVATE
  ddebug
  ddio
  logger
  mem
  popup_font
)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
target_include_directories(grtext PUBLIC
  $<BUILD_INTERFACE:
    ${PROJECT_SOURCE_DIR}/grtext
  >
)
