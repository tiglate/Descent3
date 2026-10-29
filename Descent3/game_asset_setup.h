#ifndef D3_GAME_ASSET_SETUP_H
#define D3_GAME_ASSET_SETUP_H

#include <filesystem>

// Resolves and remembers original game data before the engine loads assets.
// Returns an empty path on cancellation. Throws on headless/configuration errors.
std::filesystem::path SetupGameAssets(const std::filesystem::path &preferences, bool headless);
#endif
