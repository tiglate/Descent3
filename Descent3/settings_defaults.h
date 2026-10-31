/*
 * Descent 3
 * Copyright (C) 2026 Descent Developers
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef D3_SETTINGS_DEFAULTS_H
#define D3_SETTINGS_DEFAULTS_H
#include "config.h"
#include <algorithm>

inline tDetailSettings MaximumDetailSettings(float terrain_size) {
  return {200.0f * terrain_size, 4.0f, true, true, false, true, true, true, true, true, true, true, true, 1, 2};
}

// Saved dimensions survive mode-list reordering and monitor changes. Missing
// or unavailable choices use the highest pixel count on the selected display.
inline int SelectStartupResolution(const std::vector<tVideoResolution> &modes, int saved_width = 0,
                                   int saved_height = 0) {
  if (modes.empty())
    return -1;
  const auto saved = std::find_if(modes.begin(), modes.end(), [&](const auto &mode) {
    return mode.width == saved_width && mode.height == saved_height;
  });
  if (saved != modes.end())
    return static_cast<int>(saved - modes.begin());
  const auto best = std::max_element(modes.begin(), modes.end(), [](const auto &a, const auto &b) {
    const uint32_t pixels_a = static_cast<uint32_t>(a.width) * a.height;
    const uint32_t pixels_b = static_cast<uint32_t>(b.width) * b.height;
    return pixels_a != pixels_b ? pixels_a < pixels_b : a.width < b.width;
  });
  return static_cast<int>(best - modes.begin());
}
#endif
