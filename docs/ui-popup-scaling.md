# High-resolution message boxes

This records the original popup fix. Current universal font coverage, the
legacy-font setting, and fresh-start defaults are documented in
[ui-fonts-and-defaults.md](ui-fonts-and-defaults.md).

Small gameplay popups used the `SmallScreen.ogf` artwork and fixed pixel
coordinates regardless of resolution. At 1920×1080 the Abort Mission
confirmation therefore occupied only about 330×90 pixels.

`newuiMessageBox` now opts into `UIF_SCALE_TO_SCREEN`. The window uses a
uniform scale of `max(1, min(screen_width / 640, screen_height / 480))`:

| Screen | Scale |
| --- | ---: |
| 640×480 | 1× |
| 1280×720 | 1.5× |
| 1920×1080 | 2.25× |
| 2560×1440 | 3× |
| 3840×2160 | 4.5× |
| 2560×1080 ultrawide | 2.25× |

The background, text, buttons and rectangle fills share the same rendering
scale. Mouse positions are mapped back to the authored coordinates during
window input processing. The previous rendering scale and physical mouse
coordinates are restored afterward. Scaling uses an existing window flag
field, so `UIWindow`'s object layout is preserved.

Other windows retain their existing rendering behavior. Menu mode still uses
its 640×480 coordinate space. No pilot format or game assets are changed.

## Validation

Linux Debug build, 2026-10-30:

- Built `Descent3` and `ui_scale_tests` successfully.
- Full CTest suite: **116/116 passed**, including seven new popup tests for
  resolution scaling, aspect ratio, centering, mouse boundaries and invalid
  scale handling.
- Ran the game at 1920×1080 on an isolated Xvfb display with Mesa llvmpipe,
  dummy audio and temporary preferences selected through `XDG_DATA_HOME`.
- Visually checked the enlarged Abort Mission popup over gameplay. Artwork,
  labels and buttons align, and the gameplay view retains its normal size.
- Clicked the enlarged **No** button and resumed gameplay. Released relative
  mouse capture with Ctrl+G for the synthetic X11 pointer test.
- Reopened the popup and used **N** to resume gameplay, then **Y** to abort.
- Verified the main menu and existing Options window after returning from
  gameplay; both retain their established size and layout.

Local screenshots are generated under `build/ui-popup-smoke/`:
`popup-1080p.png`, `mouse-no-result.png`, `keyboard-no-result.png`,
`menu-after-popup.png`, and `options-menu.png`.

1440p, 4K and ultrawide scaling were checked by automated coordinate tests;
they were not separately rendered in this smoke test. Windows and macOS
runtime validation remains outstanding.

## Initial scalable popup text

Enlarged message boxes now render their small UI font with bundled **Arimo
Bold 1.33**, using `stb_truetype` 1.26. Glyphs are rasterized from TrueType
outlines at the current popup scale, rather than magnifying the original
11-pixel bitmap glyphs. Original character advances, line heights, clipping,
capital letter shapes and input hit targets remain in use. Fonts drawn at
native scale and glyphs unavailable in the bundled font retain the bitmap path.

An antialiased ARGB4444 atlas is created on first use and reused by later
popups. Each opted-in font holds one atlas; changing resolution replaces it,
and freeing/resetting the font releases it. Atlas dimensions are limited to
2048×2048, with raster line height limited to 128 pixels (sufficient for the
standard 11-pixel popup font through 8K). Latin accents and Windows-1252
punctuation follow the existing single-byte text interface.

The unmodified TrueType file is embedded at build time, so rendering does not
depend on installed fonts, commercial asset changes, or workstation paths.
Its SIL Open Font License is included in `third_party/fonts/OFL.txt` and
installed as `popup-font-OFL.txt`; attribution is also in `THIRD_PARTY.md`.
The rasterizer only parses this bundled font, never arbitrary TTF files.

Validation on Linux, 2026-10-30:

- Debug and Release engine builds succeeded.
- Debug and Release CTest suites: **122/122 passed** in each configuration.
- Six font regression tests cover fresh detail at 4K, antialiasing, retained
  character cells, transparent atlas padding, accents/punctuation, uppercase
  shapes, invalid inputs, and bounded raster memory.
- All six font tests passed AddressSanitizer, UndefinedBehaviorSanitizer and
  LeakSanitizer. Sanitizer execution used the existing Clang 18 build outside
  the sandbox because LeakSanitizer cannot run under its ptrace restrictions.
- Visually verified the 1080p Debug Abort Mission popup, clicked **No**, reopened
  it and used **N**, then returned to the menu and checked its native-size
  confirmation font. Used an isolated Xvfb display, software OpenGL, dummy audio,
  and temporary preferences throughout.
- Visually verified the 3840×2160 Release popup at 4.5× scale, including sharp
  text and aligned button labels, and dismissed/reopened it with the keyboard.

Screenshots and logs: `build/ui-popup-smoke/scalable-1080p.png`,
`scalable-4k.png`, `scalable-mouse-no.png`, `scalable-menu-quit.png`, and
`scalable-ctest-*.log`.
Windows and macOS runtime validation remains outstanding.
