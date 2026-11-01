#include "menu_image_decode.h"
#include <gtest/gtest.h>
#include <filesystem>

TEST(MenuImage, DecodesBundledArtworkAtFullColor) {
  MenuImagePixels pixels;
  ASSERT_TRUE(LoadMenuImagePixels(D3_MENU_IMAGE_PATH, pixels));
  EXPECT_EQ(pixels.width, 1672u);
  EXPECT_EQ(pixels.height, 941u);
  ASSERT_EQ(pixels.rgba.size(), size_t(pixels.width) * pixels.height * 4);
  bool has_fractional_color = false;
  bool opaque = true;
  for (size_t i = 0; i < pixels.rgba.size(); i += 4) {
    opaque &= pixels.rgba[i + 3] == 255;
    if ((pixels.rgba[i] & 7) != 0 || (pixels.rgba[i + 1] & 7) != 0 || (pixels.rgba[i + 2] & 7) != 0)
      has_fractional_color = true;
  }
  EXPECT_TRUE(opaque);
  EXPECT_TRUE(has_fractional_color);
}

TEST(MenuImage, MissingFileClearsPreviousPixels) {
  MenuImagePixels pixels{1, 1, {1, 2, 3, 4}};
  EXPECT_FALSE(LoadMenuImagePixels(std::filesystem::path(D3_MENU_IMAGE_PATH) / "missing.png", pixels));
  EXPECT_EQ(pixels.width, 0u);
  EXPECT_EQ(pixels.height, 0u);
  EXPECT_TRUE(pixels.rgba.empty());
}
