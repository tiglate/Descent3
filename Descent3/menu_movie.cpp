#include "menu_movie.h"

#include "bitmap.h"
#include "cfile.h"
#include "renderer.h"
#include <SDL3/SDL.h>
#include <ogg/ogg.h>
#include <theora/theoradec.h>
#include <algorithm>
#include <array>
#include <cstdio>
#include <vector>

namespace {
constexpr int TileSize = 256;
int clamp8(int x) { return std::clamp(x, 0, 255); }
} // namespace

struct MenuMovie::State {
  FILE *file = nullptr;
  ogg_sync_state sync{};
  ogg_stream_state stream{};
  th_info info{};
  th_comment comment{};
  th_setup_info *setup = nullptr;
  th_dec_ctx *decoder = nullptr;
  bool sync_ready = false, stream_ready = false;
  std::vector<int> tiles;
  Uint64 next_frame = 0;

  State() {
    th_info_init(&info);
    th_comment_init(&comment);
  }
  ~State() {
    for (int tile : tiles)
      bm_FreeBitmap(tile);
    if (decoder)
      th_decode_free(decoder);
    if (setup)
      th_setup_free(setup);
    th_comment_clear(&comment);
    th_info_clear(&info);
    if (stream_ready)
      ogg_stream_clear(&stream);
    if (sync_ready)
      ogg_sync_clear(&sync);
    if (file)
      fclose(file);
  }
  bool Page(ogg_page &page) {
    for (;;) {
      int result = ogg_sync_pageout(&sync, &page);
      if (result > 0)
        return true;
      if (result < 0)
        return false;
      char *buffer = ogg_sync_buffer(&sync, 4096);
      if (!buffer)
        return false;
      size_t n = fread(buffer, 1, 4096, file);
      if (ogg_sync_wrote(&sync, static_cast<long>(n)) != 0 || n == 0)
        return false;
    }
  }
  bool Packet(ogg_packet &packet) {
    for (;;) {
      int result = ogg_stream_packetout(&stream, &packet);
      if (result > 0)
        return true;
      if (result < 0)
        return false;
      ogg_page page;
      if (!Page(page) || ogg_page_serialno(&page) != stream.serialno || ogg_stream_pagein(&stream, &page) != 0)
        return false;
    }
  }
  bool Open(const std::filesystem::path &path) {
    file = fopen(PATH_TO_CSTR(path), "rb");
    if (!file || ogg_sync_init(&sync) != 0)
      return false;
    sync_ready = true;
    ogg_page page;
    if (!Page(page) || ogg_stream_init(&stream, ogg_page_serialno(&page)) != 0)
      return false;
    stream_ready = true;
    if (ogg_stream_pagein(&stream, &page) != 0)
      return false;
    for (int i = 0; i < 3; ++i) {
      ogg_packet packet;
      if (!Packet(packet) || th_decode_headerin(&info, &comment, &setup, &packet) <= 0)
        return false;
    }
    if (!info.pic_width || !info.pic_height || info.pic_width > 4096 || info.pic_height > 4096 || !info.fps_numerator ||
        !info.fps_denominator || info.pixel_fmt != TH_PF_420)
      return false;
    decoder = th_decode_alloc(&info, setup);
    if (!decoder)
      return false;
    int count = ((info.pic_width + TileSize - 1) / TileSize) * ((info.pic_height + TileSize - 1) / TileSize);
    for (int i = 0; i < count; ++i) {
      int tile = bm_AllocBitmapRGBA(TileSize, TileSize);
      if (tile < 0)
        return false;
      tiles.push_back(tile);
    }
    return Decode();
  }
  bool Decode() {
    ogg_packet packet;
    if (!Packet(packet) || th_decode_packetin(decoder, &packet, nullptr) < 0)
      return false;
    th_ycbcr_buffer planes;
    if (th_decode_ycbcr_out(decoder, planes) != 0)
      return false;
    int columns = (info.pic_width + TileSize - 1) / TileSize;
    for (unsigned y = 0; y < info.pic_height; ++y) {
      int sy = y + info.pic_y;
      for (unsigned tile_x = 0; tile_x * TileSize < info.pic_width; ++tile_x) {
        int tile = tiles[(y / TileSize) * columns + tile_x];
        auto *pixels = reinterpret_cast<uint8_t *>(bm_data(tile, 0)) + (y % TileSize) * TileSize * 4;
        unsigned width = std::min<unsigned>(TileSize, info.pic_width - tile_x * TileSize);
        for (unsigned x = 0; x < width; ++x) {
          int sx = tile_x * TileSize + x + info.pic_x;
          int yy = planes[0].data[sy * planes[0].stride + sx] - 16;
          int cb = planes[1].data[(sy / 2) * planes[1].stride + sx / 2] - 128;
          int cr = planes[2].data[(sy / 2) * planes[2].stride + sx / 2] - 128;
          int r = clamp8((298 * yy + 409 * cr + 128) >> 8);
          int g = clamp8((298 * yy - 100 * cb - 208 * cr + 128) >> 8);
          int b = clamp8((298 * yy + 516 * cb + 128) >> 8);
          pixels[x * 4] = r;
          pixels[x * 4 + 1] = g;
          pixels[x * 4 + 2] = b;
          pixels[x * 4 + 3] = 255;
        }
      }
    }
    for (int tile : tiles)
      GameBitmaps[tile].flags |= BF_CHANGED;
    return true;
  }
};

