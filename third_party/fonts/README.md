# Bundled fonts

## Arimo Bold

`Arimo-Bold.ttf` is the unmodified Arimo Bold 1.33 TrueType font from Google's
Croscore font distribution. Copyright 2010 Google Inc. All Rights Reserved.
See [OFL.txt](OFL.txt) for the SIL Open Font License 1.1.

Upstream: https://github.com/notofonts/noto-fonts/tree/main/hinted/ttf/Arimo

SHA-256:
`10724c3733f60cdec733da1a439de9100e1593ceb219cc4d03c443bef1feb9b6`

CMake embeds the font into the executable using `cmake/embed_font.cmake`.
`grtext/scalable_font.cpp` rasterizes it at the current display and text scale
for all built-in game fonts. No operating-system font discovery or runtime TTF file loading
is used. The original font file and its license must accompany source releases;
the license is installed as `popup-font-OFL.txt` alongside the game documents.

## Orbitron

The main-menu face is **Orbitron**, designed by **Matt McInerney**.
Copyright 2018 The Orbitron Project Authors (https://github.com/theleagueof/orbitron),
with Reserved Font Name: "Orbitron".
See [Orbitron-OFL.txt](Orbitron-OFL.txt) for the complete SIL Open Font License 1.1.

`Orbitron.ttf` is the unmodified upstream `Orbitron[wght].ttf` variable font,
version 2.001, rendered at its default Regular weight (400). Renaming the file
does not modify the font data or its internal family name.

- Specimen: https://fonts.google.com/specimen/Orbitron
- Distribution: https://github.com/google/fonts/tree/main/ofl/orbitron
- Font: https://raw.githubusercontent.com/google/fonts/main/ofl/orbitron/Orbitron%5Bwght%5D.ttf
- License: https://raw.githubusercontent.com/google/fonts/main/ofl/orbitron/OFL.txt
- Retrieved: 2026-10-31
- SHA-256: `f42db2dd16e642258e35782916eceb1dcdbea06fb958d77ad71dc5963587e8fd`

The font is embedded using the same CMake script and CPU rasterizer as Arimo.
Only the dedicated main-menu font handle selects this face; the legacy-font
setting restores the original bitmap font. The original TTF and license accompany
source releases. Binary installs include the notice and license as
`orbitron-font-OFL.txt` and this author/provenance document as `bundled-fonts.md`.
