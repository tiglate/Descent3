/*
 * Descent 3
 * Copyright (C) 2026 Descent Developers
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "window_scale.h"

#include <gtest/gtest.h>

#include <limits>
#include <utility>

TEST(UIPopupScale, NativeAndSmallerScreensKeepOriginalSize) {
  EXPECT_FLOAT_EQ(ui_scale::PopupScale(640, 480), 1.0f);
  EXPECT_FLOAT_EQ(ui_scale::PopupScale(640, 360), 1.0f);
  EXPECT_FLOAT_EQ(ui_scale::PopupScale(320, 240), 1.0f);
  EXPECT_FLOAT_EQ(ui_scale::PopupScale(0, 0), 1.0f);
}

TEST(UIPopupScale, HighResolutionUsesUniformHeightLimitedScale) {
  EXPECT_FLOAT_EQ(ui_scale::PopupScale(1280, 720), 1.5f);
  EXPECT_FLOAT_EQ(ui_scale::PopupScale(1920, 1080), 2.25f);
  EXPECT_FLOAT_EQ(ui_scale::PopupScale(2560, 1440), 3.0f);
  EXPECT_FLOAT_EQ(ui_scale::PopupScale(3840, 2160), 4.5f);
}

TEST(UIPopupScale, UltrawideDoesNotStretchOrOverEnlargePopups) {
  EXPECT_FLOAT_EQ(ui_scale::PopupScale(2560, 1080), ui_scale::PopupScale(1920, 1080));
  EXPECT_FLOAT_EQ(ui_scale::PopupScale(1280, 960), 2.0f);
  EXPECT_FLOAT_EQ(ui_scale::PopupScale(800, 1200), 1.25f);
}

TEST(UIPopupScale, InvalidDrawingScaleFallsBackToPixels) {
  EXPECT_FLOAT_EQ(ui_scale::ValidScale(0), 1.0f);
  EXPECT_FLOAT_EQ(ui_scale::ValidScale(-1), 1.0f);
  EXPECT_FLOAT_EQ(ui_scale::ValidScale(std::numeric_limits<float>::infinity()), 1.0f);
  EXPECT_FLOAT_EQ(ui_scale::ValidScale(std::numeric_limits<float>::quiet_NaN()), 1.0f);
}

TEST(UIPopupScale, MouseCoordinatesMatchVisibleButtonBounds) {
  const float scale = ui_scale::PopupScale(1920, 1080);
  constexpr int left = 500, top = 230, width = 70, height = 16;
  const auto hit = [&](int px, int py) {
    const int x = ui_scale::ToLogical(px, scale), y = ui_scale::ToLogical(py, scale);
    return x >= left && x < left + width && y >= top && y < top + height;
  };
  EXPECT_TRUE(hit(1200, 530));
  EXPECT_FALSE(hit(1124, 530));
  EXPECT_FALSE(hit(1283, 530));
  EXPECT_FALSE(hit(1200, 517));
  EXPECT_FALSE(hit(1200, 554));
}

TEST(UIPopupScale, CenteringUsesTheSameLogicalSpaceAsDrawingAndInput) {
  for (const auto &resolution : {std::pair{640, 480}, std::pair{1920, 1080}, std::pair{3840, 2160}}) {
    const float scale = ui_scale::PopupScale(resolution.first, resolution.second);
    const int x = ui_scale::ToLogical(resolution.first, scale) / 2 - 160;
    const int y = ui_scale::ToLogical(resolution.second, scale) / 2 - 45;
    EXPECT_NEAR((x + 160) * scale, resolution.first / 2.0f, scale);
    EXPECT_NEAR((y + 45) * scale, resolution.second / 2.0f, scale);
  }
}

TEST(UIPopupScale, NegativeMouseCoordinatesStayOutsideTheWindow) {
  EXPECT_EQ(ui_scale::ToLogical(-1, 2.25f), -1);
  EXPECT_EQ(ui_scale::ToLogical(100, 1.0f), 100);
}
