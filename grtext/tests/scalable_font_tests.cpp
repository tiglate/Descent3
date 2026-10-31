/*
 * Descent 3
 * Copyright (C) 2026 Descent Developers
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "scalable_font.h"
#include <gtest/gtest.h>
#include <algorithm>
#include <cmath>
#include <limits>

namespace {
std::array<int, 256> Widths() {
  std::array<int, 256> widths{};
  std::fill(widths.begin() + 32, widths.end(), 7);
  widths['I'] = 3;
  widths['W'] = 9;
  return widths;
}

int Ink(const popup_font::Atlas &atlas, int ch) {
  const auto &cell = atlas.glyphs[ch];
  int ink = 0;
  for (int y = 0; y < cell.height; ++y)
    for (int x = 0; x < cell.width; ++x)
      ink += (atlas.pixels[(cell.y + y) * atlas.side + cell.x + x] >> 12) > 0;
  return ink;
}
} // namespace

TEST(ScalablePopupFont, RasterizesAtDisplayResolutionWithAntialiasing) {
  const auto atlas = popup_font::Rasterize(Widths(), 11, 2.25f);
  ASSERT_GT(atlas.side, 0);
  EXPECT_EQ(atlas.glyphs['A'].height, 25);
  EXPECT_EQ(atlas.glyphs['A'].width, 16);
  EXPECT_GT(Ink(atlas, 'A'), 0);
  EXPECT_TRUE(std::any_of(atlas.pixels.begin(), atlas.pixels.end(),
                          [](uint16_t p) { return (p >> 12) > 0 && (p >> 12) < 15; }));
}

TEST(ScalablePopupFont, RasterizesNewDetailAt4KInsteadOfEnlargingOldPixels) {
  const auto hd = popup_font::Rasterize(Widths(), 11, 2.25f);
  const auto uhd = popup_font::Rasterize(Widths(), 11, 4.5f);
  ASSERT_GT(hd.side, 0);
  ASSERT_GT(uhd.side, 0);
  EXPECT_EQ(uhd.glyphs['A'].height, 50);
  EXPECT_GT(Ink(uhd, 'A'), Ink(hd, 'A') * 3);
}

TEST(ScalablePopupFont, RetainsLegacyCellsAndLeavesPaddingTransparent) {
  const auto widths = Widths();
  const auto atlas = popup_font::Rasterize(widths, 11, 4.5f);
  ASSERT_GT(atlas.side, 0);
  for (int ch = 32; ch < 256; ++ch) {
    const auto &cell = atlas.glyphs[ch];
    EXPECT_EQ(cell.width, std::ceil(widths[ch] * 4.5f));
    ASSERT_GT(cell.x, 0);
    ASSERT_GT(cell.y, 0);
    ASSERT_LT(cell.x + cell.width, atlas.side);
    ASSERT_LT(cell.y + cell.height, atlas.side);
    for (int x = cell.x - 1; x <= cell.x + cell.width; ++x) {
      EXPECT_EQ(atlas.pixels[(cell.y - 1) * atlas.side + x] >> 12, 0);
      EXPECT_EQ(atlas.pixels[(cell.y + cell.height) * atlas.side + x] >> 12, 0);
    }
  }
  EXPECT_EQ(Ink(atlas, ' '), 0);
}

TEST(ScalablePopupFont, SupportsAccentsAndWindowsPunctuation) {
  const auto atlas = popup_font::Rasterize(Widths(), 11, 2.25f);
  ASSERT_GT(atlas.side, 0);
  for (int ch : {0xe9, 0xdf, 0xfc, 0x80, 0x92, 0x97}) {
    EXPECT_TRUE(atlas.glyphs[ch].present) << ch;
    EXPECT_GT(Ink(atlas, ch), 0) << ch;
  }
  EXPECT_FALSE(atlas.glyphs[0x81].present); // undefined byte falls back to bitmap font
}

TEST(ScalablePopupFont, PreservesTheOriginalCapitalLetterShapes) {
  const auto atlas = popup_font::Rasterize(Widths(), 11, 2.25f);
  ASSERT_GT(atlas.side, 0);
  const auto &upper = atlas.glyphs['A'];
  const auto &lower = atlas.glyphs['a'];
  ASSERT_EQ(upper.width, lower.width);
  for (int y = 0; y < upper.height; ++y)
    for (int x = 0; x < upper.width; ++x)
      EXPECT_EQ(atlas.pixels[(upper.y + y) * atlas.side + upper.x + x],
                atlas.pixels[(lower.y + y) * atlas.side + lower.x + x]);
}

TEST(ScalablePopupFont, RejectsInvalidMetricsAndBoundsLargeRasterizations) {
  for (float scale : {0.0f, -1.0f, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()})
    EXPECT_EQ(popup_font::Rasterize(Widths(), 11, scale).side, 0);
  EXPECT_EQ(popup_font::Rasterize(Widths(), 0, 2.25f).side, 0);
  auto invalid = Widths();
  invalid['A'] = 256;
  EXPECT_EQ(popup_font::Rasterize(invalid, 11, 2.25f).side, 0);
  const auto atlas = popup_font::Rasterize(Widths(), 11, 10000.0f);
  ASSERT_GT(atlas.side, 0);
  EXPECT_LE(atlas.side, 2048);
  EXPECT_EQ(atlas.glyphs['A'].height, 128);
}

TEST(ScalablePopupFont, HudAndMenuScalesDetermineRasterResolution) {
  EXPECT_EQ(popup_font::RasterHeight(11, 1, 1, 2.25f), 25);
  EXPECT_EQ(popup_font::RasterHeight(18, 0.6f, 0.6f, 1), 11);
  EXPECT_EQ(popup_font::RasterHeight(18, 2, 2, 1), 36);
  EXPECT_EQ(popup_font::RasterHeight(19, 1, 1, 4.5f), 86);
  EXPECT_EQ(popup_font::RasterHeight(18, 1, 0.8f, 2), 36);
  EXPECT_EQ(popup_font::RasterHeight(18, 0, 1, 1), 0);
  EXPECT_EQ(popup_font::RasterHeight(18, 1, 1, 100000), 128);
}

TEST(ScalablePopupFont, BriefingAndChatCanRetainLowercase) {
  const auto atlas = popup_font::Rasterize(Widths(), 11, 2.25f, false);
  ASSERT_GT(atlas.side, 0);
  EXPECT_GT(Ink(atlas, 'a'), 0);
  EXPECT_NE(Ink(atlas, 'a'), Ink(atlas, 'A'));
  EXPECT_GT(Ink(atlas, 'g'), 0);
}

TEST(ScalablePopupFont, OrbitronUsesDifferentGlyphsWithoutChangingMenuLayout) {
  const auto ui = popup_font::Rasterize(Widths(), 19, 2.25f);
  const auto menu = popup_font::Rasterize(Widths(), 19, 2.25f, true, popup_font::Face::MainMenu);
  ASSERT_GT(ui.side, 0);
  ASSERT_EQ(menu.side, ui.side);
  EXPECT_NE(menu.pixels, ui.pixels);
  for (int ch = 'A'; ch <= 'Z'; ++ch) {
    EXPECT_TRUE(menu.glyphs[ch].present);
    EXPECT_GT(Ink(menu, ch), 0);
    EXPECT_EQ(menu.glyphs[ch].width, ui.glyphs[ch].width);
    EXPECT_EQ(menu.glyphs[ch].height, ui.glyphs[ch].height);
  }
  for (int ch : {0xe9, 0xfc, 0x92}) {
    EXPECT_TRUE(menu.glyphs[ch].present) << ch;
    EXPECT_GT(Ink(menu, ch), 0) << ch;
  }
  EXPECT_EQ(Ink(menu, ' '), 0);
  EXPECT_FALSE(menu.glyphs[0x81].present);
}

TEST(ScalablePopupFont, OrbitronRasterizesAt4KAndKeepsTransparentPadding) {
  const auto hd = popup_font::Rasterize(Widths(), 19, 2.25f, true, popup_font::Face::MainMenu);
  const auto uhd = popup_font::Rasterize(Widths(), 19, 4.5f, true, popup_font::Face::MainMenu);
  ASSERT_GT(hd.side, 0);
  ASSERT_GT(uhd.side, 0);
  EXPECT_GT(Ink(uhd, 'A'), Ink(hd, 'A') * 3);
  const auto &cell = uhd.glyphs['A'];
  for (int x = cell.x - 1; x <= cell.x + cell.width; ++x) {
    EXPECT_EQ(uhd.pixels[(cell.y - 1) * uhd.side + x] >> 12, 0);
    EXPECT_EQ(uhd.pixels[(cell.y + cell.height) * uhd.side + x] >> 12, 0);
  }
}
