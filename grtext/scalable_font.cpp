/*
 * Descent 3
 * Copyright (C) 2026 Descent Developers
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "scalable_font.h"

#include <algorithm>
#include <cmath>

#include "stb_truetype.h"

namespace popup_font {
extern const unsigned char FontData[];
extern const unsigned char OrbitronData[];

namespace {
int Codepoint(int ch, bool uppercase) {
  // The UI/string tables predate UTF-8. Keep accented Latin characters and
  // Windows punctuation instead of interpreting bytes as Unicode controls.
  constexpr int extended[] = {0x20ac, 0,      0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021, 0x02c6, 0x2030, 0x0160,
                              0x2039, 0x0152, 0,      0x017d, 0,      0,      0x2018, 0x2019, 0x201c, 0x201d, 0x2022,
                              0x2013, 0x2014, 0x02dc, 0x2122, 0x0161, 0x203a, 0x0153, 0,      0x017e, 0x0178};
  int codepoint = ch >= 128 && ch < 160 ? extended[ch - 128] : ch;
  if (!uppercase)
    return codepoint;
  // smallui.fnt draws capital shapes for lowercase letters too. Preserve that
  // convention, since its character cells were sized for those wider shapes.
  if ((codepoint >= 'a' && codepoint <= 'z') || (codepoint >= 0x00e0 && codepoint <= 0x00fe && codepoint != 0x00f7))
    codepoint -= 0x20;
  else if (codepoint == 0x00ff)
    codepoint = 0x0178;
  else if (codepoint == 0x0153 || codepoint == 0x0161 || codepoint == 0x017e)
    --codepoint;
  return codepoint;
}
} // namespace

int RasterHeight(int height, float text_scale_x, float text_scale_y, float pixel_scale) {
  if (height <= 0 || !std::isfinite(text_scale_x) || text_scale_x <= 0 || !std::isfinite(text_scale_y) ||
      text_scale_y <= 0 || !std::isfinite(pixel_scale) || pixel_scale <= 0)
    return 0;
  const double pixels = height * static_cast<double>(std::max(text_scale_x, text_scale_y)) * pixel_scale;
  return static_cast<int>(std::ceil(std::clamp(pixels, 1.0, 128.0)));
}

Atlas Rasterize(const std::array<int, 256> &widths, int height, float screen_scale, bool uppercase, Face face) {
  Atlas atlas;
  if (height <= 0 || height > 128 || !std::isfinite(screen_scale) || screen_scale <= 0)
    return atlas;

  // Bound memory even for unusually large windows. 128 pixels per line covers
  // normal popup text through 8K; one atlas is at most 2048x2048 (8 MiB).
  const float density = std::min(screen_scale, 128.0f / height);
  const int cell_height = static_cast<int>(std::ceil(height * density));
  for (int ch = 32; ch < 256; ++ch) {
    if (widths[ch] < 0 || widths[ch] > 255)
      return {};
    atlas.glyphs[ch].width = static_cast<int>(std::ceil(widths[ch] * density));
    atlas.glyphs[ch].height = cell_height;
  }

  // A transparent pixel around each cell prevents neighboring characters from
  // bleeding through when linear filtering samples their antialiased edges.
  for (int side = 256; side <= 2048; side *= 2) {
    int x = 1, y = 1;
    bool fits = true;
    for (int ch = 32; ch < 256; ++ch) {
      auto &cell = atlas.glyphs[ch];
      if (!cell.width)
        continue;
      if (cell.width + 2 > side) {
        fits = false;
        break;
      }
      if (x + cell.width + 1 > side) {
        x = 1;
        y += cell_height + 2;
      }
      if (y + cell_height + 1 > side) {
        fits = false;
        break;
      }
      cell.x = x;
      cell.y = y;
      x += cell.width + 2;
    }
    if (fits) {
      atlas.side = side;
      break;
    }
  }
  if (!atlas.side)
    return {};

  stbtt_fontinfo font{};
  // Only bundled fonts are parsed; arbitrary/mod-provided TTFs are never read.
  if (!stbtt_InitFont(&font, face == Face::MainMenu ? OrbitronData : FontData, 0))
    return {};
  int ascent, descent, line_gap;
  stbtt_GetFontVMetrics(&font, &ascent, &descent, &line_gap);
  const float sy = stbtt_ScaleForPixelHeight(&font, cell_height);
  const int baseline = static_cast<int>(std::round(ascent * sy));
  atlas.pixels.assign(atlas.side * atlas.side, 0x0fff);

  for (int ch = 32; ch < 256; ++ch) {
    auto &cell = atlas.glyphs[ch];
    const int codepoint = Codepoint(ch, uppercase);
    const int glyph = codepoint ? stbtt_FindGlyphIndex(&font, codepoint) : 0;
    if (!cell.width || !glyph)
      continue;
    cell.present = true;
    int x0, y0, x1, y1;
    stbtt_GetGlyphBitmapBox(&font, glyph, sy, sy, &x0, &y0, &x1, &y1);
    const float sx = sy * std::min(1.0f, static_cast<float>(cell.width) / std::max(1, x1 - x0));
    stbtt_GetGlyphBitmapBox(&font, glyph, sx, sy, &x0, &y0, &x1, &y1);
    const int w = x1 - x0, h = y1 - y0;
    if (!w || !h) // spaces retain the legacy advance but have no ink
      continue;
    std::vector<unsigned char> coverage(w * h);
    stbtt_MakeGlyphBitmap(&font, coverage.data(), w, h, w, sx, sy, glyph);
    const int left = (cell.width - w) / 2;
    for (int y = 0; y < h; ++y) {
      const int cy = baseline + y0 + y;
      if (cy < 0 || cy >= cell.height)
        continue;
      for (int x = 0; x < w; ++x) {
        const int cx = left + x;
        if (cx < 0 || cx >= cell.width)
          continue;
        const int alpha = (coverage[y * w + x] * 15 + 127) / 255;
        atlas.pixels[(cell.y + cy) * atlas.side + cell.x + cx] = static_cast<uint16_t>((alpha << 12) | 0x0fff);
      }
    }
  }
  return atlas;
}
} // namespace popup_font
