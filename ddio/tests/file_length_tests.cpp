/*
 * Descent 3
 * Copyright (C) 2026 Descent Developers
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ddio.h"

#include <gtest/gtest.h>

#include <cstdio>
#include <cstdint>
#include <limits>
#include <memory>

#ifdef POSIX
#include <unistd.h>
#endif

namespace {
struct CloseFile {
  void operator()(FILE *file) const { std::fclose(file); }
};
using File = std::unique_ptr<FILE, CloseFile>;

TEST(FileLengthTest, NullFileIsRejected) { EXPECT_EQ(ddio_GetFileLength(nullptr), -1); }

TEST(FileLengthTest, EmptyAndNormalFilesReportExactLength) {
  File file(std::tmpfile());
  ASSERT_NE(file, nullptr);
  EXPECT_EQ(ddio_GetFileLength(file.get()), 0);
  ASSERT_EQ(std::fwrite("hello", 1, 5, file.get()), 5u);
  ASSERT_EQ(std::fflush(file.get()), 0);
  ASSERT_EQ(std::fseek(file.get(), 2, SEEK_SET), 0);
  EXPECT_EQ(ddio_GetFileLength(file.get()), 5);
  EXPECT_EQ(std::ftell(file.get()), 2);
}

#ifdef POSIX
TEST(FileLengthTest, FailedStatReturnsAnError) {
  File file(std::tmpfile());
  ASSERT_NE(file, nullptr);
  ASSERT_EQ(close(fileno(file.get())), 0);
  const int length = ddio_GetFileLength(file.get());
  // Release the stdio object before assertions or any other code can reuse fd.
  file.reset();
  EXPECT_EQ(length, -1);
}

TEST(FileLengthTest, LargestRepresentableSparseFileIsAccepted) {
  File file(std::tmpfile());
  ASSERT_NE(file, nullptr);
  ASSERT_EQ(ftruncate(fileno(file.get()), std::numeric_limits<int>::max()), 0);
  EXPECT_EQ(ddio_GetFileLength(file.get()), std::numeric_limits<int>::max());
}

TEST(FileLengthTest, OversizedSparseFilesCannotWrapToNegativeOrSmallLengths) {
  if (sizeof(off_t) <= sizeof(int))
    GTEST_SKIP() << "Large sparse fixtures require a wider off_t";
  File file(std::tmpfile());
  ASSERT_NE(file, nullptr);
  for (uint64_t size : {static_cast<uint64_t>(std::numeric_limits<int>::max()) + 1, (uint64_t{1} << 32) + 17}) {
    ASSERT_EQ(ftruncate(fileno(file.get()), static_cast<off_t>(size)), 0);
    EXPECT_EQ(ddio_GetFileLength(file.get()), -1);
  }
}
#endif
} // namespace