MenuMovie::MenuMovie() = default;
MenuMovie::~MenuMovie() = default;
bool MenuMovie::Open() {
  auto path = cf_LocatePath(std::filesystem::path("movies") / "mainmenu-modern.ogv");
  if (path.empty())
    return false;
  auto candidate = std::make_unique<State>();
  if (!candidate->Open(path))
    return false;
  state = std::move(candidate);
  state->next_frame = SDL_GetTicksNS() + 1000000000ULL * state->info.fps_denominator / state->info.fps_numerator;
  return true;
}
bool MenuMovie::Draw(int width, int height) {
  if (!state)
    return false;
  Uint64 now = SDL_GetTicksNS();
  if (now >= state->next_frame) {
    if (!state->Decode()) {
      state.reset();
      if (!Open())
        return false;
    } else {
      Uint64 duration = 1000000000ULL * state->info.fps_denominator / state->info.fps_numerator;
      state->next_frame += duration;
      if (state->next_frame + duration < now)
        state->next_frame = now + duration;
    }
  }
  auto &s = *state;
  float scale = std::min(float(width) / s.info.pic_width, float(height) / s.info.pic_height);
  int draw_w = int(s.info.pic_width * scale), draw_h = int(s.info.pic_height * scale);
  int ox = (width - draw_w) / 2, oy = (height - draw_h) / 2;
  int columns = (s.info.pic_width + TileSize - 1) / TileSize;
  rend_ClearScreen(GR_BLACK);
  rend_SetAlphaType(AT_ALWAYS);
  rend_SetLighting(LS_NONE);
  rend_SetColorModel(CM_MONO);
  rend_SetOverlayType(OT_NONE);
  rend_SetWrapType(WT_CLAMP);
  rend_SetZBufferState(0);
  for (unsigned y = 0; y < s.info.pic_height; y += TileSize) {
    for (unsigned x = 0; x < s.info.pic_width; x += TileSize) {
      unsigned tw = std::min<unsigned>(TileSize, s.info.pic_width - x);
      unsigned th = std::min<unsigned>(TileSize, s.info.pic_height - y);
      rend_DrawScaledBitmap(ox + int(x * scale), oy + int(y * scale), ox + int((x + tw) * scale),
                            oy + int((y + th) * scale), s.tiles[(y / TileSize) * columns + x / TileSize], 0, 0,
                            float(tw) / TileSize, float(th) / TileSize);
    }
  }
  rend_SetZBufferState(1);
  return true;
}
