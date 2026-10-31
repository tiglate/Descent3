# Universal fonts and startup defaults

Engine-rendered UI text uses bundled Arimo Bold by default: menus, buttons,
labels, text entry, list boxes, dialogs, briefings, captions, the HUD, and
multiplayer overlays using the shared game fonts. The existing capital/small-cap
convention, character advances, line heights, and layout coordinates are retained.
Text embedded in textures, artwork, or movies is part of those assets and is
unaffected. No commercial assets or pilot file formats are changed.

Main-menu items use a dedicated **Orbitron Regular** face, designed by
**Matt McInerney**, retaining the original item metrics, shadow, and gold focus
effect. Orbitron is embedded from the unmodified upstream variable TTF at weight
400. Its SIL Open Font License and author credits accompany source and installed
builds; see [font credits and provenance](../third_party/fonts/README.md).
Other shared menu/message font users continue to use Arimo. The legacy-font
setting restores the original main-menu bitmap font too.

**Options > Video > High-res fonts** switches immediately between the scalable
font and original bitmap fonts. The preference is saved as `RS_highres_fonts`
and defaults to enabled when absent. Disabling it releases cached font textures.
Each game font caches up to four raster sizes, covering mixed HUD scales without
rebuilding textures each frame; individual atlases remain bounded at 2048×2048.
HUD text retains a dark shadow for readability over gameplay.

Main-menu labels have a black outline and offset shadow to remain readable over
the bright background. Pointer movement selects the hovered item and changes its
text to gold with a subtle amber focus animation. Keyboard navigation uses the
same focus color; a stationary pointer does not override keyboard selection.

Menus retain their 640×480 layout and input coordinate system but render into
a buffer fitted to the actual display pixels. This avoids enlarging already
rasterized text. Aspect ratio, letterboxing, 3D previews, rectangle fills,
and screenshots use the same buffer dimensions. Gameplay keeps its selected
render resolution, including intentionally lower resolutions.

The in-game F1 keyboard-help window opts into the same uniform screen scaling as
message boxes. Its 448×384 layout remains unchanged at 640×480; at 1920×1080 it
uses scale 2.25 (1008×864), staying centered with correspondingly scaled button
targets and freshly rasterized text. Widescreen displays do not stretch it.

## UI artwork seams

The OpenGL texture cache now records newly created textures as repeating, matching
their actual initial sampler state. A first draw that requests edge clamping then
applies it correctly. Previously the cache incorrectly reported clamping already
enabled, so filtered UI tiles sampled their opposite edges and exposed horizontal
and vertical lines around list-box corners. Lightmap wrap updates also use the
effective requested mode, keeping the cached and actual sampler states consistent.
The fix retains bilinear filtering and does not change artwork or tile geometry.

## Fresh configuration

| Setting | Default | Saved choice |
| --- | --- | --- |
| Fonts | High resolution | Global preference |
| Resolution | Highest pixel count supported by the selected display | Video selection stored as width/height |
| Detail | Highest, including full terrain distance and object detail | Preset/custom settings |
| Color/texture filtering | 32-bit, full texture detail, bilinear, mipmapping | Existing Video settings |
| Audio quality/voices | High, maximum mixed sounds | Existing Sound settings |
| New pilot controls | Modern bindings and Mouselook | Existing pilot bindings and mouse mode |

Automatic saves do not turn the default resolution into an explicit selection.
Only accepting a choice in the resolution dialog stores `RS_width`/`RS_height`.
Command-line dimensions override one launch. Missing or unavailable saved modes
fall back to the highest supported mode; unstable list indices are no longer
used for startup selection. Existing saved graphics values and pilot controls
are read over the new defaults.

The Highest detail preset now reaches the UI's maximum terrain distance and
minimum pixel error, enables the quality effects, and uses the full-quality
headlight path. Terrain slider conversion now round-trips its supported range
without reducing quality simply by opening and closing Settings.

## Validation

Linux, isolated Xvfb display, Mesa software OpenGL, dummy audio, and temporary
preferences under `build/ui-fonts-smoke/`:

- Debug and Release engine builds succeeded; both CTest suites passed **132/132**.
- Sixteen font, buffer-sizing, and default-selection tests passed with
  AddressSanitizer, UndefinedBehaviorSanitizer, and LeakSanitizer.
- Started with no registry or pilots: selected 1920×1080 without CLI dimensions;
  checked filtering, mipmapping, Highest detail, and enabled font selection.
- Created a disposable pilot through the edit dialog; verified serialized
  W/S/A/D, Space/Ctrl bindings and enabled Mouselook without changing the format.
- Switched fonts off in Video, exited, restarted, and verified the legacy choice;
  switched back on and verified the immediate visual change.
- Accepted the resolution dialog and verified saved width/height values. Selected
  Medium detail in Settings to exercise preservation of an explicit lower-quality
  preference across restart, rather than resetting it to the new maximum default.
- Checked briefing wrapping, text entry, in-game HUD text, popup layout,
  maximum-detail controls, and returning from gameplay to menu mode.
- An in-game menu screenshot produced a 1440×1080 PNG on the 1920×1080 display,
  confirming the native render area and screenshot readback agree.
- Checked main-menu shadows against the bright background at 1920×1080,
  pointer hover across multiple items, keyboard navigation with the pointer
  stationary over another item, and clicking the highlighted Options item.
  Also checked the legacy font and restored high-resolution fonts afterward.
- Reproduced list-box tile seams at 1920×1080 with bilinear filtering enabled,
  then verified clean Pilots borders on first opening and reopening after the
  texture-cache correction. In an empty 360×280 interior region, stray nonblack
  pixels fell from 280 to zero in both corrected captures. Also checked Settings
  artwork. Before/after screenshots use the `tile-seams-` prefix in the same local
  screenshot directory. Debug/Release builds and all 130 Release tests passed.
- Checked Orbitron main-menu rendering and gold hover at 1920×1080, toggled
  legacy fonts and restored high-resolution fonts, and verified all three font
  notice/credit files install to a temporary staging directory. Orbitron tests
  check distinct glyphs, preserved layout cells, localized text, and 4K rendering.
- Opened F1 help in 1920×1080 gameplay: verified all rows fit the centered enlarged
  panel and clicking its visible scaled OK button returns to gameplay. Previews:
  `orbitron-main-menu-hover.png` and `scaled-help-1080p.png`.

Local screenshots and logs are in `build/ui-fonts-smoke/`. Windows/macOS runtime
and live multiplayer sessions were not tested. The earlier popup-specific
implementation and 4K checks are recorded in [ui-popup-scaling.md](ui-popup-scaling.md).
