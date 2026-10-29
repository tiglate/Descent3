# Descent 3 Open Source Usage

**Important note**: This open source distribution of Descent 3 DOES NOT CONTAIN
GAME ASSETS. Assets must be acquired separately from an official copy of the
game. See [BUILD.md](BUILD.md) for prerequisites and build instructions.

## Run locally

After setting up the build dependencies, run these commands from the checkout:

```sh
make run
make run ARGS="-windowed"
```

The Makefile forwards to `make.sh`; `make run` builds and installs Release
before launching it from `~/Apps/Descent3`. Arguments in `ARGS` go to the game.
The direct equivalent is `./make.sh run -windowed`. To launch an installed
copy without building, run its `Descent3` executable from its installation
directory.

## First-run asset setup

The application asks you to locate the original game's installation on its
first launch. Select the folder containing `d3.hog` and `missions/`. The
location is validated and saved in `game-assets.cfg` alongside your user
preferences and pilots. Assets are read directly from the original installation.
Later launches reuse the saved path; a missing or moved installation triggers
setup again. Cancelling setup closes the application without starting the game.

To choose another installation or configure a headless server, pass
`-assetdir "/path/to/Descent 3"`. This validates and saves the path without
opening the chooser. If a native folder chooser is unavailable, use this option.
Delete `game-assets.cfg` to repeat setup while retaining your other settings.

On Linux the file normally lives under
`~/.local/share/Outrage Entertainment/Descent 3/`, respecting `XDG_DATA_HOME`.
Building the engine does not require the commercial assets.

For example:

```sh
make run ARGS='-assetdir "/path/to/Descent 3" -windowed'
```

On a desktop, the chooser uses the native dialog backend available to SDL.
If that backend is unavailable, use `-assetdir` as above. Dedicated servers
need either a valid saved location or an explicit `-assetdir` argument.

## Manual portable installation

The following steps describe assembling a separate portable game directory.
When building locally with the wrapper, you can instead run `make run` and
point first-run setup at the original installation without copying its assets.

