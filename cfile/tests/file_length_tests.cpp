/*
 * Descent 3
 * Copyright (C) 2026 Descent Developers
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cfile.h"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>

namespace {
class CFileLengthTest : public testing::Test {
protected:
  void SetUp() override {
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    path_ = std::filesystem::temp_directory_path() / ("descent3-file-length-" + std::to_string(stamp));
    std::ofstream file(path_, std::ios::binary);
    file << "hello";
    ASSERT_TRUE(file.good());
  }
  void TearDown() override {
    std::error_code ec;
    std::filesystem::remove(path_, ec);
  }
  std::filesystem::path path_;
};

TEST_F(CFileLengthTest, NormalFileRetainsItsSizeAndContents) {
  std::unique_ptr<CFILE, decltype(&cfclose)> file(cfopen(path_, "rb"), cfclose);
  ASSERT_NE(file, nullptr);
  EXPECT_EQ(cfilelength(file.get()), 5u);
  char bytes[6]{};
  EXPECT_EQ(cf_ReadBytes(reinterpret_cast<uint8_t *>(bytes), 5, file.get()), 5);
  EXPECT_STREQ(bytes, "hello");
}

#ifdef POSIX
TEST_F(CFileLengthTest, LargestSupportedSparseFileOpensWithoutNarrowing) {
  std::filesystem::resize_file(path_, std::numeric_limits<int>::max());
  std::unique_ptr<CFILE, decltype(&cfclose)> file(cfopen(path_, "rb"), cfclose);
  ASSERT_NE(file, nullptr);
  EXPECT_EQ(cfilelength(file.get()), static_cast<uint32_t>(std::numeric_limits<int>::max()));
}

TEST_F(CFileLengthTest, OversizedSparseFilesAreRejectedInsteadOfPublishingInvalidSizes) {
  for (uint64_t size : {static_cast<uint64_t>(std::numeric_limits<int>::max()) + 1, (uint64_t{1} << 32) + 17}) {
    std::filesystem::resize_file(path_, size);
    std::unique_ptr<CFILE, decltype(&cfclose)> file(cfopen(path_, "rb"), cfclose);
    EXPECT_EQ(file, nullptr);
  }
}
#endif
} // namespace
