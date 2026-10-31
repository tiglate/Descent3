cmake_path(SET GENERATED_HEADERS NORMALIZE ${CMAKE_CURRENT_BINARY_DIR}/generated)
cmake_path(SET SHADERS_H NORMALIZE ${GENERATED_HEADERS}/shaders.h)
set(SHADERS
  shaders/fragment.glsl
  shaders/vertex.glsl
)
add_custom_command(
  OUTPUT ${SHADERS_H}
  COMMAND ${CMAKE_COMMAND} -Dshaders_h=${SHADERS_H} -P ../cmake/shaders.cmake -- ${SHADERS}
  DEPENDS ../cmake/shaders.cmake ${SHADERS}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

set(HEADERS
  dyna_gl.h
  HardwareInternal.h
  ShaderProgram.h
  ${SHADERS_H}
)
set(CPPS
  HardwareClipper.cpp
  HardwareDraw.cpp
  HardwareGlobalVars.cpp
  HardwareInstance.cpp
  HardwareOpenGL.cpp
  HardwareBaseGPU.cpp
  HardwarePoints.cpp
  HardwareSetup.cpp
  HardwareTransforms.cpp
)

add_library(renderer STATIC ${HEADERS} ${CPPS})
if(BUILD_TESTING)
  add_executable(render_dimensions_tests tests/render_dimensions_tests.cpp)
  target_include_directories(render_dimensions_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  target_link_libraries(render_dimensions_tests PRIVATE GTest::gtest_main)
  gtest_discover_tests(render_dimensions_tests)

  if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND ENABLE_MEM_RTL)
    # Compile the production entry points with section GC so the lifecycle
    # fixtures can supply synthetic bitmap/input data without linking gameplay.
    file(STRINGS dyna_gl.h GL_TEST_ENTRY_POINTS REGEX "^DYNAEXTERN\\(gl")
    set(GL_TEST_STUBS "")
    foreach(GL_TEST_ENTRY IN LISTS GL_TEST_ENTRY_POINTS)
      string(REGEX MATCH "DYNAEXTERN\\((gl[A-Za-z0-9_]+)\\)" GL_TEST_MATCH "${GL_TEST_ENTRY}")
      string(APPEND GL_TEST_STUBS "GL_STUB(${CMAKE_MATCH_1})\n")
    endforeach()
    file(GENERATE OUTPUT ${GENERATED_HEADERS}/test_gl_entries.h CONTENT "${GL_TEST_STUBS}")
    add_executable(renderer_lifecycle_tests
      tests/lifecycle_tests.cpp HardwareOpenGL.cpp HardwareBaseGPU.cpp ${SHADERS_H})
    target_include_directories(renderer_lifecycle_tests PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR} ${GENERATED_HEADERS} ${PROJECT_SOURCE_DIR}/bitmap ${PROJECT_SOURCE_DIR}/ddio)
    target_compile_options(renderer_lifecycle_tests PRIVATE -ffunction-sections -fdata-sections -fno-builtin-malloc)
    target_link_libraries(renderer_lifecycle_tests PRIVATE
      GTest::gtest_main glm::glm SDL3::SDL3 ddebug logger mem misc module rtperformance)
    target_link_options(renderer_lifecycle_tests PRIVATE "LINKER:--gc-sections"
      "LINKER:--wrap=malloc" "LINKER:--wrap=free" "LINKER:--wrap=_Z5ErrorPKcz")
    foreach(SDL_FN Init WasInit GL_LoadLibrary GL_UnloadLibrary GL_SetAttribute
        CreateWindowWithProperties DestroyWindow GL_CreateContext GL_MakeCurrent GL_DestroyContext
        GL_GetProcAddress GetWindowSizeInPixels SetWindowSize SetWindowRelativeMouseMode
        SetWindowFullscreen SyncWindow GL_SwapWindow)
      target_link_options(renderer_lifecycle_tests PRIVATE "LINKER:--wrap=SDL_${SDL_FN}")
    endforeach()
    # PRE_TEST keeps separate discovery files for Debug and Release builds.
    gtest_discover_tests(renderer_lifecycle_tests DISCOVERY_MODE PRE_TEST PROPERTIES TIMEOUT 30)
  endif()
endif()
target_include_directories(renderer PRIVATE ${GENERATED_HEADERS} ${SDL3_INCLUDE_DIRS})
target_link_libraries(renderer PRIVATE
  glm::glm
  SDL3::SDL3
  bitmap
  ddebug
  ddio
  logger
  mem
  misc
  module
  rtperformance
)
