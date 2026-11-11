/*
 * Descent 3
 * Copyright (C) 2026 Descent Developers
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

// pcx.cpp is compiled directly with a minimal bitmap allocator so malformed
// files can be exercised without the game's bitmap system.
#include "bitmap.h"
#include "cfile.h"
#include "grdefs.h"
#include "iff.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <vector>

namespace {
std::map<int, std::vector<uint16_t>> g_bitmaps;
int g_next_handle = 1;
bool g_fail_alloc = false;
} // namespace

int bm_AllocBitmap(int w, int h, int) {
  if (g_fail_alloc)
    return -1;
  g_bitmaps[g_next_handle] = std::vector<uint16_t>(static_cast<size_t>(w) * h);
  return g_next_handle++;
}
uint16_t *bm_data(int handle, int) { return g_bitmaps.at(handle).data(); }

namespace {

using Bytes = std::vector<uint8_t>;

void Put16(Bytes &b, size_t off, int v) {
  b[off] = static_cast<uint8_t>(v);
  b[off + 1] = static_cast<uint8_t>(v >> 8);
}

Bytes Header(int w, int h, int planes, int bytes_per_line) {
  Bytes b(128, 0);
  b[0] = 10; // manufacturer
  b[1] = 5;  // version
  b[2] = 1;  // RLE
  b[3] = 8;  // bits per pixel per plane
  Put16(b, 4, 0);
  Put16(b, 6, 0);
  Put16(b, 8, w - 1);
  Put16(b, 10, h - 1);
  b[65] = static_cast<uint8_t>(planes);
  Put16(b, 66, bytes_per_line);
  return b;
}

class PcxLoaderTest : public ::testing::Test {
protected:
  void SetUp() override {
    g_bitmaps.clear();
    g_fail_alloc = false;
    dir_ = std::filesystem::temp_directory_path() /
           ("d3_pcx_" + std::to_string(::testing::UnitTest::GetInstance()->random_seed()) + "_" +
            ::testing::UnitTest::GetInstance()->current_test_info()->name());
    std::filesystem::create_directories(dir_);
  }
  void TearDown() override { std::filesystem::remove_all(dir_); }

  // Returns the loader result; a cfile_error thrown on truncation counts as -2.
  int Load(const Bytes &bytes) {
    auto path = dir_ / "image.pcx";
    std::ofstream(path, std::ios::binary).write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
    CFILE *f = cfopen(path, "rb");
    EXPECT_NE(f, nullptr);
    int result;
    try {
      result = bm_pcx_alloc_file(f);
    } catch (cfile_error *) {
      result = -2;
    }
    cfclose(f);
    return result;
  }

  std::filesystem::path dir_;
};

Bytes Palette8(Bytes b) {
  b.push_back(12); // palette marker / pad byte
  for (int i = 0; i < 256; i++) {
    b.push_back(static_cast<uint8_t>(i));
    b.push_back(static_cast<uint8_t>(255 - i));
    b.push_back(0x80);
  }
  return b;
}

} // namespace

TEST_F(PcxLoaderTest, Decodes8BitImageWithRunsAndPalette) {
  Bytes b = Header(2, 2, 1, 2);
  b.insert(b.end(), {0xC3, 5, 0}); // run of three 5s, then literal index 0 (transparent)
  b = Palette8(b);
  int h = Load(b);
  ASSERT_GT(h, 0);
  const auto &px = g_bitmaps.at(h);
  const uint16_t five = OPAQUE_FLAG | ((5 >> 3) << 10) | ((250 >> 3) << 5) | (0x80 >> 3);
  EXPECT_EQ(px[0], five);
  EXPECT_EQ(px[2], five);
  EXPECT_EQ(px[3], NEW_TRANSPARENT_COLOR);
}

TEST_F(PcxLoaderTest, Decodes24BitImagePerPlane) {
  Bytes b = Header(2, 1, 3, 2);
  b.insert(b.end(), {0xC2, 0xF8, 0x08, 0x10, 0xC2, 0x00}); // R: F8 F8, G: 08 10, B: 00 00
  int h = Load(b);
  ASSERT_GT(h, 0);
  const auto &px = g_bitmaps.at(h);
  EXPECT_EQ(px[0], OPAQUE_FLAG | GR_RGB16(0xF8, 0x08, 0x00));
  EXPECT_EQ(px[1], OPAQUE_FLAG | GR_RGB16(0xF8, 0x10, 0x00));
}

TEST_F(PcxLoaderTest, OverlongRunsAreTruncatedNotWrittenPastTheImage) {
  Bytes b8 = Header(2, 2, 1, 2);
  b8.insert(b8.end(), {0xFF, 7}); // a 63-byte run for a 4-byte image
  EXPECT_GT(Load(Palette8(b8)), 0);

  Bytes b24 = Header(2, 1, 3, 2);
  b24.insert(b24.end(), {0xFF, 1, 0xFF, 2, 0xFF, 3}); // 63-byte runs for 2-byte scanlines
  int h = Load(b24);
  ASSERT_GT(h, 0);
  EXPECT_EQ(g_bitmaps.at(h)[1], OPAQUE_FLAG | GR_RGB16(1, 2, 3));
}

TEST_F(PcxLoaderTest, RejectsInvalidDimensionsAndScanlines) {
  Bytes inverted = Header(2, 2, 1, 2);
  Put16(inverted, 8, 0);
  Put16(inverted, 4, 5); // xmax < xmin
  EXPECT_EQ(Load(Palette8(inverted)), -1);
  EXPECT_EQ(Load(Palette8(Header(5000, 1, 1, 5000))), -1);
  EXPECT_EQ(Load(Header(4, 1, 3, 2)), -1); // bytes per line below width
  EXPECT_TRUE(g_bitmaps.empty());
}

TEST_F(PcxLoaderTest, TruncatedDataAndAllocationFailureDoNotLeak) {
  Bytes truncated = Header(16, 16, 3, 16);
  truncated.insert(truncated.end(), {0xC4, 1}); // far less data than the image needs
  EXPECT_EQ(Load(truncated), -1);
  Bytes truncated8 = Header(4, 4, 1, 4);
  truncated8.insert(truncated8.end(), {0xC0, 0xC0, 0xC0}); // zero-length runs, then EOF
  EXPECT_EQ(Load(truncated8), -1);

  Bytes ok = Header(2, 1, 3, 2);
  ok.insert(ok.end(), {0xC2, 1, 0xC2, 2, 0xC2, 3});
  g_fail_alloc = true;
  EXPECT_EQ(Load(ok), -1);
}
