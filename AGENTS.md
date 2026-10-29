# Repository Guidelines

## Project Structure & Module Organization

This is the Descent 3 open-source engine, built with CMake and C++17.
`Descent3/` contains gameplay, configuration, and pilot persistence. Shared
modules include `ddio/` (input), `renderer/`, `ui/`, `physics/`, and `sndlib/`.
Platform code lives in `linux/` and `win32/`; multiplayer modules are in
`netgames/` and `netcon/`. Tests live in `tests/` and module-specific `tests/`
directories. Keep generated files in `build/` or preset-managed `builds/`.

Commercial game assets are supplied separately. First-run setup validates and
saves their location in the user's `game-assets.cfg`. Never embed workstation
paths or modify or commit commercial assets.

## Build, Test, and Development Commands

Use CMake, Ninja, a C++17 compiler, and a bootstrapped vcpkg checkout identified
by `VCPKG_ROOT`. See `BUILD.md` for platform prerequisites and CMake presets.
The local Linux wrapper provides:

- `./make.sh configure`: configure the Ninja Multi-Config build.
- `./make.sh build` / `./make.sh release`: compile Debug / Release.
- `./make.sh install`: build Release and install to `~/Apps/Descent3`.
- `./make.sh run -windowed`: build, install, and launch; arguments reach the game.

Enable testing explicitly before running the suite:

```sh
rtk proxy cmake -S . -B build -DBUILD_TESTING=ON
rtk proxy cmake --build build --config Release --parallel
rtk proxy ctest --test-dir build -C Release --output-on-failure
```

Never configure in the source root.

## Coding Style & Naming Conventions

Follow `.editorconfig`: two-space indentation, UTF-8, and LF endings, with
documented exceptions for Visual Studio project files. `.clang-format` uses
LLVM style, attached braces, and a 120-column limit. Format changed C++ files
with `clang-format -i path/to/file.cpp`; avoid repository-wide formatting.
Match surrounding naming conventions instead of renaming legacy APIs. Keep
serialized controller IDs and pilot-file formats backward compatible.

## Testing Guidelines

Tests use GoogleTest and CTest. Name files `*_tests.cpp` and use descriptive
`TEST(Suite, Behavior)` cases. Add regression tests for changed behavior;
there is no specified coverage percentage. Filter controls tests with
`ctest --test-dir build -C Release -R 'Control(Presets|Bindings)'`.
For UI/input changes, also verify the dialogs, saved bindings, and gameplay.
Use temporary preferences through `XDG_DATA_HOME` when testing pilots.

## Commit & Pull Request Guidelines

History uses concise, descriptive subjects such as `Specify SDL version` and
`build: fix build failure ...`; Conventional Commits are not mandatory.
Follow `.github/pull_request_template.md`: identify change types, explain
behavior, link related issues, report local testing, and include screenshots
for visible changes. Document modified functionality and preserve unrelated
worktree changes.

## Agent Instructions

Prefix shell commands with `rtk`; use `rtk proxy` for unfiltered commands.
