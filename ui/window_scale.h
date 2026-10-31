/*
 * Descent 3
 * Copyright (C) 2026 Descent Developers
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef D3_UI_WINDOW_SCALE_H
#define D3_UI_WINDOW_SCALE_H

#include <algorithm>
#include <cmath>

#include "grdefs.h"

namespace ui_scale {
inline float ValidScale(float scale) { return std::isfinite(scale) && scale > 0.0f ? scale : 1.0f; }

// Legacy message-box artwork and layouts were authored for a 640x480 screen.
// Fit that coordinate space uniformly so widescreen displays do not stretch it.
inline float PopupScale(int width, int height) {
  return std::max(
      1.0f, std::min(static_cast<float>(width) / FIXED_SCREEN_WIDTH, static_cast<float>(height) / FIXED_SCREEN_HEIGHT));
}

inline int ToLogical(int coordinate, float scale) {
  return static_cast<int>(std::floor(static_cast<float>(coordinate) / ValidScale(scale)));
}
} // namespace ui_scale

#endif
