/*
 * Descent 3
 * Copyright (C) 2026 Descent Developers
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef D3_RENDER_DIMENSIONS_H
#define D3_RENDER_DIMENSIONS_H
#include <algorithm>
#include <cmath>

namespace render_dimensions {
struct Size {
  int width = 0, height = 0;
};

inline Size Fit(Size source, Size destination) {
  if (source.width <= 0 || source.height <= 0 || destination.width <= 0 || destination.height <= 0)
    return {};
  const double scale = std::min(static_cast<double>(destination.width) / source.width,
                                static_cast<double>(destination.height) / source.height);
  return {std::max(1, static_cast<int>(std::round(source.width * scale))),
          std::max(1, static_cast<int>(std::round(source.height * scale)))};
}

inline Size Buffer(Size logical, Size drawable, bool native_ui) {
  if (!native_ui)
    return logical;
  const auto fit = Fit(logical, drawable);
  return {std::max(logical.width, fit.width), std::max(logical.height, fit.height)};
}
} // namespace render_dimensions
#endif