Report reproducible issues with the engine version, platform, and relevant
logs. The upstream project uses its
[online issue tracker](https://github.com/DescentDevelopers/Descent3/issues).

1. Make sure that you have a copy of Descent 3. You can purchase a copy of
Descent 3 from [GOG](https://www.gog.com/game/descent_3_expansion) or
[Steam](https://store.steampowered.com/app/273590/Descent_3/).

2. Install Descent 3.
**Note for Steam users:** If you own Descent 3 on Steam, then it’s recommended
that you install the Windows version of the game even if you’re running macOS
or Linux, otherwise movies will not work due to
[current lack of OGV support](https://github.com/DescentDevelopers/Descent3/issues/240).
You can use either [Steam Play](https://help.steampowered.com/en/faqs/view/08F7-5D56-9654-39AF)
or [SteamCMD](https://developer.valvesoftware.com/wiki/SteamCMD#Cross-Platform_Installation)
to install the Windows version of the game on macOS or Linux.

3. If your version of Descent 3 is older than v1.4, then
[update it to v1.4](http://descent3.com/downloads.php).

4. Find the installation location of Descent 3. Using the Steam client, you can
find it from the library page using the `Manage > Browse local files`
context menu.

5. Create a new folder named `D3-open-source`.

6. Copy the following files from your installation of Descent 3 to `D3-open-source`:
    - All `.hog` files
    - The `missions` folder
    - _(Optional)_ All `.pld` files
    - _(Optional)_ The `movies` folder

7. Create the `custom/` folder in `D3-open-source`

8. Obtain new Descent 3 engine files:
    - If you want to use pre-built binaries, then download the latest
      [release](https://github.com/DescentDevelopers/Descent3/releases). For a
      more cutting-edge experience with the latest features, use the artifacts
      from the latest automated build. You can find the list of automated
      builds [here](https://github.com/DescentDevelopers/Descent3/actions/workflows/build.yml?query=branch%3Amain+event%3Apush).
      These are upstream builds and may not include this fork's asset setup,
      presets, and wrapper. Build this checkout to use the features described here.
    - If you want to build the engine files yourself, the follow the
      instructions in [BUILD.md](BUILD.md). Preset builds put engine files in
      `builds/<platform>/build/<configuration>/`; the Linux wrapper uses
      `build/build/<configuration>/`. `make install` assembles the installed
      engine under `~/Apps/Descent3`.

9. Copy all the new engine files into `D3-open-source` and overwrite any
conflicts.

10. Special notes:
    - D3 Open Source compiles level scripts in their own hogfiles. Make sure
      you copy and overwrite `d3-{platform}.hog`.

11. Run the game:
    - On Windows, run `D3-open-source\Descent3.exe` from a command-line or
      double-click on the `Descent3` executable.
    - On Linux, `cd` to `D3-open-source` and run `./Descent3`. Wayland users
      may need to set environment variable `SDL_VIDEODRIVER=wayland` before
      launching the game.
    - On macOS, open the terminal, `cd` to `D3-open-source`, and run `./Descent3.app/Contents/MacOS/Descent3`.

    On first launch, select `D3-open-source` in asset setup if you copied the
    original data there. A portable engine directory does not relocate your
    user preferences or remove the need to configure the asset location.

## Keyboard and mouse presets

Open **Configure Keyboard > Control preset...** to preview Modern and Classic
bindings. Modern is the default for new pilots. Existing pilots retain their
bindings until you choose a preset and press OK. Custom means keeping your
current layout. Both presets preserve joystick bindings, mouse sensitivity,
mouse inversion, and the selected mouse-control mode.

Modern uses W/S for forward/reverse, A/D for strafing, Space/Left Ctrl for
vertical thrust, Shift for afterburner, and Q/E for roll. Left and right mouse
fire primary weapons and missiles; F or middle mouse fires flares. Mouse wheel
up cycles primary weapons and wheel down cycles missiles. Tab opens the
automap and H toggles the headlight.

If a new binding is already used, the editor names the conflicting action and
offers Replace, Swap, or Cancel. Swap is available for a single conflict;
Replace removes all conflicting assignments. **Restore Settings** restores all
bindings as they were when you entered the controls menu. Leave the menu to
save your bindings with the pilot.

### Gameplay mouse settings

Open **Joy/Mouse > Adjust Settings** to select Flight Sim or Mouselook,
adjust mouse X/Y sensitivity up to 16×, and toggle **Invert mouse Y**.
Inversion is enabled for new/default controls. Existing pilots keep their
saved settings. Mouselook uses the full sensitivity value; Flight Sim still
obeys the ship's turning-rate limit. These settings do not affect menu cursor
speed. **Ctrl+G** releases or recaptures the mouse.

## Troubleshooting

```
Descent 3 Message(Error: Couldn't find the string table.)
```

Check that `game-assets.cfg` points to a complete, accessible original game
installation, including `d3.hog` and `missions/`. Rerun with `-assetdir` to
replace the saved location, or delete only `game-assets.cfg` to repeat setup.
Also ensure the generated `d3-linux.hog`, `d3-win.hog`, or `d3-osx.hog` is
installed beside the engine. Copying assets is optional; do not delete your
pilot or save files to repair an asset location.

For diagnostics, run `make run ARGS="-logfile -loglevel DEBUG"` and inspect
`Descent3.log` in the installation directory. When launching the executable
directly, this log is relative to the directory you launched it from.

## Base directories

The engine keeps writable preferences separate from commercial data. Pilots
(`.plt`), savegames, and `game-assets.cfg` live in the SDL user preferences
directory. On Linux this normally means
`~/.local/share/Outrage Entertainment/Descent 3/`; setting `XDG_DATA_HOME`
changes its parent directory. The installation directory is not the default
save directory.

The selected original installation is a read-only asset source. Other base
directories provide packaged engine data or optional overrides. Base-directory
lookup searches later-added directories first, in this order:

1. The executable directory, when distinct from the platform data directory.
2. The platform data directory. For the default portable build this resolves
   from the current working directory; a non-portable build uses its configured
   data directory.
3. The optional legacy `GAME_base_directory` setting.
4. `-additionaldir` directories, last argument first.
5. Optional compiled-in `DEFAULT_ADDITIONAL_DIRS`, last entry first.
6. The original game directory saved by asset setup.
7. The writable user preferences directory.

Only existing directories are added. HOG archives and extension-specific
searches also participate in file loading. Keep generated engine HOG files
beside the executable and use `-assetdir` for the original commercial data.
See [build options](BUILD.md#build-options) for packager-controlled directories;
the Linux wrapper clears compiled-in asset paths.

For an additional read-only data directory, use:

```sh
make run ARGS='-additionaldir "/path/to/extra-data"'
```

This supplements the configured installation; it does not set the writable
directory or replace first-run asset configuration. The legacy `-setdir` and
`-useexedir` arguments are not implemented by the current startup path.

## Command-Line Options

The following command-line options are available in Descent 3. You can set command-line options on the Misc. tab of the Setup section of the Descent 3 launcher or by creating a shortcut to `Descent3.exe`. Case is not significant in command-line options, and `-`, `--`, and `+` are all accepted.

### Display Options

- `-aspect <value>`

    **Type:** floating-point number

    **Default:** 1.3333333333333333

    **Platform:** all

    **Description:** Specifies the screen aspect ratio for non-standard displays, such as wide-screen TVs.

- `-display <number>`

    **Type:** integer

    **Default:** 0

    **Platforms:** all

    **Description:** Run game on the selected display.

- `-fullscreen` or `-f`

    **Type:** boolean

    **Default:** On

    **Platform:** all

    **Description:** Run game in fullscreen mode.

- `-height <height>`

    **Type:** integer

    **Default:** Desktop height of the selected display at startup

    **Platform:** all

    **Description:** Overrides gameplay height. Specify `-width` as well.
    Without both dimensions, startup uses desktop resolution. Video Settings
    can change the resolution during the current session.

- `-himem`

    **Type:** boolean

    **Default:** Off

    **Platform:** all

    **Description:** Forces normal operations even when low memory conditions are detected.

- `-lowmem`

    **Type:** boolean

    **Default:** Off

    **Platform:** all

    **Description:** Uses scaled-down textures and lower quality (8-bit) sounds to conserve memory.

- `-NoRenderWindows`

    **Type:** boolean

    **Default:** Off

    **Platform:** all

    **Description:** Causes all windows to be fully transparent. Use this option if your card does not correctly render partially-transparent windows.

- `-superlowmem`

    **Type:** boolean

    **Default:** Off

    **Platform:** all

    **Description:** Uses the `-lowmem` settings and further scales down textures to conserve memory.

- `-vsync`

    **Type:** boolean

    **Default:** On for new settings; otherwise the saved video setting

    **Platform:** all

    **Description:** Turns on Vertical Sync. The flag will be enabled in the registry so it will be on when the game is run again.

- `-width <width>`

    **Type:** integer

    **Default:** Desktop width of the selected display at startup

    **Platform:** all

    **Description:** Overrides gameplay width. Specify `-height` as well.
    Example: `make run ARGS="-width 1920 -height 1080"`. Without an explicit
    pair, each launch starts at desktop resolution.

- `-windowed` or `-w`

    **Type:** boolean

    **Default:** Off

    **Platform:** all

    **Description:** Runs the game in a window.

### Audio Options

- `-nomusic`

    **Type:** boolean

    **Default:** Off

    **Platform:** all

    **Description:** Disables music.

- `-nosound`

    **Type:** boolean

    **Default:** Off

    **Platform:** all

    **Description:** Disables all sound, including music.

### Input Options

- `-deadzone# <k>`

    **Type:** `#` is either the character `0` or the character `1`. `k` is a floating-point number.

    **Default:** 0.2

    **Platform:** all

    **Description:** Specifies the size of the deadzone for a joystick.

The legacy `-mlooksens` and `-mousesens` arguments are parsed, but their values
are not used by the current SDL controller backend. Use **Joy/Mouse > Adjust
Settings** to change gameplay sensitivity instead.

- `-nomousegrab` or `-m`

    **Type:** boolean

    **Default:** Off

    **Platform:** all

    **Description:** Disable mouse capture.

### Performance Options

- `-fastdemo`

    **Type:** boolean

    **Default:** Off

    **Platform:** all

    **Description:** Causes a demo to play back at the highest speed your computer is capable of.

- `-forcelightmaps`

    **Type:** boolean

    **Default:** Off

    **Platform:** all

    **Description:** Forces the use of lightmaps, even the Default Detail Level is set to Low in the launcher setup.

- `-framecap <fps>`

    **Type:** integer

    **Default:** 60

    **Platform:** all

    **Description:** Limits the framerate to the number of frames per second specified.

- `-nomotionblur`

    **Type:** boolean

    **Default:** Off

    **Platform:** all

    **Description:** Disables motion blur on robots (Pentium III only).

- `-nosatomega`

    **Type:** boolean

    **Default:** Off

    **Platform:** all

    **Description:** Disables alpha saturation on the omega cannon effect.

- `-nosparkles`

    **Type:** boolean

    **Default:** Off

    **Platform:** all

    **Description:** Disables powerup sparkles (Pentium III only).

### Multiplayer and Network Options

- `-audiotauntdelay <time>`

    **Type:** floating-point number

    **Default:** 5.0

    **Platform:** all

    **Description:** Sets the time in seconds the a user must wait after sending an audio taunt before he or she is able to send another. This option is only active when starting a server.

- `-autoexec <file>`

    **Type:** path

    **Default:** `<writable-base-directory>/netgames/autoexec.dmfc`

    **Platform:** all

    **Description:** Specifies the full path and file name of the multiplayer config file to be loaded and executed when a multiplayer game is initialized.

- `-dedicated <config file>` or `-d <config file>`

    **Type:** path

    **Default:** None

    **Platform:** all

    **Description:** Starts a dedicated server.

- `-gamespyport <port>`

    **Type:** integer

    **Default:** 20142

    **Platform:** all

    **Description:** Specifies a port for to listen for GameSpy requests.

- `-gspyfile <config file>`

    **Type:** path

    **Default:** gamespy.cfg

    **Platform:** all

    **Description:** Specifies a GameSpy configuration file to use.

- `-httpproxy <addr>` or `-httpproxy <addr:port>`

    **Type:** string

    **Default:** None

    **Platform:** all

    **Description:** Specifies an HTTP proxy server. Descent 3 uses HTTP to auto-download a mission; use this option if your ISP requires a proxy server for HTTP connections. If the first form is used, the port value defaults to 80.

- `-nooutragelogo`

    **Type:** boolean

    **Default:** Off

    **Platform:** all

    **Description:** Disables the Outrage logo that is normally displayed for five seconds at the start of a multiplayer game.

- `-nomultibmp`

    **Type:** boolean

    **Default:** Off

    **Platform:** all

    **Description:** Disables sending or receiving in-game custom bitmaps for ships.

- `-nonetwork`

    **Type:** boolean

    **Default:** Off

    **Platform:** all

    **Description:** Disables all network multiplayer functionality.

- `-playermessages`

    **Type:** boolean

    **Default:** Off

    **Platform:** all

    **Description:** Disables all weapon-related HUD messages in multiplayer games.

- `-pxoport <port>`

    **Type:** integer

    **Default:** No advertised-port override (0)

    **Platform:** all

    **Description:** Overrides the game port advertised to PXO clients.
    Without an override, the connector uses its normal game-port behavior;
    the engine's default game port is 2092.

- `-useip <IP>`

    **Type:** string

    **Default:** All available network interfaces

    **Platform:** all

    **Description:** Binds Descent 3 to this IP address. Use this option to tell D3 which IP address to use if your computer has multiple IP addresses.

- `-useport <port>`

    **Type:** integer

    **Default:** 2092

    **Platform:** all

    **Description:** Specifies the port that TCP/IP and IPX will use.

- `-usesmoothing`

    **Type:** boolean

    **Default:** Off

    **Platform:** all

    **Description:** Enables code to smooth the interpolation for fast-moving objects. This will fix "skipping" problems with fast weapons, such as the Phoenix. This option applies to clients only; using it on the server has no effect.

### Base Directory Options

- `-assetdir <path>`

    **Type:** path

    **Default:** The location saved in `game-assets.cfg`; otherwise first-run setup

    **Platform:** all

    **Description:** Validates the original installation and saves its absolute
    path for later launches. The directory must contain `d3.hog` and `missions/`.
    Use this option for dedicated servers or when native dialogs are unavailable.

- `-additionaldir <path>`

    **Type:** path

    **Default:** None

    **Platform:** all

    **Description:** Adds a read-only base directory. Repeat the option for
    multiple directories; later arguments have priority within this group.
    See [Base directories](#base-directories) for the full search order.

### Other Options

- `-loadlevel`

    **Type:** int

    **Default:** Off

    **Platform:** all

    **Description:** Load a specific level from the mission selected with `-mission`. Only has an effect when `-mission` is specified.

- `-logfile`

    **Type:** boolean

    **Default:** Off

    **Platform:** all

    **Description:** Enables `Descent3.log` in the launch working directory
    for both Debug and Release. With `make run`, that is `~/Apps/Descent3`.
    Use `-loglevel DEBUG` to include debug messages. An existing log is replaced
    when file logging starts.

- `-loglevel <LEVEL>`

    **Type:** string

    **Default:** INFO (on Release), DEBUG (on Debug)

    **Platform:** all

    **Description:** Set log level (NONE, VERBOSE, DEBUG, INFO, WARNING, ERROR, FATAL)

- `-makemovie`

    **Type:** boolean

    **Default:** Off

    **Platform:** all

    **Description:** Causes the demo system to save a screenshot of every frame during playback.

- `-mission <name>`

    **Type:** string

    **Default:** None

    **Platform:** all

    **Description:** Loads the specified mission file at startup. Example: `-mission d3` to load the main campaign. Combine this option with `-loadlevel` to load a specific level from the mission.

- `-pilot <name>`

    **Type:** string

    **Default:** None

    **Platform:** all

    **Description:** Specifies the pilot to use, skipping the pilot selection dialog when the game starts.

- `-service`

    **Type:** boolean

    **Default:** Off

    **Platform:** all

    **Description:** Run game in service mode.

- `-winconsole`

    **Type:** boolean

    **Default:** Off

    **Platform:** WIN

    **Description:** Enable windows console (off by default).
