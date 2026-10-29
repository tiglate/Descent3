#include "game_asset_setup.h"

#include <SDL3/SDL.h>
#include <mutex>
#include <stdexcept>
#include <string>

#include "args.h"
#include "game_assets.h"
#include "log.h"

namespace {
struct FolderChoice {
  std::mutex mutex;
  bool done = false;
  std::string path;
  std::string error;
};

void SDLCALL FolderChosen(void *userdata, const char *const *files, int) {
  auto &choice = *static_cast<FolderChoice *>(userdata);
  std::lock_guard<std::mutex> guard(choice.mutex);
  if (!files) {
    choice.error = SDL_GetError();
    if (choice.error.empty())
      choice.error = "No native file-dialog backend is available.";
  } else if (files[0])
    choice.path = files[0];
  choice.done = true;
}

std::filesystem::path ChooseFolder(const std::filesystem::path &previous) {
  FolderChoice choice;
  const auto location = previous.u8string();
  SDL_ShowOpenFolderDialog(FolderChosen, &choice, nullptr, location.empty() ? nullptr : location.c_str(), false);
  for (;;) {
    {
      std::lock_guard<std::mutex> guard(choice.mutex);
      if (choice.done) {
        if (!choice.error.empty())
          throw std::runtime_error("Unable to open the folder chooser: " + choice.error +
                                   "\nStart the game with -assetdir followed by your game installation path.");
        return std::filesystem::u8path(choice.path);
      }
    }
    // SDL's Linux portal backend needs the event loop while its asynchronous
    // callback may be dispatched on another thread.
    SDL_PumpEvents();
    SDL_Delay(10);
  }
}
} // namespace

std::filesystem::path SetupGameAssets(const std::filesystem::path &preferences, bool headless) {
  std::string error;
  const int argument = FindArg("-assetdir");
  if (argument) {
    const char *value = GetArg(argument + 1);
    if (!value || !*value)
      throw std::runtime_error("-assetdir requires the original game's installation directory.");
    const auto path = std::filesystem::u8path(value);
    if (!game_assets::Save(preferences, path, error))
      throw std::runtime_error(error);
    return game_assets::Load(preferences);
  }

  auto path = game_assets::Load(preferences);
  if (!path.empty() && game_assets::Validate(path, error))
    return path;

  if (headless)
    throw std::runtime_error("Original game assets are not configured or are unavailable. "
                             "Start with -assetdir followed by the installation directory.");

  std::string message =
      path.empty() ? "Welcome to Descent 3!\n\n" : "The saved game installation is no longer available.\n\n";
  message += "Choose the folder where the original Descent 3 is installed. It must contain d3.hog and missions/.\n\n"
             "The location will be saved for future launches. The original game files will not be changed.";
  if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Locate original Descent 3", message.c_str(), nullptr))
    throw std::runtime_error(
        "Unable to show asset setup. Start with -assetdir followed by the game installation path.");

  for (;;) {
    path = ChooseFolder(path);
    if (path.empty())
      return {};
    if (game_assets::Save(preferences, path, error)) {
      LOG_INFO << "Saved original game installation in " << preferences / game_assets::config_filename;
      return game_assets::Load(preferences);
    }
    const SDL_MessageBoxButtonData buttons[] = {{SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Choose again"},
                                                {SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "Cancel"}};
    const SDL_MessageBoxData dialog{
        SDL_MESSAGEBOX_ERROR, nullptr, "Game installation not accepted", error.c_str(), 2, buttons, nullptr};
    int result = 0;
    if (!SDL_ShowMessageBox(&dialog, &result) || result != 1)
      return {};
  }
}
