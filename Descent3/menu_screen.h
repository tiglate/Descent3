#pragma once

#include <algorithm>

// Keep the original logical height, expanding only when the display is wider.
inline int MenuScreenWidth(int width, int height, bool modern) {
  if (!modern || width <= 0 || height <= 0)
    return 640;
  return std::max(640, static_cast<int>(480LL * width / height));
}
