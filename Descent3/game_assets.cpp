#include "game_assets.h"

#include <algorithm>
#include <cctype>
#include <fstream>

namespace game_assets {
bool Validate(const std::filesystem::path &directory, std::string &error) {
  error.clear();
  std::error_code ec;
  if (!std::filesystem::is_directory(directory, ec)) {
    error = "The selected directory does not exist or cannot be accessed.";
    return false;
  }
  bool hog = false, missions = false;
  std::filesystem::directory_iterator it(directory, ec), end;
  for (; !ec && it != end; it.increment(ec)) {
    auto name = it->path().filename().u8string();
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return std::tolower(c); });
    if (name == "d3.hog") {
      std::ifstream file(it->path(), std::ios::binary);
      hog = it->is_regular_file(ec) && file.good();
    }
    if (name == "missions")
      missions = it->is_directory(ec);
  }
  if (ec || !hog || !missions) {
    error = "Select the original game's installation folder containing a readable d3.hog file and a missions folder.";
    return false;
  }
  return true;
}

std::filesystem::path Load(const std::filesystem::path &preferences) {
  std::ifstream file(preferences / config_filename, std::ios::binary);
  std::string line;
  if (!std::getline(file, line))
    return {};
  if (!line.empty() && line.back() == '\r')
    line.pop_back();
  return std::filesystem::u8path(line);
}

bool Save(const std::filesystem::path &preferences, const std::filesystem::path &directory, std::string &error) {
  if (!Validate(directory, error))
    return false;
  std::error_code ec;
  const auto absolute = std::filesystem::canonical(directory, ec);
  if (ec) {
    error = "Unable to resolve the game installation directory: " + ec.message();
    return false;
  }
  const auto value = absolute.u8string();
  if (value.find_first_of("\r\n") != std::string::npos) {
    error = "The game installation path must not contain line breaks.";
    return false;
  }
  std::filesystem::create_directories(preferences, ec);
  if (ec) {
    error = "Unable to create the preferences directory: " + ec.message();
    return false;
  }
  std::ofstream file(preferences / config_filename, std::ios::binary | std::ios::trunc);
  file << value << '\n';
  file.close();
  if (!file) {
    error = "Unable to save the game location in " + (preferences / config_filename).u8string();
    return false;
  }
  error.clear();
  return true;
}
} // namespace game_assets
