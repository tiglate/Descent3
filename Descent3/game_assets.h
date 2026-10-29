#ifndef D3_GAME_ASSETS_H
#define D3_GAME_ASSETS_H

#include <filesystem>
#include <string>

namespace game_assets {
constexpr const char *config_filename = "game-assets.cfg";

// Configuration is a single UTF-8 directory path, independent of the engine
// registry and commercial game files.
bool Validate(const std::filesystem::path &directory, std::string &error);
std::filesystem::path Load(const std::filesystem::path &preferences);
bool Save(const std::filesystem::path &preferences, const std::filesystem::path &directory, std::string &error);
} // namespace game_assets
#endif
