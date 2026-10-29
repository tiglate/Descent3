#include <filesystem>
#include <fstream>
#include <random>
#include <gtest/gtest.h>

#include "game_assets.h"

namespace {
class GameAssets : public testing::Test {
protected:
  std::filesystem::path root;
  std::string error;

  void SetUp() override {
    root = std::filesystem::temp_directory_path() / ("d3-assets-test-" + std::to_string(std::random_device{}()));
    ASSERT_TRUE(std::filesystem::create_directory(root));
  }
  void TearDown() override { std::filesystem::remove_all(root); }

  std::filesystem::path Game(const std::string &name = "Original game") {
    const auto path = root / std::filesystem::u8path(name);
    std::filesystem::create_directories(path / "missions");
    std::ofstream(path / "d3.hog") << "test archive";
    return path;
  }
};
} // namespace

TEST_F(GameAssets, FirstRunHasNoConfiguredInstallation) {
  EXPECT_TRUE(game_assets::Load(root / "preferences").empty());
}

TEST_F(GameAssets, ValidatesRequiredFilesWithoutModifyingThem) {
  const auto game = Game();
  EXPECT_TRUE(game_assets::Validate(game, error)) << error;
  EXPECT_TRUE(error.empty());
  EXPECT_EQ(std::filesystem::file_size(game / "d3.hog"), 12);
  EXPECT_FALSE(std::filesystem::exists(game / game_assets::config_filename));
}

TEST_F(GameAssets, SavesAbsoluteUnicodePathAndLoadsOnNextLaunch) {
  const auto game = Game("Original game \xc3\xa7 spaces");
  const auto prefs = root / "preferences";
  ASSERT_TRUE(game_assets::Save(prefs, game, error)) << error;
  EXPECT_EQ(game_assets::Load(prefs), std::filesystem::canonical(game));
  EXPECT_TRUE(game_assets::Validate(game_assets::Load(prefs), error));
}

TEST_F(GameAssets, AcceptsCaseInsensitiveOriginalGameNames) {
  const auto game = Game();
  std::filesystem::rename(game / "d3.hog", game / "D3.HOG");
  std::filesystem::rename(game / "missions", game / "MISSIONS");
  EXPECT_TRUE(game_assets::Validate(game, error)) << error;
}

TEST_F(GameAssets, RejectsMissingArchiveOrMissionsOrAnArchiveDirectory) {
  EXPECT_FALSE(game_assets::Validate(root / "missing", error));
  EXPECT_FALSE(error.empty());
  const auto game = Game();
  std::filesystem::remove(game / "d3.hog");
  EXPECT_FALSE(game_assets::Validate(game, error));
  std::filesystem::create_directory(game / "d3.hog");
  EXPECT_FALSE(game_assets::Validate(game, error));
  const auto other = Game("Other");
  std::filesystem::remove(other / "missions");
  EXPECT_FALSE(game_assets::Validate(other, error));
}

TEST_F(GameAssets, InvalidSelectionDoesNotReplaceExistingConfiguration) {
  const auto game = Game();
  const auto prefs = root / "preferences";
  ASSERT_TRUE(game_assets::Save(prefs, game, error));
  EXPECT_FALSE(game_assets::Save(prefs, root, error));
  EXPECT_EQ(game_assets::Load(prefs), std::filesystem::canonical(game));
}

TEST_F(GameAssets, MovedInstallationIsDetectedAndCanBeReplaced) {
  const auto game = Game();
  const auto prefs = root / "preferences";
  ASSERT_TRUE(game_assets::Save(prefs, game, error));
  const auto moved = root / "Moved game";
  std::filesystem::rename(game, moved);
  EXPECT_FALSE(game_assets::Validate(game_assets::Load(prefs), error));
  ASSERT_TRUE(game_assets::Save(prefs, moved, error));
  EXPECT_EQ(game_assets::Load(prefs), std::filesystem::canonical(moved));
}

TEST_F(GameAssets, ReportsConfigurationWriteFailures) {
  const auto game = Game();
  std::ofstream(root / "not-a-directory") << "file";
  EXPECT_FALSE(game_assets::Save(root / "not-a-directory", game, error));
  EXPECT_FALSE(error.empty());
}

TEST_F(GameAssets, ReadsWindowsLineEndingsAndHandlesEmptyConfiguration) {
  const auto game = Game();
  std::ofstream(root / game_assets::config_filename, std::ios::binary) << game.u8string() << "\r\n";
  EXPECT_EQ(game_assets::Load(root), game);
  std::ofstream(root / game_assets::config_filename, std::ios::trunc).close();
  EXPECT_TRUE(game_assets::Load(root).empty());
}
