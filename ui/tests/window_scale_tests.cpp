/*
 * Descent 3
 * Copyright (C) 2026 Descent Developers
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "window_scale.h"

#include <gtest/gtest.h>

#include <limits>
#include <utility>

TEST(UIPopupScale, ReferenceAndSmallerScreensKeepOriginalSize) {
  EXPECT_FLOAT_EQ(ui_scale::PopupScale(1024, 768), 1.0f);
  EXPECT_FLOAT_EQ(ui_scale::PopupScale(800, 600), 1.0f);
  EXPECT_FLOAT_EQ(ui_scale::PopupScale(640, 480), 1.0f);
  EXPECT_FLOAT_EQ(ui_scale::PopupScale(854, 480), 1.0f); // widescreen menu mode
  EXPECT_FLOAT_EQ(ui_scale::PopupScale(1280, 720), 1.0f);
  EXPECT_FLOAT_EQ(ui_scale::PopupScale(0, 0), 1.0f);
}

TEST(UIPopupScale, HighResolutionKeeps1024x768Proportions) {
  EXPECT_FLOAT_EQ(ui_scale::PopupScale(1920, 1080), 1.40625f);
  EXPECT_FLOAT_EQ(ui_scale::PopupScale(2560, 1440), 1.875f);
  EXPECT_FLOAT_EQ(ui_scale::PopupScale(3840, 2160), 2.8125f);
  EXPECT_FLOAT_EQ(ui_scale::PopupScale(2048, 1536), 2.0f);
}

TEST(UIPopupScale, UltrawideDoesNotStretchOrOverEnlargePopups) {
  EXPECT_FLOAT_EQ(ui_scale::PopupScale(2560, 1080), ui_scale::PopupScale(1920, 1080));
  EXPECT_FLOAT_EQ(ui_scale::PopupScale(1280, 1024), 1.25f);
  EXPECT_FLOAT_EQ(ui_scale::PopupScale(1200, 1920), 1.171875f);
}

TEST(UIPopupScale, LegacyLayoutsAlwaysFitTheLogicalScreen) {
  for (const auto &resolution :
       {std::pair{640, 480}, std::pair{1024, 768}, std::pair{1280, 720}, std::pair{1280, 1024}, std::pair{1920, 1080},
        std::pair{2560, 1080}, std::pair{3840, 2160}, std::pair{7680, 4320}}) {
    const float scale = ui_scale::PopupScale(resolution.first, resolution.second);
    EXPECT_GE(ui_scale::ToLogical(resolution.first, scale), 640);
    EXPECT_GE(ui_scale::ToLogical(resolution.second, scale), 480);
  }
}

TEST(UIMenuScale, CursorSpeedFollowsStretchedMenus) {
  EXPECT_FLOAT_EQ(ui_scale::MenuScale(640, 480), 1.0f);
  EXPECT_FLOAT_EQ(ui_scale::MenuScale(1920, 1080), 2.25f);
  EXPECT_FLOAT_EQ(ui_scale::MenuScale(3840, 2160), 4.5f);
  EXPECT_FLOAT_EQ(ui_scale::MenuScale(320, 240), 1.0f);
}

TEST(UIPopupScale, InvalidDrawingScaleFallsBackToPixels) {
  EXPECT_FLOAT_EQ(ui_scale::ValidScale(0), 1.0f);
  EXPECT_FLOAT_EQ(ui_scale::ValidScale(-1), 1.0f);
  EXPECT_FLOAT_EQ(ui_scale::ValidScale(std::numeric_limits<float>::infinity()), 1.0f);
  EXPECT_FLOAT_EQ(ui_scale::ValidScale(std::numeric_limits<float>::quiet_NaN()), 1.0f);
}

TEST(UIPopupScale, MouseCoordinatesMatchVisibleButtonBounds) {
  const float scale = ui_scale::PopupScale(1920, 1080);
  constexpr int left = 800, top = 400, width = 64, height = 16; // pixels 1125-1214 x 563-584
  const auto hit = [&](int px, int py) {
    const int x = ui_scale::ToLogical(px, scale), y = ui_scale::ToLogical(py, scale);
    return x >= left && x < left + width && y >= top && y < top + height;
  };
  EXPECT_TRUE(hit(1170, 570));
  EXPECT_FALSE(hit(1124, 570));
  EXPECT_FALSE(hit(1215, 570));
  EXPECT_FALSE(hit(1170, 561));
  EXPECT_FALSE(hit(1170, 585));
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
  EXPECT_EQ(ui_scale::ToLogical(-1, 1.40625f), -1);
  EXPECT_EQ(ui_scale::ToLogical(100, 1.0f), 100);
}
