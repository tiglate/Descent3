#include "menu_image_decode.h"

#include <png.h>
#include <fstream>

bool LoadMenuImagePixels(const std::filesystem::path &path, MenuImagePixels &pixels) {
  pixels = {};
  std::ifstream input(path, std::ios::binary | std::ios::ate);
  if (!input)
    return false;
  const auto size = input.tellg();
  if (size <= 0 || size > 32 * 1024 * 1024)
    return false;
  std::vector<uint8_t> bytes(static_cast<size_t>(size));
  input.seekg(0);
  if (!input.read(reinterpret_cast<char *>(bytes.data()), bytes.size()))
    return false;

  png_image image{};
  image.version = PNG_IMAGE_VERSION;
  if (!png_image_begin_read_from_memory(&image, bytes.data(), bytes.size())) {
    png_image_free(&image);
    return false;
  }
  if (!image.width || !image.height || image.width > 4096 || image.height > 4096) {
    png_image_free(&image);
    return false;
  }
  image.format = PNG_FORMAT_RGBA;
  MenuImagePixels result;
  result.width = image.width;
  result.height = image.height;
  result.rgba.resize(PNG_IMAGE_SIZE(image));
  const bool success = png_image_finish_read(&image, nullptr, result.rgba.data(), 0, nullptr) != 0;
  png_image_free(&image);
  if (!success)
    return false;
  pixels = std::move(result);
  return true;
}
