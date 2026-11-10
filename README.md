# Descent 3

![Descent 3 main menu with the modern widescreen artwork](screenshots/main-menu.png)

The Descent 3 engine, open source under [GPL-3.0](LICENSE). This fork builds on
the community engine with modern controls, smoother mouse handling, a
high-resolution interface, simpler setup and many reliability fixes, while
staying compatible with original pilots, saves and missions.

**Game assets are not included.** You need an official copy of Descent 3 (for
example from GOG or Steam). See [USAGE.md](USAGE.md) for what the engine needs.

## Credits

**The original game.** Descent 3 was developed by **Outrage Entertainment** and
published by **Interplay** in 1999, continuing the Descent series created by
**Parallax Software**. Everything you fly through, see and hear is their work.

**The open source release.** The source was released under GPL-3.0 in April
2024. It includes the unreleased 1.5 patch by **Kevin Bentley** and **Jeff
Slutter**.

**The community engine.** The [Descent Developers](https://github.com/DescentDevelopers/Descent3)
community turned the release into a portable, maintained engine for 64-bit
Windows, macOS and Linux on SDL. Major contributors include Azamat H. Hackimov,
Louis Gombert, Jan Engelhardt, Chris Sarbora, Thomas Roß, GravisZro, Jason Yundt,
C.W. Betts, Ryan C. Gordon and many others. See the
[contributor list](https://github.com/DescentDevelopers/Descent3/graphs/contributors).
This fork regularly merges their work.

**This fork.** The improvements listed below are by **Tiglate Pileser III**.

Bundled fonts are Orbitron by Matt McInerney and Arimo, both under the SIL Open
Font License. See [third_party/fonts](third_party/fonts/README.md) and
[THIRD_PARTY.md](THIRD_PARTY.md) for all third-party credits.

## What's new in this fork

- **Modern controls:** WASD flight with Mouselook for new pilots, a
  Modern/Classic/Custom preset selector with previews, and rebinding that
  detects conflicts and offers Replace, Swap or Cancel.
- **Better mouse handling:** precise and consistent cursor and flight input on
  Linux and high-DPI displays. See [Mouse handling](#mouse-handling) below.
- **High-resolution interface:** scalable fonts across menus, dialogs, HUD and
  briefings, menus drawn at native resolution, and message boxes and F1 help that
  scale with the game resolution.
- **Modern main menu:** widescreen menu movie and loading screen, Orbitron
  lettering, and gold hover and focus highlights. The classic background is one
  toggle away.
- **Better defaults:** fresh installs start at the desktop resolution with
  maximum detail, full-quality textures and high-quality sound. Saved settings
  always win.
- **Simple setup:** a native folder chooser on first launch finds your
  original game and remembers it, and `-assetdir` sets it from the command line.
- **Linux workflow:** a `make.sh` wrapper and Makefile to configure, build,
  test, install and run.
- **Reliability:** a warning-free build, plus fixes found along the way:
  ammo wrap-around, missing `break` statements, shifted detail presets, DNS
  lookup lifetime, WAV parsing, startup with missing config files, and renderer
  re-initialization. Regression tests cover these fixes.
- **Documentation:** a reference for the game data and model formats in
  [docs/](docs/), and an ongoing [source review plan](docs/code-review/PLAN.md).

The [CHANGELOG](CHANGELOG.md) has the full list.

## Controls

New pilots start with the **Modern** preset and **Mouselook**. To switch an
existing pilot, open **Configure Keyboard → Control preset… → Modern → OK**, then
leave the controls menu to save. **Classic** restores the original layout, and
**Custom** keeps your own bindings.

| Action | Modern binding |
| --- | --- |
| Forward / reverse | W / S |
| Strafe left / right | A / D |
| Ascend / descend | Space / Left Ctrl |
| Roll left / right | Q / E |
| Afterburner | Left Shift |
| Fire primary / missile | Left / right mouse button |
| Cycle primaries / missiles | Mouse wheel up / down |
| Flare | F or middle mouse button |
| Automap / headlight | Tab / H |
| Release or capture the mouse | Ctrl+G |

If you bind an input that is already in use, you can **Replace** the old
binding, **Swap** the two, or **Cancel**. **Restore Settings** undoes changes
made since you opened the menu. Switching presets keeps your joystick bindings,
mouse sensitivity, inversion and mouse mode. The
[controls guide](USAGE.md#keyboard-and-mouse-presets) has more detail.

## Mouse handling

The mouse code was reworked for SDL3, and the result is most noticeable on
Linux:

- **Accurate cursor:** sub-pixel SDL3 motion is kept instead of dropped, the
  old 20× menu-cursor slowdown is gone, and motion is no longer re-applied, which
  used to make the cursor speed up unpredictably.
- **Same speed everywhere:** the cursor in in-game dialogs moves as fast as in
  the main menus, whatever the game resolution.
- **All your buttons:** right and middle buttons are mapped correctly, the wheel
  works as weapon cycling, and side buttons (4 to 8) can be bound separately
  from the wheel.
- **Reliable capture:** the mouse is captured and the desktop cursor hidden
  during play. Your Ctrl+G choice survives focus changes, fullscreen switches and
  window recreation. `-nomousegrab` disables capture entirely.
- **Tunable feel:** under **Joy/Mouse → Adjust Settings**, choose Mouselook or
  Flight Sim, set X/Y sensitivity up to 16×, and toggle **Invert mouse Y**.
  Mouselook uses the full sensitivity value, and gameplay sensitivity never
  changes the menu cursor.

## Getting started on Linux

Install CMake, Ninja, a C++17 compiler and the packages listed in
[BUILD.md](BUILD.md), and point `VCPKG_ROOT` at a bootstrapped vcpkg checkout.
Images and videos are stored with Git LFS, so run `git lfs install` before
cloning, or `git lfs pull` in an existing clone.

```sh
export VCPKG_ROOT=/path/to/vcpkg
./make.sh run -windowed
```

The first launch asks where Descent 3 is installed. Pick the folder that
contains `d3.hog` and `missions/`. The location is saved in `game-assets.cfg` in
`~/.local/share/Outrage Entertainment/Descent 3/`, next to your pilots. Your
original game files are never modified. To set or change the location without
the dialog:

```sh
./make.sh run -assetdir "/path/to/Descent 3"
```

| Command | Purpose |
| --- | --- |
| `./make.sh build` | Debug build |
| `./make.sh release` | Release build |
| `./make.sh install` | Release build installed to `~/Apps/Descent3` |
| `./make.sh uninstall` | Remove the installed files, keeping pilots and settings |
| `./make.sh run [game arguments]` | Install if needed, then launch |
| `./make.sh test` | Debug build and run the test suite |

`make release`, `make run ARGS="-windowed"` and so on work the same through
the root Makefile. For Windows, macOS and manual CMake builds, see
[BUILD.md](BUILD.md). For command-line options and file locations, see
[USAGE.md](USAGE.md).

## Contributing

Read [AGENTS.md](AGENTS.md) for the repository layout, code style and testing
guidelines. Include screenshots for visible UI changes. Issues that also affect
the upstream engine belong on the
[Descent Developers tracker](https://github.com/DescentDevelopers/Descent3/issues).
The community meets at the [Descent Developer Network](https://discord.gg/GNy5CUQ)
on Discord.

## License

The engine is licensed under [GPL-3.0](LICENSE). The bundled fonts use the SIL
Open Font License 1.1. The original game's data is commercial and is not part of
this repository.
