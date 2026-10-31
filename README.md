# Descent 3 Open Source

![Descent 3](https://github.com/DescentDevelopers/Descent3/assets/47716344/82ba0911-ee32-4565-84ee-b432c215ab95)

The Descent 3 engine, licensed under [GPL-3.0](LICENSE). It includes the 1.5
patch by Kevin Bentley and Jeff Slutter and subsequent work by the Descent
community. This checkout adds modern controls, high-resolution text and UI,
higher default graphics and sound quality, simpler asset setup, and a Linux
build/run wrapper, alongside ongoing reliability improvements.

**Game assets are not included.** You need an official copy of Descent 3.
See [USAGE.md](USAGE.md) for asset requirements and installation details.

## Improvements in this checkout

| Area | Improvements |
| --- | --- |
| Graphics and sound | Highest supported display resolution, maximum detail, full-quality textures, and high-quality audio for fresh settings; saved choices are respected |
| Interface | Scalable fonts throughout engine-rendered UI, native-resolution menus, resolution-aware message boxes and F1 help, and corrected artwork tile seams |
| Main menu | Orbitron lettering, dark shadows and outlines, and gold highlights for mouse hover and keyboard focus |
| Controls | Modern keyboard/mouse defaults, Mouselook for new pilots, configurable sensitivity and inversion, and conflict-aware rebinding |
| Setup | Native first-run asset chooser, saved asset location, and separate writable preferences |
| Reliability | Fixes for asynchronous DNS lifetime, WAV loading and cleanup, configuration parsing, and file-size handling, backed by regression tests |

The following sections explain the settings and workflow. See
[USAGE.md](USAGE.md) for the full runtime guide and
[the UI implementation notes](docs/ui-fonts-and-defaults.md) for rendering details
and validation evidence.

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

New pilots use the **Modern** preset with **Mouselook** enabled. For an existing
pilot, open **Configure Keyboard → Control preset… → Modern → OK**, then leave the
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

## Graphics, sound, and mouse settings

Without saved settings, the game starts with the highest supported resolution
on the selected display, maximum detail, 32-bit color, full-quality textures,
bilinear filtering, mipmapping, and high-quality sound with the maximum number
of mixed sounds. Change these through Options; existing saved choices take
precedence over the new defaults.

- Resolution selections persist by width and height. If a saved mode becomes
  unavailable, startup falls back to the highest supported resolution.
  An explicit `-width` and `-height` pair overrides a single launch.
- **Options → Detail → Highest** enables maximum terrain distance, object
  detail, and quality effects. Opening and closing Settings preserves the
  selected terrain detail.
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

## High-resolution interface

Scalable fonts are enabled by default for engine-rendered menus, buttons,
labels, text entry, lists, dialogs, briefings, HUD text, and multiplayer overlays
that use the shared game fonts. Menus render at the display's pixel resolution
while preserving their original layout and aspect ratio.

Main-menu items use **Orbitron Regular**, designed by **Matt McInerney**, with
a dark outline and drop shadow for readability over the background. Hovering
over an item highlights it in gold with an amber glow; keyboard navigation uses
the same highlight. Other shared UI fonts use **Arimo Bold**.

**Options → Video → High-res fonts** lets you switch between scalable and original
bitmap fonts; the choice takes effect immediately and is saved. Fonts are bundled,
so no system font installation is needed. Text embedded in original artwork,
textures, and movies remains part of those assets.

In-game message boxes, including **Abort Mission**, and the **F1** keyboard-help
window scale their artwork, text, and button targets together with the gameplay
resolution. They stay centered without stretching on widescreen displays and
retain their original size at 640×480. Texture clamping also prevents visible
seams between UI artwork tiles while retaining bilinear filtering.

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

Regression coverage also includes font rasterization, UI scaling and mouse
coordinates, rendering dimensions, startup defaults, asset setup, file handling,
WAV parsing, and asynchronous DNS. UI validation and focused sanitizer results
are recorded in the [implementation notes](docs/ui-fonts-and-defaults.md).

The ongoing source review tracks dead code, configuration opportunities,
resource ownership, and clang-tidy findings in a
[Markdown plan](docs/code-review/PLAN.md) and an
[interactive progress dashboard](docs/code-review/dashboard.html).
Open the dashboard locally in a browser for charts, status tables, and
progress import/export; see the [tracker guide](docs/code-review/README.md).

## License and credits

The engine is licensed under [GPL-3.0](LICENSE). Bundled Orbitron and Arimo fonts
use the SIL Open Font License 1.1. Font author credits, provenance, and full
license files are included in [third_party/fonts](third_party/fonts/README.md)
and installed with the game. See [THIRD_PARTY.md](THIRD_PARTY.md) for dependency
credits and licenses. Original commercial game assets are supplied separately.

## Contributing

Read [AGENTS.md](AGENTS.md) for repository structure, coding conventions, and
testing guidance. Use the [pull request template](.github/pull_request_template.md)
and include screenshots for visible UI changes.

Upstream build and runtime issues can be reported on the
[GitHub tracker](https://github.com/DescentDevelopers/Descent3/issues).
Community discussion is available at the
[Descent Developer Network](https://discord.gg/GNy5CUQ).
