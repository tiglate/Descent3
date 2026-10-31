/*
 * Descent 3
 * Copyright (C) 2026 Descent Developers
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cfile.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>

namespace {
enum class Failure { None, Handle, Name };
thread_local Failure failure = Failure::None;
thread_local bool watching = false;
thread_local int error_calls = 0;
thread_local int live_streams = 0;
thread_local void *watched_handle = nullptr;
} // namespace

extern "C" void *__real_malloc(size_t size);
extern "C" char *__real_strdup(const char *text);
extern "C" void __real_free(void *ptr);
extern "C" FILE *__real_fopen(const char *path, const char *mode);
extern "C" int __real_fclose(FILE *file);

extern "C" void *__wrap_malloc(size_t size) {
  if (watching && size == sizeof(CFILE)) {
    if (failure == Failure::Handle) {
      failure = Failure::None;
      return nullptr;
    }
    watched_handle = __real_malloc(size);
    return watched_handle;
  }
  return __real_malloc(size);
}

extern "C" char *__wrap_strdup(const char *text) {
  if (watching && failure == Failure::Name) {
    failure = Failure::None;
    return nullptr;
  }
  return __real_strdup(text);
}

extern "C" void __wrap_free(void *ptr) {
  if (ptr == watched_handle)
    watched_handle = nullptr;
  __real_free(ptr);
}

extern "C" FILE *__wrap_fopen(const char *path, const char *mode) {
  FILE *file = __real_fopen(path, mode);
  if (file)
    ++live_streams;
  return file;
}

extern "C" int __wrap_fclose(FILE *file) {
  --live_streams;
  return __real_fclose(file);
}

// GNU's symbol wrapper substitutes only this executable's Error() calls. It
// deliberately returns so the tests never open a debugger dialog or exit.
extern "C" void __wrap__Z5ErrorPKcz(const char *, ...) {
  if (!watching)
    std::abort();
  ++error_calls;
}

namespace {
using FileOwner = std::unique_ptr<CFILE, decltype(&cfclose)>;

class WatchAllocation {
public:
  explicit WatchAllocation(Failure next_failure) {
    failure = next_failure;
    error_calls = 0;
    watching = true;
  }
  ~WatchAllocation() {
    watching = false;
    failure = Failure::None;
  }
};

class CFileAllocationTest : public testing::Test {
protected:
  void SetUp() override {
    initial_streams_ = live_streams;
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    directory_ = std::filesystem::temp_directory_path() / ("descent3-cfile-allocation-" + std::to_string(stamp));
    ASSERT_TRUE(std::filesystem::create_directory(directory_));
    cf_ClearBaseDirectories();
    cf_AddBaseDirectory(directory_);
    disk_ = directory_ / "sample.txt";
    std::ofstream(disk_, std::ios::binary) << "hello";

    // Redistributable, one-entry HOG2: 68-byte header, 48-byte entry, 5 bytes.
    std::array<char, 121> hog{};
    const char magic[] = "HOG2";
    std::copy_n(magic, 4, hog.begin());
    hog[4] = 1;
    hog[8] = 116;
    const char name[] = "sample.txt";
    std::copy_n(name, sizeof(name), hog.begin() + 68);
    hog[108] = 5;
    std::copy_n("hello", 5, hog.begin() + 116);
    library_ = directory_ / "sample.hog";
    std::ofstream out(library_, std::ios::binary);
    out.write(hog.data(), hog.size());
    ASSERT_TRUE(out.good());
  }

  void TearDown() override {
    if (library_handle_)
      cf_CloseLibrary(library_handle_);
    cf_ClearBaseDirectories();
    EXPECT_EQ(live_streams, initial_streams_);
    EXPECT_EQ(watched_handle, nullptr);
    std::error_code ec;
    std::filesystem::remove_all(directory_, ec);
  }

  void ExpectContents(CFILE *file) {
    ASSERT_NE(file, nullptr);
    EXPECT_EQ(cfilelength(file), 5u);
    char bytes[6]{};
    EXPECT_EQ(cf_ReadBytes(reinterpret_cast<uint8_t *>(bytes), 5, file), 5);
    EXPECT_STREQ(bytes, "hello");
  }

  void CheckDiskFailure(Failure next_failure) {
    {
      WatchAllocation watch(next_failure);
      FileOwner file(cfopen(disk_, "rb"), cfclose);
      EXPECT_EQ(file, nullptr);
      EXPECT_EQ(failure, Failure::None); // Prove the requested fault was injected.
      EXPECT_EQ(error_calls, 1);
      EXPECT_EQ(watched_handle, nullptr);
      EXPECT_EQ(live_streams, initial_streams_);
    }
    FileOwner retry(cfopen(disk_, "rb"), cfclose);
    ExpectContents(retry.get());
  }

  void CheckLibraryFailure(bool explicit_library, bool cached_stream) {
    library_handle_ = cf_OpenLibrary(library_.filename());
    ASSERT_GT(library_handle_, 0);
    FileOwner held(nullptr, cfclose);
    if (!cached_stream) {
      // Keep the cached stream in use, forcing the next open to use fopen().
      held.reset(cf_OpenFileInLibrary("sample.txt", library_handle_));
      ASSERT_NE(held, nullptr);
    }
    auto open = [&]() {
      return explicit_library ? cf_OpenFileInLibrary("sample.txt", library_handle_) : cfopen("sample.txt", "rb");
    };
    {
      WatchAllocation watch(Failure::Handle);
      FileOwner file(open(), cfclose);
      EXPECT_EQ(file, nullptr);
      EXPECT_EQ(failure, Failure::None);
      EXPECT_EQ(error_calls, 1);
      EXPECT_EQ(watched_handle, nullptr);
      EXPECT_EQ(live_streams, initial_streams_ + (cached_stream ? 0 : 1));
    }
    FileOwner retry(open(), cfclose);
    ExpectContents(retry.get());
    if (held) {
      ExpectContents(held.get()); // A failed parallel open must not close this stream.
    }
  }

  int initial_streams_ = 0;
  int library_handle_ = 0;
  std::filesystem::path directory_, disk_, library_;
};

TEST_F(CFileAllocationTest, DiskHandleFailureClosesStreamAndAllowsRetry) { CheckDiskFailure(Failure::Handle); }
TEST_F(CFileAllocationTest, DiskNameFailureFreesHandleAndClosesStream) { CheckDiskFailure(Failure::Name); }
TEST_F(CFileAllocationTest, ExplicitLibraryFailureReleasesCachedStream) { CheckLibraryFailure(true, true); }
TEST_F(CFileAllocationTest, ExplicitLibraryFailureClosesReopenedStream) { CheckLibraryFailure(true, false); }
TEST_F(CFileAllocationTest, LibrarySearchFailureReleasesCachedStream) { CheckLibraryFailure(false, true); }
TEST_F(CFileAllocationTest, LibrarySearchFailureClosesReopenedStream) { CheckLibraryFailure(false, false); }

TEST_F(CFileAllocationTest, SuccessfulDiskOpenOwnsHandleUntilClose) {
  WatchAllocation watch(Failure::None);
  {
    FileOwner file(cfopen(disk_, "rb"), cfclose);
    ExpectContents(file.get());
    EXPECT_EQ(watched_handle, file.get());
    EXPECT_EQ(live_streams, initial_streams_ + 1);
  }
  EXPECT_EQ(watched_handle, nullptr);
  EXPECT_EQ(live_streams, initial_streams_);
  EXPECT_EQ(error_calls, 0);
}
} // namespace
