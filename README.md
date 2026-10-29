# Descent 3 Open Source

![Descent 3](https://github.com/DescentDevelopers/Descent3/assets/47716344/82ba0911-ee32-4565-84ee-b432c215ab95)

The Descent 3 engine, licensed under [GPL-3.0](LICENSE). It includes the 1.5
patch by Kevin Bentley and Jeff Slutter and subsequent work by the Descent
community. This checkout adds a Linux build/run wrapper, modern control
presets, and mouse and display improvements.

**Game assets are not included.** You need an official copy of Descent 3.
See [USAGE.md](USAGE.md) for asset requirements and installation details.

## Build and run on Linux

Install CMake, Ninja, a C++17 compiler, and the host dependencies listed in
[BUILD.md](BUILD.md). Point `VCPKG_ROOT` at a bootstrapped vcpkg checkout;
the wrapper uses vcpkg to resolve the engine's dependencies.

```sh
export VCPKG_ROOT=/path/to/vcpkg
make release
make run ARGS="-windowed"
```

The root Makefile forwards actions to `make.sh`. Use `make run`, `make install`,
or `make help`; plain `make` builds Debug. Pass wrapper options or game arguments
through `ARGS`, for example `make release ARGS="-j 4"`. The direct `make.sh`
commands below remain available.

On the first launch, a native folder chooser asks where the original game is
installed. Choose the folder containing `d3.hog` and `missions/`. The engine
validates it and saves its absolute path in `game-assets.cfg` in your user
preferences directory. Later launches read that file automatically. If the
installation moves or becomes unavailable, setup asks for its new location.
The original game files are read directly and are not modified or copied.

You can set or change the location explicitly, including for headless use:

```sh
./make.sh run -assetdir "/path/to/Descent 3"
```

This validates and saves the location too. Compilation and installation do
not require game assets, and the wrapper embeds no machine-specific asset path.

| Command | Purpose |
| --- | --- |
| `./make.sh configure` | Configure the Ninja Multi-Config build in `build/` |
| `./make.sh build` | Compile Debug |
| `./make.sh release` | Compile optimized Release |
| `./make.sh install` | Build Release and install to `~/Apps/Descent3` |
| `./make.sh run [game arguments]` | Build, install, and launch Release |
| `./make.sh clean` | Remove compiled outputs while retaining configuration |

Use `./make.sh --help` for additional options. The installed executable is
`~/Apps/Descent3/Descent3`. On Linux, pilots and preferences normally live in
`~/.local/share/Outrage Entertainment/Descent 3/`, or under `XDG_DATA_HOME` when set.
The saved asset-location file is stored there as well. Delete only
`game-assets.cfg` to repeat setup without deleting your pilots or other settings.

For Windows, macOS, and manual CMake builds, follow [BUILD.md](BUILD.md).

## Controls

New pilots use the **Modern** preset. For an existing pilot, open
**Configure Keyboard → Control preset… → Modern → OK**, then leave the
controls menu to save. The selector previews both Modern and Classic;
Custom keeps your current bindings.

| Action | Modern binding |
| --- | --- |
| Forward / reverse | W / S |
| Strafe left / right | A / D |
| Ascend / descend | Space / Left Ctrl |
| Roll left / right | Q / E |
| Afterburner | Left Shift |
| Primary / missile fire | Left / right mouse |
| Cycle primary weapons / missiles | Wheel up / down |
| Flare | F or middle mouse |
| Automap / headlight | Tab / H |

Reassigning an occupied input offers **Replace**, **Swap**, or **Cancel**.
**Restore Settings** restores bindings from when you entered the controls menu.
Preset changes preserve joystick bindings, mouse sensitivity, inversion, and
mouse-control mode. See [the controls guide](USAGE.md#keyboard-and-mouse-presets)
for details.

## Mouse and display settings

- Gameplay starts at the desktop resolution on the selected display. Override
  it with `-width` and `-height`, or change it in Video Settings during play.
- **Joystick/Mouse → Adjust Settings** provides mouse X/Y sensitivity up to
  16× and **Invert mouse Y**, which is enabled for new/default controls.
- **Mouselook** uses the full sensitivity value; flight-sim mode retains the
  ship's turning-rate limit. Gameplay sensitivity does not change menu cursor speed.
- **Ctrl+G** toggles mouse capture. Capture is restored while focused according
  to that choice, including after window recreation.

For example, to launch with an explicit resolution:

```sh
./make.sh run -windowed -width 1920 -height 1080
```

## Tests

Tests use GoogleTest and CTest. Enable them explicitly, then build and run:

```sh
./make.sh configure
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

To run only control-preset and binding tests, add
`-R 'Control(Presets|Bindings)'` to the CTest command.

## Contributing

Read [AGENTS.md](AGENTS.md) for repository structure, coding conventions, and
testing guidance. Use the [pull request template](.github/pull_request_template.md)
and include screenshots for visible UI changes.

Upstream build and runtime issues can be reported on the
[GitHub tracker](https://github.com/DescentDevelopers/Descent3/issues).
Community discussion is available at the
[Descent Developer Network](https://discord.gg/GNy5CUQ).
