# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

See `AGENTS.md` for the repository guidelines (style, testing, commit/PR conventions); `BUILD.md` and `USAGE.md` for platform prerequisites and asset setup. Shell commands are prefixed with `rtk` (use `rtk proxy <cmd>` for unfiltered output).

## Build, run, test

Descent 3 engine: C++17, CMake + Ninja Multi-Config, dependencies (SDL3, glm, plog, cpp-httplib, zlib, gtest) via vcpkg. `VCPKG_ROOT` must point at a bootstrapped vcpkg checkout. Never configure in the source root.

Linux wrapper (build dir `build/`, install prefix `~/Apps/Descent3`):

```sh
./make.sh configure      # Ninja Multi-Config
./make.sh build          # Debug
./make.sh release        # Release
./make.sh install        # Release + install
./make.sh uninstall      # remove files listed in build/install_manifest.txt
./make.sh run -windowed  # install if needed, launch; args go to the game
./make.sh test           # Debug build + ctest
```

A root `Makefile` forwards to `make.sh` (`make release ARGS="-j 4"`). Presets in `CMakePresets.json` (`win`, `mac`, `linux`, `linux-cross-arm64`) build into `builds/<preset>`.

Tests are off by default (`BUILD_TESTING=OFF`):

```sh
rtk proxy cmake -S . -B build -DBUILD_TESTING=ON
rtk proxy cmake --build build --config Release --parallel
rtk proxy ctest --test-dir build -C Release --output-on-failure
rtk proxy ctest --test-dir build -C Release -R 'Control(Presets|Bindings)'   # single suite / test
```

Tests are GoogleTest, files named `*_tests.cpp`, in top-level `tests/` (registered in `tests/CMakeLists.txt` with `gtest_discover_tests`) and in module `tests/` dirs (e.g. `cfile/tests`). Format changed files only with `clang-format -i <file>` (no repo-wide formatting).

## Assets and local state

Commercial game data (`d3.hog`, `missions/`) is never in the repo. First launch prompts for the install folder and saves it in `game-assets.cfg` under the user data dir (`~/.local/share/Outrage Entertainment/Descent 3/`, or `$XDG_DATA_HOME`); `-assetdir <path>` sets it explicitly. Logic lives in `Descent3/game_assets.cpp`. Don't embed workstation paths in CMake defaults. For pilot/preference testing, point `XDG_DATA_HOME` at a temp dir.

## Architecture

The top-level `CMakeLists.txt` adds one subdirectory per module, each building a static library that the `Descent3/` executable links. Rough layering:

- `Descent3/` — the game itself: game loop (`GameLoop.cpp`, `gamesequence.cpp`), AI (`AI*.cpp`, `BOA.cpp`, `aipath.cpp`), objects/physics glue, level load (`LoadLevel.cpp`), TelCom UI, HUD, pilots, multiplayer (`multi*.cpp`), controls (`Controls.cpp`, `ctlconfig.cpp`, `CtlCfgElem.cpp`), and the Osiris script host (`Osiris*.cpp`).
- Engine libraries: `renderer/` (OpenGL), `ddio/` (input/IO abstraction), `sndlib/` + `music/` + `stream_audio/` (audio), `ui/` (menu widgets), `physics/`, `model/`, `bitmap/`, `cfile/` (HOG archive / file layer), `manage/` (game data "pagefiles": textures, objects, weapons, etc.), `vecmat/`, `fix/`, `grtext/`, `2dlib/`, `libmve/` (movies), `networking/`, `mem/`, `ddebug/`, `logger/`.
- `linux/`, `win32/` — platform layers (`win32/` and `editor/`/`dd_grwin32/` only build on Windows with `BUILD_EDITOR`).
- `netcon/` (network connection types) and `netgames/` (game modes) build as loadable modules; `scripts/` holds the per-level Osiris scripts (compiled into shared modules, headers `osiris_*.h`; `.msg` files are localized strings) — keep them ABI-compatible with the host in `Descent3/`.
- `tools/` — hogmaker, hogedit, and other utilities; `module/` is the dynamic-module loader.

Compatibility constraints: serialized controller IDs and pilot-file formats must stay backward compatible; match legacy naming rather than renaming old APIs.
