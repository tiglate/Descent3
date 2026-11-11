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

// Reference display for in-game dialogs. The original game drew its dialogs
// 1:1 over gameplay; at the then-common 1024x768 they covered a comfortable
// share of the screen. Scale them uniformly so they keep that proportion at any
// resolution (1080p: 1.40625x, 4K: 2.8125x) without stretching on widescreen.
// Screens at or below the reference, including the 640x480 menu mode, keep 1:1.
inline constexpr int kDialogReferenceWidth = 1024;
inline constexpr int kDialogReferenceHeight = 768;

inline float PopupScale(int width, int height) {
  return std::max(1.0f, std::min(static_cast<float>(width) / kDialogReferenceWidth,
                                 static_cast<float>(height) / kDialogReferenceHeight));
}

// Scale at which the 640x480 menu screens are stretched to fill the display.
// The UI cursor uses it so it moves as fast in-game as it does in the menus.
inline float MenuScale(int width, int height) {
  return std::max(
      1.0f, std::min(static_cast<float>(width) / FIXED_SCREEN_WIDTH, static_cast<float>(height) / FIXED_SCREEN_HEIGHT));
}

inline int ToLogical(int coordinate, float scale) {
  return static_cast<int>(std::floor(static_cast<float>(coordinate) / ValidScale(scale)));
}
} // namespace ui_scale

#endif
