#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

struct MenuImagePixels {
  unsigned width = 0;
  unsigned height = 0;
  std::vector<uint8_t> rgba;
};

bool LoadMenuImagePixels(const std::filesystem::path &path, MenuImagePixels &pixels);
