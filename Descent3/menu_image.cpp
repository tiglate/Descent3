#include "menu_image.h"

#include "bitmap.h"
#include "cfile.h"
#include "menu_image_decode.h"
#include "renderer.h"
#include <algorithm>
#include <cstring>

namespace {
constexpr unsigned TileSize = 256;
}

MenuImage::~MenuImage() {
  for (int tile : tiles)
    bm_FreeBitmap(tile);
}

bool MenuImage::Open() {
  auto path = cf_LocatePath(std::filesystem::path("images") / "mainmenu_new.png");
  MenuImagePixels image;
  if (path.empty() || !LoadMenuImagePixels(path, image))
    return false;
  image_width = image.width;
  image_height = image.height;
  for (unsigned top = 0; top < image_height; top += TileSize) {
    for (unsigned left = 0; left < image_width; left += TileSize) {
      int tile = bm_AllocBitmapRGBA(TileSize, TileSize);
      if (tile < 0)
        return false;
      tiles.push_back(tile);
      auto *dest = reinterpret_cast<uint8_t *>(bm_data(tile, 0));
      unsigned width = std::min(TileSize, image_width - left);
      for (unsigned y = 0; y < TileSize; ++y) {
        unsigned source_y = std::min(top + y, image_height - 1);
        auto *row = dest + y * TileSize * 4;
        const auto *source = image.rgba.data() + (static_cast<size_t>(source_y) * image_width + left) * 4;
        std::memcpy(row, source, width * 4);
        for (unsigned x = width; x < TileSize; ++x)
          std::memcpy(row + x * 4, row + (width - 1) * 4, 4);
      }
      GameBitmaps[tile].flags |= BF_CHANGED;
    }
  }
  return true;
}

void MenuImage::Draw(int width, int height) const {
  if (!image_width || !image_height || tiles.empty())
    return;
  const float scale = std::min(float(width) / image_width, float(height) / image_height);
  const int x_offset = (width - int(image_width * scale)) / 2;
  const int y_offset = (height - int(image_height * scale)) / 2;
  const unsigned columns = (image_width + TileSize - 1) / TileSize;
  rend_SetAlphaType(AT_ALWAYS);
  rend_SetLighting(LS_NONE);
  rend_SetColorModel(CM_MONO);
  rend_SetOverlayType(OT_NONE);
  rend_SetWrapType(WT_CLAMP);
  rend_SetZBufferState(0);
  for (unsigned y = 0; y < image_height; y += TileSize) {
    for (unsigned x = 0; x < image_width; x += TileSize) {
      unsigned tw = std::min(TileSize, image_width - x);
      unsigned th = std::min(TileSize, image_height - y);
      rend_DrawScaledBitmap(x_offset + int(x * scale), y_offset + int(y * scale), x_offset + int((x + tw) * scale),
                            y_offset + int((y + th) * scale), tiles[(y / TileSize) * columns + x / TileSize], 0, 0,
                            float(tw) / TileSize, float(th) / TileSize);
    }
  }
  rend_SetZBufferState(1);
}
