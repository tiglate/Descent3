/*
 * Descent 3
 * Copyright (C) 2026 Descent Developers
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef D3_SCALABLE_FONT_H
#define D3_SCALABLE_FONT_H

#include <array>
#include <cstdint>
#include <vector>

namespace popup_font {
enum class Face { UI, MainMenu };

struct Glyph {
  int x = 0, y = 0, width = 0, height = 0;
  bool present = false;
};

struct Atlas {
  int side = 0;
  std::array<Glyph, 256> glyphs{};
  std::vector<uint16_t> pixels; // white ARGB4444 with antialiased coverage
};

// Use the original font's character cells, spacing and line height. Only the
// glyph images are replaced, so existing layout and clipping remain valid.
// Inputs use the engine's existing single-byte Windows-1252 character set.
Atlas Rasterize(const std::array<int, 256> &widths, int height, float screen_scale, bool uppercase = true,
                Face face = Face::UI);

// Quantize cache sizes by raster line height, including HUD/text scaling.
int RasterHeight(int height, float text_scale_x, float text_scale_y, float pixel_scale);
} // namespace popup_font

#endif
