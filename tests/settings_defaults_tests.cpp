/*
 * Descent 3
 * Copyright (C) 2026 Descent Developers
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "settings_defaults.h"
#include <gtest/gtest.h>

TEST(SettingsDefaults, ChoosesHighestPixelCountWithoutSavedChoice) {
  const std::vector<tVideoResolution> modes{{3840, 1080}, {1280, 720}, {2560, 2160}, {1920, 1080}};
  EXPECT_EQ(SelectStartupResolution(modes), 2);
}

TEST(SettingsDefaults, RetainsExplicitDimensionsAfterModeListChanges) {
  const std::vector<tVideoResolution> modes{{3840, 2160}, {1280, 720}, {1920, 1080}};
  EXPECT_EQ(SelectStartupResolution(modes, 1280, 720), 1);
  EXPECT_EQ(SelectStartupResolution(modes, 1600, 900), 0);
  EXPECT_EQ(SelectStartupResolution({}), -1);
}

TEST(SettingsDefaults, MaximumDetailEnablesQualityAndFullGeometry) {
  const auto detail = MaximumDetailSettings(32);
  EXPECT_FLOAT_EQ(detail.Terrain_render_distance, 200 * 32);
  EXPECT_FLOAT_EQ(detail.Pixel_error, 4);
  EXPECT_EQ(detail.Object_complexity, 2);
  EXPECT_TRUE(detail.Specular_lighting);
  EXPECT_TRUE(detail.Dynamic_lighting);
  EXPECT_TRUE(detail.Mirrored_surfaces);
  EXPECT_TRUE(detail.Fog_enabled);
  EXPECT_TRUE(detail.Coronas_enabled);
  EXPECT_TRUE(detail.Procedurals_enabled);
  EXPECT_TRUE(detail.Powerup_halos);
  EXPECT_TRUE(detail.Scorches_enabled);
  EXPECT_TRUE(detail.Weapon_coronas_enabled);
  EXPECT_FALSE(detail.Fast_headlight_on);
}
