/*
 * Descent 3
 * Copyright (C) 2026 Descent Developers
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#define SDL_FUNCTION_POINTER_IS_VOID_POINTER
#include <SDL3/SDL.h>

#include "3d.h"
#include "application.h"
#include "args.h"
#include "bitmap.h"
#include "config.h"
#include "dyna_gl.h"
#include "HardwareInternal.h"
#include "lightmap.h"
#include "rend_opengl.h"
#include "renderer.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>
#include <string>
#include <type_traits>

// Only asset/input/transform data is synthetic. Startup, resource ownership,
// shader construction, state changes and cleanup use the production sources.
std::vector<tVideoResolution> Video_res_list{{640, 480}};
int Current_video_resolution_id = 0;
int Display_id = 0;
bool Game_fullscreen = false;
bool Force_one_texture = false;
bms_bitmap GameBitmaps[MAX_BITMAPS]{};
bms_lightmap GameLightmaps[MAX_LIGHTMAPS]{};
int bm_w(int, int) { return 2; }
int bm_h(int, int) { return 2; }
int bm_mipped(int) { return 0; }
int bm_format(int) { return BITMAP_FORMAT_1555; }
uint16_t *bm_data(int, int) {
  static uint16_t pixels[4]{0xffff, 0xffff, 0xffff, 0xffff};
  return pixels;
}
uint16_t *lm_data(int) { return bm_data(0, 0); }
void g3_ForceTransformRefresh() {}
void g3_RefreshTransforms(bool) { rend_TransformSetToPassthru(); }
bool ddio_MouseGetGrab() { return false; }
// No application instance is used here. Define the virtuals as well as the
// size callback so UBSan can retain the platform application's type information.
oeLnxApplication::~oeLnxApplication() = default;
void oeLnxApplication::init() {}
void oeLnxApplication::get_info(void *) {}
unsigned oeLnxApplication::defer() { return 0; }
const char *oeLnxApplication::get_window_name() { return "Renderer test"; }
void oeLnxApplication::clear_window() {}
void oeLnxApplication::set_defer_handler(void (*)(bool)) {}
void oeLnxApplication::delay(float) {}
int oeLnxApplication::flags() const { return 0; }
void oeLnxApplication::set_sizepos(int, int, int, int) {}
int FindArgChar(const char *, char) { return 0; }
char GameArgs[MAX_ARGS][MAX_CHARS_PER_ARG]{};
int lm_w(int) { return 2; }
int lm_h(int) { return 2; }

extern rendering_state gpu_state;
extern uint8_t Renderer_initted;
extern SDL_Window *GSDLWindow;
extern SDL_GLContext GSDLGLContext;

namespace {
enum class Failure {
  None,
  Video,
  Library,
  Attributes,
  Window,
  Context,
  Current,
  Proc,
  Drawable,
  Framebuffer,
  ShaderCreate,
  ShaderCompile,
  ProgramLink,
  Uniform
};
Failure failure = Failure::None;
bool real_driver = false;
bool video_initialized = true;
bool packed_pixels = false;
SDL_GLContext current_context = nullptr;
int windows = 0, contexts = 0, library_refs = 0, lookup_calls = 0, gl_calls = 0, bad_gl_calls = 0;
int generation = 0, error_calls = 0;
GLuint next_id = 1;
std::set<GLuint> gpu_objects;
bool watch_allocations = false;
int fail_allocation_after = -1;
std::array<void *, 32> cpu_objects{};
std::string driver_error = "injected renderer failure %s";

void RecordGL() {
  ++gl_calls;
  if (!current_context)
    ++bad_gl_calls;
}
GLuint NewObject() {
  RecordGL();
  const GLuint id = next_id++;
  gpu_objects.insert(id);
  return id;
}
void GLFUNCCALL GenObjects(GLsizei count, GLuint *ids) {
  for (GLsizei i = 0; i < count; ++i)
    ids[i] = NewObject();
}
void GLFUNCCALL DeleteObjects(GLsizei count, const GLuint *ids) {
  RecordGL();
  for (GLsizei i = 0; i < count; ++i)
    EXPECT_EQ(gpu_objects.erase(ids[i]), 1u);
}
void GLFUNCCALL DeleteObject(GLuint id) { DeleteObjects(1, &id); }
GLuint GLFUNCCALL CreateShader(GLenum) { return failure == Failure::ShaderCreate ? 0 : NewObject(); }
GLuint GLFUNCCALL CreateProgram() { return NewObject(); }
void GLFUNCCALL ShaderStatus(GLuint, GLenum, GLint *value) {
  RecordGL();
  *value = failure == Failure::ShaderCompile ? GL_FALSE : GL_TRUE;
}
void GLFUNCCALL ProgramStatus(GLuint, GLenum, GLint *value) {
  RecordGL();
  *value = failure == Failure::ProgramLink ? GL_FALSE : GL_TRUE;
}
void GLFUNCCALL InfoLog(GLuint, GLsizei capacity, GLsizei *length, GLchar *text) {
  *length = snprintf(text, capacity, "injected shader failure");
}
void GLFUNCCALL GetInteger(GLenum name, GLint *value) {
  RecordGL();
  *value = name == GL_NUM_EXTENSIONS && packed_pixels ? 1 : 0;
}
const GLubyte *GLFUNCCALL GetString(GLenum) {
  RecordGL();
  return reinterpret_cast<const GLubyte *>("synthetic GL");
}
const GLubyte *GLFUNCCALL GetStringIndex(GLenum, GLuint) {
  RecordGL();
  return reinterpret_cast<const GLubyte *>("GL_EXT_packed_pixels");
}
GLenum GLFUNCCALL FramebufferStatus(GLenum) {
  RecordGL();
  return failure == Failure::Framebuffer ? GL_FRAMEBUFFER_UNSUPPORTED : GL_FRAMEBUFFER_COMPLETE;
}
GLint GLFUNCCALL UniformLocation(GLuint, const GLchar *) {
  RecordGL();
  return failure == Failure::Uniform ? -1 : 0;
}
void *GLFUNCCALL MapBuffer(GLenum, GLintptr, GLsizeiptr, GLbitfield) {
  RecordGL();
  alignas(16) static std::array<char, 1024 * 1024> buffer{};
  return buffer.data();
}
template <typename T> struct GLStub;
template <typename Ret, typename... Args> struct GLStub<Ret GLFUNCCALL(Args...)> {
  static Ret GLFUNCCALL Invoke(Args...) {
    RecordGL();
    if constexpr (!std::is_void_v<Ret>)
      return {};
  }
};
} // namespace

extern "C" void *__real_malloc(size_t size);
extern "C" void __real_free(void *ptr);
extern "C" void *__wrap_malloc(size_t size) {
  const bool renderer_allocation = size == MAX_BITMAPS || size == MAX_LIGHTMAPS || size == 2 * MAX_BITMAPS ||
                                   size == 2 * MAX_LIGHTMAPS || size == 2048 * 2048 * 2 || size == 2048 * 2048 * 4 ||
                                   size == 65536 * 2 || size == 65536 * 4;
  if (watch_allocations && renderer_allocation) {
    if (fail_allocation_after == 0) {
      fail_allocation_after = -1;
      return nullptr;
    }
    if (fail_allocation_after > 0)
      --fail_allocation_after;
    void *ptr = __real_malloc(size);
    auto slot = std::find(cpu_objects.begin(), cpu_objects.end(), nullptr);
    if (slot == cpu_objects.end())
      std::abort();
    *slot = ptr;
    return ptr;
  }
  return __real_malloc(size);
}
extern "C" void __wrap_free(void *ptr) {
  if (ptr) {
    auto slot = std::find(cpu_objects.begin(), cpu_objects.end(), ptr);
    if (slot != cpu_objects.end())
      *slot = nullptr;
  }
  __real_free(ptr);
}
extern "C" void __wrap__Z5ErrorPKcz(const char *, ...) { ++error_calls; }

extern "C" bool __real_SDL_Init(SDL_InitFlags);
extern "C" bool __wrap_SDL_Init(SDL_InitFlags flags) {
  if (real_driver)
    return __real_SDL_Init(flags);
  video_initialized = failure != Failure::Video;
  return video_initialized;
}
extern "C" SDL_InitFlags __real_SDL_WasInit(SDL_InitFlags);
extern "C" SDL_InitFlags __wrap_SDL_WasInit(SDL_InitFlags flags) {
  return real_driver ? __real_SDL_WasInit(flags) : (video_initialized ? flags : 0);
}
extern "C" bool __real_SDL_GL_LoadLibrary(const char *);
extern "C" bool __wrap_SDL_GL_LoadLibrary(const char *path) {
  if (real_driver)
    return __real_SDL_GL_LoadLibrary(path);
  if (failure == Failure::Library)
    return false;
  ++library_refs;
  return true;
}
extern "C" void __real_SDL_GL_UnloadLibrary();
extern "C" void __wrap_SDL_GL_UnloadLibrary() {
  if (real_driver)
    return __real_SDL_GL_UnloadLibrary();
  EXPECT_EQ(contexts, 0);
  EXPECT_EQ(windows, 0);
  EXPECT_EQ(--library_refs, 0);
}
extern "C" bool __real_SDL_GL_SetAttribute(SDL_GLAttr, int);
extern "C" bool __wrap_SDL_GL_SetAttribute(SDL_GLAttr attr, int value) {
  return real_driver ? __real_SDL_GL_SetAttribute(attr, value) : failure != Failure::Attributes;
}
extern "C" SDL_Window *__real_SDL_CreateWindowWithProperties(SDL_PropertiesID);
extern "C" SDL_Window *__wrap_SDL_CreateWindowWithProperties(SDL_PropertiesID props) {
  if (real_driver)
    return __real_SDL_CreateWindowWithProperties(props);
  if (failure == Failure::Window)
    return nullptr;
  ++windows;
  return reinterpret_cast<SDL_Window *>(uintptr_t{0x100});
}
extern "C" void __real_SDL_DestroyWindow(SDL_Window *);
extern "C" void __wrap_SDL_DestroyWindow(SDL_Window *window) {
  if (real_driver)
    return __real_SDL_DestroyWindow(window);
  EXPECT_EQ(contexts, 0);
  EXPECT_EQ(--windows, 0);
}
extern "C" SDL_GLContext __real_SDL_GL_CreateContext(SDL_Window *);
extern "C" SDL_GLContext __wrap_SDL_GL_CreateContext(SDL_Window *window) {
  if (real_driver)
    return __real_SDL_GL_CreateContext(window);
  if (failure == Failure::Context) {
    SDL_SetError("%s", driver_error.c_str());
    return nullptr;
  }
  ++contexts;
  current_context = reinterpret_cast<SDL_GLContext>(static_cast<uintptr_t>(++generation * 16 + 0x100));
  return current_context;
}
extern "C" bool __real_SDL_GL_MakeCurrent(SDL_Window *, SDL_GLContext);
extern "C" bool __wrap_SDL_GL_MakeCurrent(SDL_Window *window, SDL_GLContext context) {
  if (real_driver)
    return __real_SDL_GL_MakeCurrent(window, context);
  if (context && failure == Failure::Current) {
    current_context = nullptr;
    return false;
  }
  current_context = context;
  return true;
}
extern "C" bool __real_SDL_GL_DestroyContext(SDL_GLContext);
extern "C" bool __wrap_SDL_GL_DestroyContext(SDL_GLContext context) {
  if (real_driver)
    return __real_SDL_GL_DestroyContext(context);
  EXPECT_EQ(--contexts, 0);
  // GPU resources are implicitly reclaimed by a lost/destroyed context.
  gpu_objects.clear();
  return true;
}
extern "C" SDL_FunctionPointer __real_SDL_GL_GetProcAddress(const char *);
extern "C" SDL_FunctionPointer __wrap_SDL_GL_GetProcAddress(const char *name) {
  if (real_driver)
    return __real_SDL_GL_GetProcAddress(name);
  ++lookup_calls;
  EXPECT_NE(current_context, nullptr);
  if (failure == Failure::Proc && std::strcmp(name, "glGetUniformLocation") == 0)
    return nullptr;
#define SPECIAL_GL(name_, stub)                                                                                        \
  if (std::strcmp(name, #name_) == 0)                                                                                  \
  return reinterpret_cast<SDL_FunctionPointer>(&stub)
  SPECIAL_GL(glCreateShader, CreateShader);
  SPECIAL_GL(glCreateProgram, CreateProgram);
  SPECIAL_GL(glGetShaderiv, ShaderStatus);
  SPECIAL_GL(glGetProgramiv, ProgramStatus);
  SPECIAL_GL(glGetShaderInfoLog, InfoLog);
  SPECIAL_GL(glGetProgramInfoLog, InfoLog);
  SPECIAL_GL(glGetIntegerv, GetInteger);
  SPECIAL_GL(glGetString, GetString);
  SPECIAL_GL(glGetStringi, GetStringIndex);
  SPECIAL_GL(glCheckFramebufferStatus, FramebufferStatus);
  SPECIAL_GL(glGetUniformLocation, UniformLocation);
  SPECIAL_GL(glMapBufferRange, MapBuffer);
  SPECIAL_GL(glGenTextures, GenObjects);
  SPECIAL_GL(glGenBuffers, GenObjects);
  SPECIAL_GL(glGenVertexArrays, GenObjects);
  SPECIAL_GL(glGenFramebuffers, GenObjects);
  SPECIAL_GL(glGenRenderbuffers, GenObjects);
  SPECIAL_GL(glDeleteTextures, DeleteObjects);
  SPECIAL_GL(glDeleteBuffers, DeleteObjects);
  SPECIAL_GL(glDeleteVertexArrays, DeleteObjects);
  SPECIAL_GL(glDeleteFramebuffers, DeleteObjects);
  SPECIAL_GL(glDeleteRenderbuffers, DeleteObjects);
  SPECIAL_GL(glDeleteShader, DeleteObject);
  SPECIAL_GL(glDeleteProgram, DeleteObject);
#undef SPECIAL_GL
#define GL_STUB(name_)                                                                                                 \
  if (std::strcmp(name, #name_) == 0)                                                                                  \
    return reinterpret_cast<SDL_FunctionPointer>(&GLStub<decltype(name_)>::Invoke);
#include "test_gl_entries.h"
#undef GL_STUB
  ADD_FAILURE() << "Unknown GL entry point: " << name;
  return nullptr;
}
extern "C" bool __real_SDL_GetWindowSizeInPixels(SDL_Window *, int *, int *);
extern "C" bool __wrap_SDL_GetWindowSizeInPixels(SDL_Window *window, int *w, int *h) {
  if (real_driver)
    return __real_SDL_GetWindowSizeInPixels(window, w, h);
  *w = 640;
  *h = 480;
  return failure != Failure::Drawable;
}
extern "C" bool __real_SDL_SetWindowSize(SDL_Window *, int, int);
extern "C" bool __wrap_SDL_SetWindowSize(SDL_Window *window, int w, int h) {
  return real_driver ? __real_SDL_SetWindowSize(window, w, h) : true;
}
extern "C" bool __real_SDL_SetWindowRelativeMouseMode(SDL_Window *, bool);
extern "C" bool __wrap_SDL_SetWindowRelativeMouseMode(SDL_Window *window, bool enabled) {
  return real_driver ? __real_SDL_SetWindowRelativeMouseMode(window, enabled) : true;
}
extern "C" bool __real_SDL_SetWindowFullscreen(SDL_Window *, bool);
extern "C" bool __wrap_SDL_SetWindowFullscreen(SDL_Window *window, bool enabled) {
  return real_driver ? __real_SDL_SetWindowFullscreen(window, enabled) : true;
}
extern "C" bool __real_SDL_SyncWindow(SDL_Window *);
extern "C" bool __wrap_SDL_SyncWindow(SDL_Window *window) { return real_driver ? __real_SDL_SyncWindow(window) : true; }
extern "C" bool __real_SDL_GL_SwapWindow(SDL_Window *);
extern "C" bool __wrap_SDL_GL_SwapWindow(SDL_Window *window) {
  return real_driver ? __real_SDL_GL_SwapWindow(window) : true;
}

namespace {
renderer_preferred_state Preferences() {
  renderer_preferred_state pref{};
  pref.width = 640;
  pref.height = 480;
  pref.bit_depth = 32;
  pref.gamma = 1.5f;
  return pref;
}

class RendererLifecycleTest : public testing::Test {
protected:
  void SetUp() override {
    failure = Failure::None;
    real_driver = false;
    rend_Close();
    video_initialized = true;
    packed_pixels = false;
    error_calls = bad_gl_calls = gl_calls = lookup_calls = 0;
    next_id = 1;
    watch_allocations = false;
    fail_allocation_after = -1;
    Current_video_resolution_id = 0;
    driver_error = "injected renderer failure %s";
  }
  void TearDown() override {
    watch_allocations = false;
    rend_Close();
    rend_Close();
    ExpectClosed();
    EXPECT_EQ(error_calls, 0);
  }
  void ExpectClosed() {
    EXPECT_EQ(gpu_state.initted, 0);
    EXPECT_EQ(Renderer_initted, 0);
    EXPECT_EQ(GSDLWindow, nullptr);
    EXPECT_EQ(GSDLGLContext, nullptr);
    EXPECT_EQ(windows, 0);
    EXPECT_EQ(contexts, 0);
    EXPECT_EQ(library_refs, 0);
    EXPECT_EQ(bad_gl_calls, 0);
    EXPECT_TRUE(gpu_objects.empty());
    EXPECT_TRUE(std::all_of(cpu_objects.begin(), cpu_objects.end(), [](void *p) { return p == nullptr; }));
  }
  void ExpectReadyAndDraw() {
    ASSERT_EQ(gpu_state.initted, 1);
    ASSERT_EQ(Renderer_initted, 1);
    ASSERT_NE(GSDLGLContext, nullptr);
    const int calls = gl_calls;
    rend_SetGammaValue(1.75f);
    rend_StartFrame(0, 0, 640, 480, 0);
    rend_SetAlphaType(AT_CONSTANT);
    rend_SetPixel(GR_RGB(255, 255, 255), 10, 10);
    EXPECT_GT(gl_calls, calls);
    EXPECT_EQ(bad_gl_calls, 0);
  }
};

TEST_F(RendererLifecycleTest, SuccessReloadsFunctionsOnReinitAndAfterClose) {
  auto pref = Preferences();
  ASSERT_EQ(rend_Init(RENDERER_OPENGL, nullptr, &pref), 1);
  ExpectReadyAndDraw();
  const int first_lookups = lookup_calls;
  EXPECT_GT(first_lookups, 0);
  pref.gamma = 2.0f;
  ASSERT_EQ(rend_SetPreferredState(&pref, true), 1);
  EXPECT_GT(lookup_calls, first_lookups);
  ExpectReadyAndDraw();
  rend_Close();
  ExpectClosed();
  ASSERT_EQ(rend_Init(RENDERER_OPENGL, nullptr, &pref), 1);
  ExpectReadyAndDraw();
}

TEST_F(RendererLifecycleTest, VideoInitializationUsesSDL3BooleanResult) {
  video_initialized = false;
  auto pref = Preferences();
  ASSERT_EQ(rend_Init(RENDERER_OPENGL, nullptr, &pref), 1);
  ExpectReadyAndDraw();
}

class RendererFailureTest : public RendererLifecycleTest, public testing::WithParamInterface<Failure> {};
TEST_P(RendererFailureTest, FailedStartupCleansUpAndCanRetry) {
  auto pref = Preferences();
  failure = GetParam();
  if (failure == Failure::Video)
    video_initialized = false;
  ASSERT_EQ(rend_Init(RENDERER_OPENGL, nullptr, &pref), 0);
  EXPECT_NE(std::strlen(rend_GetErrorMessage()), 0u);
  EXPECT_LT(std::strlen(rend_GetErrorMessage()), 256u);
  ExpectClosed();
  failure = Failure::None;
  ASSERT_EQ(rend_Init(RENDERER_OPENGL, nullptr, &pref), 1);
  ExpectReadyAndDraw();
}
class RendererReinitFailureTest : public RendererLifecycleTest, public testing::WithParamInterface<Failure> {};
TEST_P(RendererReinitFailureTest, FailedReinitWithGammaChangeReturnsFailureAndCanRetry) {
  auto pref = Preferences();
  ASSERT_EQ(rend_Init(RENDERER_OPENGL, nullptr, &pref), 1);
  failure = GetParam();
  pref.gamma = 2.25f;
  pref.width = 800;
  ASSERT_EQ(rend_SetPreferredState(&pref, true), 0);
  ExpectClosed();
  failure = Failure::None;
  ASSERT_EQ(rend_Init(RENDERER_OPENGL, nullptr, &pref), 1);
  ExpectReadyAndDraw();
}
INSTANTIATE_TEST_SUITE_P(Injected, RendererFailureTest,
                         testing::Values(Failure::Video, Failure::Library, Failure::Attributes, Failure::Window,
                                         Failure::Context, Failure::Current, Failure::Proc, Failure::Drawable,
                                         Failure::Framebuffer, Failure::ShaderCreate, Failure::ShaderCompile,
                                         Failure::ProgramLink, Failure::Uniform));
INSTANTIATE_TEST_SUITE_P(Injected, RendererReinitFailureTest,
                         testing::Values(Failure::Attributes, Failure::Context, Failure::Current, Failure::Proc,
                                         Failure::Drawable, Failure::Framebuffer, Failure::ShaderCreate,
                                         Failure::ShaderCompile, Failure::ProgramLink, Failure::Uniform));

TEST_F(RendererLifecycleTest, EveryCacheAndUploadAllocationFailureIsRolledBack) {
  for (bool packed : {false, true}) {
    packed_pixels = packed;
    for (bool reinitialize : {false, true}) {
      for (int allocation = 0; allocation < 7; ++allocation) {
        SCOPED_TRACE(allocation);
        SCOPED_TRACE(reinitialize);
        auto pref = Preferences();
        if (reinitialize) {
          ASSERT_EQ(rend_Init(RENDERER_OPENGL, nullptr, &pref), 1);
        }
        watch_allocations = true;
        fail_allocation_after = allocation;
        pref.gamma = 2.0f;
        const int initialized =
            reinitialize ? rend_SetPreferredState(&pref, true) : rend_Init(RENDERER_OPENGL, nullptr, &pref);
        watch_allocations = false;
        EXPECT_EQ(fail_allocation_after, -1);
        ASSERT_EQ(initialized, 0);
        ExpectClosed();
        ASSERT_EQ(rend_Init(RENDERER_OPENGL, nullptr, &pref), 1);
        ExpectReadyAndDraw();
        rend_Close();
      }
    }
  }
}

TEST_F(RendererLifecycleTest, LostContextDuringCloseDoesNotCallGLAndCanRetry) {
  auto pref = Preferences();
  ASSERT_EQ(rend_Init(RENDERER_OPENGL, nullptr, &pref), 1);
  const int calls = gl_calls;
  failure = Failure::Current;
  rend_Close();
  EXPECT_EQ(gl_calls, calls);
  ExpectClosed();
  failure = Failure::None;
  ASSERT_EQ(rend_Init(RENDERER_OPENGL, nullptr, &pref), 1);
  ExpectReadyAndDraw();
}

TEST_F(RendererLifecycleTest, InvalidDimensionsAndSelectionAreRejectedBeforeGL) {
  auto pref = Preferences();
  pref.width = 0;
  EXPECT_EQ(rend_Init(RENDERER_OPENGL, nullptr, &pref), 0);
  ExpectClosed();
  pref = Preferences();
  Current_video_resolution_id = -1;
  EXPECT_EQ(rend_Init(RENDERER_OPENGL, nullptr, &pref), 0);
  ExpectClosed();
  EXPECT_EQ(gl_calls, 0);
}

TEST_F(RendererLifecycleTest, LongDriverErrorsAreBoundedAndPreservePercentCharacters) {
  driver_error = std::string(1024, '%');
  failure = Failure::Context;
  auto pref = Preferences();
  EXPECT_EQ(rend_Init(RENDERER_OPENGL, nullptr, &pref), 0);
  EXPECT_EQ(std::strlen(rend_GetErrorMessage()), 255u);
  EXPECT_NE(std::strchr(rend_GetErrorMessage(), '%'), nullptr);
}

TEST_F(RendererLifecycleTest, FatalFailureCannotResumeAfterReturningErrorHandler) {
  EXPECT_EXIT(rend_FatalError(), testing::ExitedWithCode(EXIT_FAILURE), "");
}

TEST(RendererRealGL, DrawsAfterReinitializationAndReopening) {
  if (!std::getenv("D3_TEST_REAL_GL"))
    GTEST_SKIP() << "Opt in with D3_TEST_REAL_GL=1 and a working SDL GL video driver.";
  real_driver = true;
  struct Cleanup {
    ~Cleanup() {
      rend_Close();
      SDL_Quit();
      real_driver = false;
    }
  } cleanup;
  ASSERT_TRUE(SDL_Init(SDL_INIT_VIDEO)) << SDL_GetError();
  auto pref = Preferences();
  for (int round = 0; round < 3; ++round) {
    SCOPED_TRACE(round);
    ASSERT_EQ(rend_Init(RENDERER_OPENGL, nullptr, &pref), 1) << rend_GetErrorMessage();
    ASSERT_EQ(rend_SetPreferredState(&pref, true), 1) << rend_GetErrorMessage();
    rend_StartFrame(0, 0, 640, 480, 0);
    rend_ClearScreen(GR_RGB(0, 0, 0));
    rend_SetAlphaType(AT_CONSTANT);
    rend_SetPixel(GR_RGB(255, 255, 255), 10, 10);
    auto get_error = reinterpret_cast<GLenum (*)()>(SDL_GL_GetProcAddress("glGetError"));
    auto read_pixels = reinterpret_cast<decltype(&glReadPixels)>(SDL_GL_GetProcAddress("glReadPixels"));
    std::vector<unsigned char> pixels(640 * 480 * 4);
    read_pixels(0, 0, 640, 480, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    ASSERT_EQ(get_error(), GL_NO_ERROR);
    bool lit_pixel = false;
    for (size_t i = 0; i < pixels.size(); i += 4)
      lit_pixel |= pixels[i] != 0 || pixels[i + 1] != 0 || pixels[i + 2] != 0;
    EXPECT_TRUE(lit_pixel);
    rend_Close();
    EXPECT_EQ(gpu_state.initted, 0);
    EXPECT_EQ(Renderer_initted, 0);
  }
}
} // namespace
