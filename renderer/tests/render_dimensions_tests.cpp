/*
 * Descent 3
 * Copyright (C) 2026 Descent Developers
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "render_dimensions.h"
#include <gtest/gtest.h>

TEST(RenderDimensions, MenuGetsNativePixelsWithinLetterbox) {
  const auto hd = render_dimensions::Buffer({640, 480}, {1920, 1080}, true);
  EXPECT_EQ(hd.width, 1440);
  EXPECT_EQ(hd.height, 1080);
  const auto uhd = render_dimensions::Buffer({640, 480}, {3840, 2160}, true);
  EXPECT_EQ(uhd.width, 2880);
  EXPECT_EQ(uhd.height, 2160);
}

TEST(RenderDimensions, GameplayKeepsItsSelectedRenderResolution) {
  const auto size = render_dimensions::Buffer({1280, 720}, {3840, 2160}, false);
  EXPECT_EQ(size.width, 1280);
  EXPECT_EQ(size.height, 720);
}

TEST(RenderDimensions, FitsNarrowAndUltrawideDestinationsWithoutCropping) {
  const auto narrow = render_dimensions::Fit({1920, 1080}, {1024, 768});
  EXPECT_EQ(narrow.width, 1024);
  EXPECT_EQ(narrow.height, 576);
  const auto wide = render_dimensions::Fit({640, 480}, {2560, 1080});
  EXPECT_EQ(wide.width, 1440);
  EXPECT_EQ(wide.height, 1080);
  EXPECT_EQ(render_dimensions::Fit({0, 480}, {1920, 1080}).width, 0);
}
