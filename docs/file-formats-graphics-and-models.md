# Descent 3 graphics, animation, font, and object formats

## Scope and authority

This is a decoder-oriented description of **OGF (Outrage Graphics Format)**,
**OOF (Outrage Object Format)**, **OAF (Outrage Animation Format)**, and the
engine's bitmap **FNT** format. It describes the implementation at commit
**8b4c924ceebe284d06604b2e985cd839ae4329d5**, rather than an assumed universal
Outrage interchange standard. No commercial asset data is reproduced.

The companion [containers and game-data specification](file-formats-game-data.md)
covers archives, levels, missions, tables, audio, and persistent game data.
Imported TGA, PCX, PNG, WAV, and TrueType files retain their external formats;
their extensions do not make them Descent-specific formats.

### Contents

- [Primitive encodings](#primitive-encodings)
- [OGF images](#ogf--outrage-graphics-format)
- [OAF animations](#oaf--outrage-animation-format)
- [FNT bitmap fonts](#fnt--outrage-bitmap-fonts)
- [OOF objects](#oof--outrage-object-format)
- [Assembling and displaying an OOF](#assembling-and-displaying-an-oof)

Tables below list fields in **physical file order**. Array elements are contiguous
and have no implicit alignment or padding. Conditional fields occupy no bytes
when their condition is false. All counts must be checked against both remaining
input bytes and an implementation's allocation limits before allocating memory.

### Primitive encodings

| Notation | Bytes | Encoding |
| --- | ---: | --- |
| U8 / I8 | 1 | Unsigned / signed two's-complement integer |
| U16 / I16 | 2 | Little-endian unsigned / signed integer |
| U32 / I32 | 4 | Little-endian unsigned / signed integer |
| F32 | 4 | Little-endian IEEE-754 binary32 |
| Vec3 | 12 | Three F32 values, X followed by Y followed by Z |
| FourCC | 4 | Four literal bytes in the order printed; no terminator |
| ZString | variable | Bytes through and including the first zero byte |
| FixedString(N) | N | Exactly N bytes; interpret through the first zero and retain/skip remaining padding |
| ModelString | 4 + N | I32 byte count N, followed by exactly N bytes; model readers expect a zero-terminated string inside these bytes |

Strings are legacy byte strings, not intrinsically UTF-8. For a lossless decoder,
retain the original bytes; choose a text encoding separately. Empty ZString is
one zero byte. ModelString length is a byte count, not a character count or an
offset. Include its terminator in the counted bytes when producing compatible
files. A negative count or a string extending beyond its enclosing record is
invalid. There is no automatic four-byte alignment after a string or RGB triple.

### Packed colors

| Encoding | Bit layout, most significant first | Conversion to RGBA |
| --- | --- | --- |
| ARGB1555 | A:1, R:5, G:5, B:5 | A is 0 or 255; expand each color component from 0–31 to 0–255 |
| ARGB4444 | A:4, R:4, G:4, B:4 | Multiply each 0–15 component by 17 |
| RGB565 | R:5, G:6, B:5 | Special legacy color-key rules below; no alpha bit |

For visualization, expanding five-bit values with bit replication or rounding
to the full 0–255 range is appropriate. The engine's legacy conversion instead
shifts five-bit components left by three and six-bit green left by two before
quantizing to its internal 1555 representation. Retain packed samples if exact
round-tripping matters. A fully opaque red ARGB1555 sample is 0xFC00, stored as
bytes **00 FC**, least significant byte first. The engine's transparent 1555 value is
0x0000; its opacity bit is 0x8000.

## OGF — Outrage Graphics Format

### Identification and supported profiles

OGF is a modified TGA-family stream. It does **not** start with the letters OGF.
Its third byte is the image type. Standard TGA readers generally cannot decode
it because Outrage inserts a name and sometimes a mip count into the header,
and uses a different pixel compression scheme.

| Type, decimal / hex | Header name | Mip-count byte | Compressed sample encoding | Status in this engine |
| --- | --- | --- | --- | --- |
| 121 / 0x79 | ZString | Yes | ARGB4444 | Supported; current writer uses this for 4444 bitmaps |
| 122 / 0x7A | ZString | Yes | ARGB1555 | Supported; current writer's normal output |
| 123 / 0x7B | ZString | Yes | Legacy RGB565, green color key | Supported compatibility input |
| 124 / 0x7C | FixedString(35) | Yes | Legacy RGB565, green color key | Supported compatibility input |
| 125 / 0x7D | Historical declaration only | Not a supported profile | An obsolete 8-bit variant | Rejected by the active header validator; do not invent a palette decoder |
| 126 / 0x7E | FixedString(35) | No | Historical Outrage TGA variant | Header recognition exists, but no active pixel-decoding branch implements it |
| 127 / 0x7F | FixedString(35) | No; implicitly 1 | Legacy RGB565, green color key | Supported compatibility input |

Types 2 and 10 handled by the same loader are ordinary uncompressed and RLE
true-color TGA, respectively. They do not contain the inserted Outrage name or
mip count. Their TGA packet rules must not be used for types 121–124 or 127.

### Header, in order

| Field | Encoding | Requirement / meaning |
| --- | --- | --- |
| Image ID length I | U8 | Writer emits zero; decoder must skip I bytes at the location specified below |
| Color-map type | U8 | Must be zero; no palette follows |
| Image type | U8 | Select the profile above |
| Bitmap name | Profile-specific string | Present only for Outrage types |
| Stored mip count M | U8, when profile includes it | Writer emits 1 for an unmipped image; otherwise includes the base level in the count |
| TGA compatibility fields | 9 bytes | Color-map specification and image-origin bytes; writer emits all zeroes; loader ignores them |
| Base width W | U16 | Positive |
| Base height H | U16 | Positive |
| Nominal pixel depth | U8 | Loader accepts 24 or 32; current writer emits 32 even though compressed samples are 16-bit |
| Image descriptor | U8 | Low nibble must be 0 or 8; writer emits 0x28 |
| Image ID | I bytes | Opaque metadata, immediately before compressed pixels |
| Pixel stream | variable | Decode M levels, or one when no count is present |

For a modern file whose name occupies N bytes **including** the zero terminator,
width begins at offset 13 + N and the first compressed packet begins at offset
19 + N + I. For type 127, the corresponding offsets are 47 and 53 + I.
For type 124 they are 48 and 54 + I. These offsets are from the start of this OGF,
including when an OGF is embedded in OAF or a save snapshot.

The descriptor's 0x20 bit denotes top-origin in normal TGA. The Outrage compressed
branch writes/reads rows in their stored order and does not apply the normal TGA
vertical-flip path. Compatible Outrage output is top-to-bottom, left-to-right,
with descriptor 0x28. Do not flip a compressed OGF merely because a generic TGA
library would do so. The loader does not implement horizontal-origin reversal.

### Compression

Each packet occupies **three bytes**: a U8 command followed by one little-endian
U16 sample. Decode commands as follows:

| Command | Pixels produced |
| --- | --- |
| 0 | One copy of the following sample |
| 2 through 250 inclusive | Exactly that many copies of the following sample |
| 1, 251 through 255 | Invalid/reserved in the active decoder |

There are no raw blocks containing several different samples, no packet lengths
minus one, and no high-bit repeat flag. In particular, 0x80 means **128 repeats**,
not a TGA control bit. Runs may cross scanline boundaries. For each level, stop
after exactly that level's pixel count; a packet may not overrun it. The next
byte is the first packet of the next level. There is no end marker, compressed
size, scanline table, or padding between levels.

For example, the three bytes **03 00 FC** produce three opaque red pixels in
type 122. **00 00 00** produces one transparent pixel in type 122. In type 121,
**00 00 F0** produces one opaque black pixel, because its sample is 0xF000.

For legacy types 123, 124, and 127, the sample is RGB565. Exactly **0x07E0** is
transparent. Every other sample is opaque, including 0x0000. To match the engine's
1555 conversion, discard the least significant green bit and set the opacity bit.
For 121 and 122, do not apply the legacy green color key.

### Mipmap levels and stream boundaries

M includes level zero. Level L has width W shifted right by L and height H shifted
right by L. Require both to remain positive for every stored level; this loader
does not clamp a zero dimension to one. Normal assets use square, power-of-two
dimensions. Base data comes first, followed by successively smaller levels.
Stop at the number of stored levels, not at end of the containing file.

When several stored levels do not extend all the way down to 1×1, the engine can
generate its remaining runtime levels from the preceding level. Those generated
levels are not additional bytes in the file. Preserve the distinction between
stored images and generated images in a decoder's output.

A file with no mip-count field has one level. A count of zero is not sensible
writer output; reject it rather than treating it as a zero-length image. Overflow
in W × H, impossible mip counts, truncated three-byte packets, and runs crossing
the expected level boundary are malformed inputs. Successful decoding should
return both the images and the exact number of bytes consumed; OAF depends on
this property. Do not consume all remaining input as one OGF.

### OGF sources

[Type constants](../lib/bitmap.h),
[header and packet reader](../bitmap/tga.cpp),
[writer and mip dimensions](../bitmap/bitmain.cpp), and
[internal color definitions](../lib/grdefs.h).

## OAF — Outrage Animation Format

OAF is an ordered sequence of embedded OGF images, not a container with offsets
or independent compressed frame sizes. The active writer emits the following
version-1 layout:

| Field | Encoding | Meaning |
| --- | --- | --- |
| Marker | U8 | 127 / 0x7F |
| Version | U8 | 1 |
| Frame count F | U8 | Number of following images |
| Frame time | F32 | Nominal seconds per frame |
| Frames | F consecutive OGF records | Each has its own dimensions, name, mip count, and compressed pixel stream |

The modern header is seven bytes. Decode each OGF through its exact declared
pixel/mip counts, then begin the next frame at the next byte. There is no
inter-frame padding. Do not identify an OAF solely from first byte 127, since
that is not a unique file signature.

If the first byte is **not** 127, the loader treats it as the legacy frame count.
It then reads F32 play time, F32 frame time, I32 flags, and F32 light value, in
that order, before the OGF frames: a 17-byte legacy header. The current loader
reads but does not validate the modern version byte. A new decoder should report
unsupported versions explicitly rather than assume their fields are identical.

The gameplay loader replaces the stored frame time with **0.07 seconds**, in
both header variants. Decode and retain the stored value; applying this playback
override is an optional engine-compatibility behavior, not part of decompression.

Source: [OAF writer and frame reader](../Descent3/vclip.cpp).

## FNT — Outrage bitmap fonts

This format is unrelated to Windows FNT and text-based BMFont files. Its first
four bytes are **BA BA ED FE**, the little-endian U32 value 0xFEEDBABA.

### Header and optional tables

| Field | Encoding | Meaning |
| --- | --- | --- |
| Signature | U32 | 0xFEEDBABA |
| Nominal width | I16 | Fixed character width, or nominal/max width for proportional fonts |
| Height H | I16 | Common glyph cell height |
| Flags | U16 | Bit definitions below |
| Baseline/brightness | U16 | Low byte contains legacy baseline information; high byte divided by 10 gives brightness |
| First character A | U8 | First byte value represented |
| Last character B | U8 | Last byte value represented, inclusive |
| Font name | FixedString(32) | Informational name |
| Tracking | I16, only if flag 0x20 | Character tracking |
| Extended reserved area | 62 bytes, only if flag 0x20 | Preserve or ignore |
| Widths | B − A + 1 I16 values, only if flag 0x02 | Width for each character in ascending character order |
| Kerning count K | I16, only if flag 0x04 | Number of following triples |
| Kerning entries | K × 3 bytes, only if flag 0x04 | U8 first character, U8 second character, I8 spacing adjustment |
| Pixel-data length D | I32 | Byte length of the entire following glyph-data block |
| Glyph data | D bytes | Cells concatenated in ascending character order |

The fixed header, including signature and name, is **46 bytes**. The extended
tracking/reserved block is exactly 64 additional bytes. The kerning sentinel
used by the engine is synthesized in memory and is **not** an extra file entry.
Character values in the kerning table are the stored character byte values.

| Flag | Meaning |
| --- | --- |
| 0x01 | Color font; otherwise monochrome packed bits |
| 0x02 | Proportional widths table present |
| 0x04 | Kerning table present |
| 0x08 | Gradient-font rendering behavior; does not add fields |
| 0x10 | Color pixels use ARGB4444; otherwise legacy RGB565 |
| 0x20 | 64-byte extended font information present |

For monochrome fonts, each glyph has H rows, each rounded up to a whole number
of bytes: ceiling(width / 8) bytes per row. The leftmost pixel is the high bit
(0x80) of its byte. One means ink and zero means transparent. Row padding bits
are unused; a new glyph starts at the next byte after the last padded row.

For color fonts, each glyph is width × H little-endian U16 samples in row-major
order, without row padding. With 0x10 set, samples are ARGB4444. Without it, they
are RGB565 and exactly 0x07E0 means transparent; other values become opaque.
The renderer drops the lowest green bit when converting to internal 1555.

Validate B ≥ A, positive dimensions, nonnegative counts/lengths, and that all
computed cell lengths fit within D. Current runtime proportional widths narrow
to eight bits, so widths outside 0–255 are not useful compatible output. Flags
and rendering decisions are not additional pixel bytes. The renderer can use
capital shapes for lowercase text when B is below lowercase 'a'; this does not
create additional glyph records on disk.

The scalable fonts added to this checkout use embedded standard TrueType files.
They do not change the FNT serialization described here.

Sources: [font reader, kerning, and pixel translation](../grtext/grfont.cpp) and
[flags and extended header](../grtext/grtext.h).

## OOF — Outrage Object Format

### Header, versions, and chunks

| Offset | Field | Encoding | Meaning |
| ---: | --- | --- | --- |
| 0 | Signature | FourCC | **PSPO**: bytes 50 53 50 4F |
| 4 | Version V | I32 | Usually major × 100 + minor; current upper limit 2300 |
| 8 | Chunk stream | repeated records | Through physical end of the model |

Do not reverse FourCC strings printed here. Some source constants are written
backwards as C/C++ multicharacter literals because they are compared as
little-endian integers; the actual file bytes are the readable strings here.

The reader multiplies versions below 18 by 100, then accepts **1807 through
2300 inclusive**. Consequently that historical normalization does not make
versions 1–17 usable by the current reader. A new decoder should report the
raw and normalized version, and reject unsupported layouts explicitly.

Each chunk consists of a FourCC ID, I32 payload length L, and exactly L payload
bytes. **L excludes the eight-byte chunk header.** Unknown chunks can be skipped
by L. Negative lengths, arithmetic overflow, and chunks extending past the
containing model are invalid. No mandatory chunk padding or file terminator
exists. For known chunks, the reference loader consumes the fields directly;
do not add undocumented trailing fields to a known version's chunk.

| Version condition | Layout effect |
| --- | --- |
| V > 1805 | Subobject geometric-center vector present; therefore present throughout the currently accepted range |
| V ≥ 1908 | Gunpoint records include an I32 parent subobject |
| Major ≥ 21 | Each face ends with two F32 lightmap-resolution values |
| Major ≥ 22 | Animation tracks are timed per subobject rather than sharing a global key count |
| Major ≥ 23 | Per-vertex F32 alpha array follows all vertex normals |

### Chunk directory

| File ID | Payload |
| --- | --- |
| OHDR | Object header; required before subobjects and animation |
| SOBJ | One indexed subobject, including geometry and properties |
| TXTR | Model-local texture-name table |
| GPNT | Gunpoints |
| ATCH | Attachment points |
| NATH | Attachment normal/up-vector extension |
| WBAT | Weapon-battery gunpoint and turret indices |
| GRND | Ground contacts |
| RANI / ANIM | Rotation animation; same decoder for either ID |
| PANI | Position animation |
| IDTA, PINF, GRID | Declared legacy/exporter metadata IDs; active new-model reader skips their payloads by length |
| Any other ID | Preserve as an opaque, length-delimited chunk or skip |

The attachment ID deserves care: the actual bytes are **ATCH**. APNT is not an
alias implemented by this reader.

### OHDR — object header

| Field | Encoding |
| --- | --- |
| Subobject count S | I32 |
| Object radius | F32 |
| Bounding-box minimum | Vec3 |
| Bounding-box maximum | Vec3 |
| Detail-level count D | I32 |
| Detail subobject indices | D × I32 |

The active reader consumes but ignores the detail list. Indices elsewhere in the
file refer to this model's subobjects or local texture/gunpoint tables; they are
not global game-resource handles. Require one OHDR and a usable nonnegative S.
The hierarchy is established from each SOBJ parent. Supply subobjects before
animation chunks so their geometry and animation metadata exist.

### SOBJ — subobject, fixed prefix

| Field, in order | Encoding | Interpretation |
| --- | --- | --- |
| Subobject index | I32 | Index 0 through S − 1 |
| Parent index | I32 | −1 for a root; the loader also converts self-parenting to a root |
| Plane normal | Vec3 | Stored subobject plane/rotation-axis information |
| Plane distance | F32 | Consumed but unused by this reader |
| Plane reference point | Vec3 | Stored point |
| Parent-relative offset | Vec3 | Pivot/placement offset in parent coordinates |
| Radius | F32 | Subobject radius |
| Tree offset | I32 | Legacy/exporter field; not a seek target for the following arrays |
| Data offset | I32 | Legacy/exporter field; not a seek target for the following arrays |
| Geometric center | Vec3, if V > 1805 | Otherwise treated as zero |
| Name | ModelString | Subobject name |
| Properties | ModelString | One recognized property expression, described below |
| Movement type | I32 | Retain stored value; no conditional fields follow from it |
| Movement axis | I32 | Retain stored value |
| Free-space chunk count C | I32 | Count of legacy/exporter integers |
| Free-space entries | C × I32 | Consumed and ignored |
| Vertex count N | I32 | Number of positions and normals |
| Positions | N × Vec3 | Entire position array first |
| Normals | N × Vec3 | Entire normal array second |
| Alpha values | N × F32, only if major ≥ 23 | Alpha per vertex; older versions imply 1.0 |
| Face count F | I32 | Number of following face records |
| Faces | F records | Layout below |

Positions and normals are separate arrays, not interleaved. Positions are local
to the subobject's pivot, and parent offsets are hierarchical. Preserve these
local coordinates in decoded data; apply the transforms specified in
[assembling and displaying an OOF](#assembling-and-displaying-an-oof) to render it.

Each face has the following layout; the three RGB bytes intentionally leave the
following value unaligned when a face is untextured:

| Field | Encoding | Condition |
| --- | --- | --- |
| Face normal | Vec3 | Always |
| Face vertex count K | I32 | Always |
| Textured flag | I32 | Zero means RGB face; nonzero means texture index follows |
| Texture index | I32 | Only when textured; indexes TXTR |
| Flat red, green, blue | Three U8 values | Only when untextured |
| Corner records | K records | Each: I32 vertex index, F32 U, F32 V |
| Lightmap width factor | F32 | Only when major ≥ 21; consumed but not retained by this loader |
| Lightmap height factor | F32 | Only when major ≥ 21; consumed but not retained by this loader |

Corner indices refer to the subobject position array and preserve polygon order.
UVs are per corner, allowing seams without duplicating positions. Do not infer
another vertex count or extra normal array inside each face. Untextured faces
still have the K index/UV records. Faces need not be triangles.

Engine limits include fewer than 2500 vertices per subobject, fewer than 100
corners per face, and fewer than 35 model textures. Face indices narrow to 16 bits
in memory despite occupying four bytes in the file. Portable tools may preserve
larger well-formed data, but should report when it exceeds the game's limits.

### TXTR — model-local texture names

I32 texture count T, followed by T ModelStrings. A textured face's I32 texture
index selects one entry. Names conventionally omit an extension; the engine
first looks for a game texture whose bitmap matches the name with **.OGF**
appended, then looks for a game texture with the original name. This
chunk names external resources; it does not embed images or materials. A decoder
can return the strings without access to game assets or the resource table.

### Gunpoints, attachment points, and ground contacts

| Chunk | Prefix | Repeated record |
| --- | --- | --- |
| GPNT | I32 count G | I32 parent if V ≥ 1908, Vec3 position, Vec3 normal |
| ATCH | I32 count A | I32 parent, Vec3 position, Vec3 normal |
| GRND | I32 count G | I32 parent, Vec3 position, Vec3 normal |
| NATH | I32 count A′ | I32 associated integer, Vec3 normal-like vector, Vec3 up vector |

GPNT versions before 1908 imply parent zero. Positions and direction vectors
belong to the associated subobject's coordinate system. Retain the NATH integer
and first vector even though the loader discards them and pairs entries with
ATCH by **array order**. It uses the last vector as attachment up direction.
NATH must follow ATCH, and its count should equal the ATCH count. Reject a count
mismatch for faithful decoding rather than reproduce the reader's unsafe
mismatch path. NATH is supplemental orientation information, not another set
of attachment positions.

### WBAT — weapon batteries

I32 battery count B, followed by B records. Each record contains:

1. I32 gunpoint count G, then G I32 gunpoint indices into GPNT.
2. I32 turret count T, then T I32 subobject indices.

The engine consumes all indices but retains only the first eight of each list.
An independent decoder should return the complete file lists and separately
report that truncation. Neither list contains embedded gunpoint geometry.

### RANI / ANIM — rotation animation

For major versions **18 through 21**, read one I32 global key count K, then
S tracks in subobject-index order. Each track has K records, each consisting of
Vec3 axis followed by I32 angle. There is no subobject ID or count before an
individual track in this profile.

For major versions **22 and 23**, there is no global count. For each of S
subobjects, read the following:

| Field | Encoding |
| --- | --- |
| Key count K | I32 |
| Track minimum tick | I32 |
| Track maximum tick | I32 |
| Keys | K records: I32 start tick, Vec3 axis, I32 angle |

Axes are normalized for use when nonzero. Angles are stored in a four-byte
integer, but the rotation routine takes a 16-bit engine angle: **65536 units
per turn**, not degrees or radians. Preserve the I32 value and use its low
16 bits for compatibility rotation. The keys describe successive axis-angle
rotations: the engine accumulates a key's rotation with the prior accumulated
matrix, beginning at identity. Treating every key as an independent absolute
Euler orientation produces different animation.

Track min/max and start ticks are integer animation time units, not necessarily
seconds. The file alone does not supply the gameplay animation's seconds-per-
range; game-object/battery metadata controls playback. Validate monotonic key
times and that track ranges support the requested interpolation.

### PANI — position animation

For majors **18 through 21**, I32 global key count K followed by S tracks,
each containing K Vec3 positions. For majors **22 and 23**, each of S tracks
contains I32 key count, I32 minimum tick, I32 maximum tick, then K records of
I32 start tick and Vec3 position. There is no orientation or angle in a PANI
key. As with RANI, tracks occur in subobject-index order.

### Subobject property strings

Properties are text inside the SOBJ ModelString; they are not extra chunks or
a bit field following the string. Matching is case-insensitive. The active
parser extracts one command through the first equals sign; it does not implement
a list of arbitrarily concatenated commands.

| Property | Data / meaning |
| --- | --- |
| $rotate= | Decimal seconds per revolution, greater than zero and at most 20; runtime stores its reciprocal |
| $jitter | Mark jittering geometry |
| $shell | Mark a door shell |
| $facing | Mark view-facing geometry; runtime expects one face |
| $frontface | Mark a door front |
| $glow= | Four comma-separated decimal values: red, green, blue, size |
| $thruster= | Four comma-separated decimal values: red, green, blue, size |
| $fov= | Three comma-separated values: field of view in degrees, seconds per revolution, reaction time in seconds |
| $monitor01 through $monitor08 | Cockpit monitor surface assignments |
| $viewer | Viewer subobject |
| $layer | Layered rendering subobject |
| $custom | Custom-texture surface |

For turret data the reader bounds FOV to 0–360, rotation period to 0–60, and
reaction time to 0–10. A zero rotation period is problematic for its reciprocal;
preserve/report it instead of generating an infinite speed. Unknown properties
can be retained without preventing geometry decoding.

### OOF decoder checklist

1. Validate the signature and version before interpreting chunks.
2. Establish an end boundary for every chunk and check every variable-length read.
3. Decode OHDR, then SOBJ geometry and hierarchy; collect TXTR names independently
   of whether textures are installed.
4. Decode optional points/batteries and version-selected animation tracks.
5. Validate unique subobject indices, parent bounds and cycles, vertex/texture/
   gunpoint references, count consistency, and finite numeric data before use.
6. Return unknown chunk bytes, ignored fields, original packed colors, and property
   strings if lossless round-tripping is a goal. Runtime-generated bounding boxes,
   child lists, GPU buffers, and animation remap tables are not serialized arrays.

Sources: [OOF reader and property/animation interpretation](../model/polymodel.cpp),
[supported versions](../model/polymodel.h), and
[engine limits](../lib/polymodel_external.h).

## Assembling and displaying an OOF

This section specifies how the decoded records become a scene. The baseline is
the ordinary whole-model rendering path at the commit stated above. A standalone
viewer can display geometry without running gameplay, but OOF alone does not
specify its camera, world placement, lighting environment, animation-state
durations, texture database, or cockpit display contents. Those are inputs to
the renderer.

### Rendering stages

**Decode records → build the parent forest → evaluate a pose → compose transforms
→ resolve materials → triangulate faces → camera/clip/project → draw surfaces
→ draw billboards and glows.**

Build one node per SOBJ index, independent of chunk order, and connect nodes
using their parent indices. There can be several roots. Ordinary whole-model
drawing starts at every root; it does not draw just SOBJ zero. The OHDR detail
list is ignored by this engine and is not an instruction to display only one
listed subobject. Game-resource metadata can select a separate OOF for a lower
detail level.

For a first static display, use identity node rotations and zero animation
displacements, retain all SOBJ offsets, and supply identity object orientation.
This is the unanimated geometry pose. Sampling animation at normalized time zero
is a separate operation: a first animation key need not be identity. Do not
recenter each part separately or normalize each part to the same radius.

### Coordinates, bases, and camera

The engine's identity basis is **+X right, +Y up, +Z forward**. An identity
camera looks along +Z, and points in front have positive camera Z. A screen's
pixel Y normally increases downward; that screen conversion is separate from
the model's +Y-up coordinates. File coordinates have no declared SI unit, and
OOF contains no per-node scale or camera FOV.

To avoid row/column and handedness ambiguity, every rotation matrix in this
section uses **column vectors**. Let a basis matrix B have right, up, forward
as its three columns. Transforming local coordinates v into the enclosing
coordinates means **B v**, equivalently x·right + y·up + z·forward. Transforming
an enclosing direction into that local frame means **Bᵀ v** for an orthonormal
basis. The source's vector-times-matrix operator takes dot products with the
three stored basis vectors and therefore performs this latter operation;
copying that operator's spelling into a different math library can invert a
rotation accidentally.

Cross products below use the usual component rule, with +X × +Y = +Z. A positive
quarter-turn about +Z maps (1,0,0) to (0,1,0); a positive quarter-turn about +Y
maps (0,0,1) to (1,0,0). These tests fix the rotation sign without relying on a
graphics API's description of its coordinate system.

Let the camera have position e and basis Bc. A world point p has camera
coordinates **q = Bcᵀ (p − e)**. For a standalone perspective viewer with vertical
field of view φ and viewport aspect a = width/height, one usable projection is:

| Quantity | Equation |
| --- | --- |
| Horizontal normalized coordinate | nx = qx / (qz · a · tan(φ/2)) |
| Vertical normalized coordinate | ny = qy / (qz · tan(φ/2)) |
| Pixel X | width · (1 + nx) / 2 |
| Pixel Y, top-down framebuffer | height · (1 − ny) / 2 |

This projection is a viewer choice, not an OOF field or a definition of the
game's zoom parameter. Clip polygons against a positive near distance and the
other frustum planes before dividing by Z. Merely discarding a triangle with
one vertex behind the camera produces holes. Interpolate UVs, shading values,
and alpha perspective-correctly during rasterization.

For a concrete initial camera, compute the assembled bounds' center c and a
positive enclosing-sphere radius r. Set Bc to identity and place e at
**c − (0,0,d)**, so the camera looks toward the model along +Z. Choose d greater
than **r + r/tan(β)**, where β is the smaller of the vertical half-FOV φ/2 and
the horizontal half-FOV atan(a·tan(φ/2)). This fits even the bounds nearest the
eye, with some extra distance providing a margin. Choose a positive near plane
less than d−r and a far plane greater than d+r. Orbiting this camera changes
its basis as well as its position; do not keep an identity basis after moving
to the opposite side of the model.

An API whose camera looks along −Z needs an appropriate view/basis conversion.
For an explicit model-coordinate conversion S, transform positions, offsets,
animation displacements, and point records consistently; rotations become
**S R S⁻¹**. Build rotations in the original coordinates before this conversion.
For an orthogonal S, converting an axis while retaining the angle instead
requires **anew = det(S) S a**; a reflection's rotation axis is an axial vector,
not an ordinary direction. A reflection, such as negating just Z, reverses
winding. Reverse triangle order or adjust front-face state accordingly. Transform normals by
the inverse transpose of the linear transform and normalize them. Do not
negate only the mesh positions while leaving offsets and animation unchanged.

### Subobject transforms and pivots

For node i, let oi be its SOBJ offset, di its evaluated PANI displacement, and
Ri its evaluated rotation in the column-vector convention above. Its local
point v becomes a point in the parent frame as follows:

**vparent = Ri v + oi + di.**

Thus rotation acts about the local origin, followed by translation in the
**parent's** axes. Do not rotate oi or di by Ri. The parent can subsequently
rotate both translations as part of its whole child transform. The static
offset and the animation displacement are additive, not alternative positions.

Let T(t) denote translation by t, and let Hi be the corresponding homogeneous
transform **T(oi + di) Ri**. With W as the externally supplied model-to-world
transform, the complete node transform is:

| Node | Local-to-world transform |
| --- | --- |
| Root i | Gi = W Hi |
| Child i of parent j | Gi = Gj Hi |
| Vertex of i | pworld = Gi v |
| Direction/normal, rotations only | nworld = rotational-part(Gi) nlocal |

The root's offset also applies. Compose each ancestor exactly once. A node's
geometric center, plane reference point, plane distance, tree offset, and data
offset do **not** introduce further transforms in the ordinary mesh path.
Neither movement type/axis nor the subobject plane normal supplies a hidden
rest-orientation matrix. Each SOBJ is a rigid mesh node: the file has no
per-vertex bone indices or skinning weights, and a vertex moves with its own
node rather than blending transforms from several nodes.
In particular, do not subtract geometric center from every vertex and then
also apply SOBJ offset. The game's isolated-fragment drawing path can explicitly
recenter a fragment for debris; that is different from whole-model assembly.

GPNT, ATCH, GRND, and NATH data are not visible polygons. Transform GPNT/ATCH/GRND
positions through their parent subobject's Gi; transform their direction vectors
with its rotation only. NATH's up vector uses the corresponding ATCH entry's
parent, matched by array order; its ignored integer is not a replacement parent
index. This keeps gun muzzles, attachments, and ground contacts aligned with the
animated mesh. A parent of −1 denotes the model frame where permitted by the
consuming subsystem; do not index a node array with it.

### Mesh construction, normals, and visibility

Each face corner selects a position and vertex normal from its SOBJ arrays,
but supplies its **own U,V**. Vertex alpha comes from the selected position
index, defaulting to 1 before version 2300. A GPU vertex must therefore include
position, normal, UV, and alpha. Expand the corner records or deduplicate the
complete attribute tuple; sharing vertices solely by position index destroys
UV seams. Indices never select another subobject's vertex array.

Preserve polygon order. The hardware renderer submits a triangle fan: for
corners numbered 0 through K−1, the triangles are **(0,1,2), (0,2,3), …,
(0,K−2,K−1)**. This works for the convex polygons expected by that path. An
importer supporting concave input needs a polygon triangulation method rather
than assuming every fan lies inside the polygon. Carry the original per-corner
UVs through any triangulation and do not generate UVs from world positions.

For a nondegenerate triangle, compare the stored face normal with
**(v1 − v0) × (v2 − v0)**. They should point to the same side for consistent
exporter output. Preserve the stored normal and report inconsistent data. The
engine's one-sided visibility test uses the face normal, not the vertex normals:
with the eye expressed in that subobject's local frame, the face is visible
when **dot(eye − v0, face-normal) ≥ 0** in the hardware model path. A strict
positive test differs only at edge-on faces. This explicit test is safer than
assuming a default GPU winding; legacy comments call the order clockwise.

Use vertex normals for smooth lighting and face normals for culling or flat
lighting. Apply the same accumulated rotation to positions and normals, with
translation omitted for normals. If a viewer adds nonuniform scale, use inverse
transpose for normals. Do not average across subobjects or recompute all vertex
normals unless intentionally replacing the authored shading.

For camera fitting, compute bounds from the assembled, transformed vertices of
the pose being shown. OHDR radius/bounds can give an initial estimate; they are
not instructions to scale every mesh or to translate it away from its pivot.

### Texture and material resolution

The lookup chain is **face texture index → TXTR entry → game texture/material
→ bitmap or OAF frames → decoded image**. A face with textured flag zero uses
its three stored RGB bytes instead and has no TXTR lookup.

For a TXTR name n, the engine first searches existing game textures for a bitmap
named n plus .OGF. For animated textures it also checks their frame bitmap names.
If none matches, it searches the game texture's logical name n. Both searches
are case-insensitive. Failure selects the game's fallback texture, index zero;
it does not make the geometry disappear. This is a material-database lookup,
not simply an attempt to open a similarly named host file.

An independent viewer can build the same lookup from the GAM texture pages and
the installed HOG/MN3 resources described in the companion specification. If
only an OOF and loose images are available, matching n plus .OGF is a useful
viewer fallback, but material metadata may remain unknown. Display unresolved
faces with a diagnostic material while preserving their UVs and names.

UVs are dimensionless and normally repeat: the engine selects wrap addressing.
Keep values outside 0–1. With OGF rows decoded in their stored top-to-bottom
order, **(U,V) = (0,0) addresses the top-left image corner; increasing V moves
down the image**. The loader and model renderer do not replace V with 1−V.
If an image upload library flips rows, compensate exactly once in that upload
or in V. Flipping both restores the original error. Filtering/mip selection
belongs to the renderer and does not change the stored UVs.

Important material behavior comes from GAM, not additional OOF face fields:

| Texture flag | Value | Rendering consequence |
| --- | ---: | --- |
| ANIMATED | 0x00000040 | Resolve OAF frames using texture animation speed |
| PING_PONG | 0x00040000 | Reverse the frame sequence on alternate passes |
| LIGHT | 0x00080000 | Use full-bright texturing in the model path |
| SATURATE | 0x00200000 | Use additive blending |
| ALPHA | 0x00400000 | Select the constant/vertex-alpha material path |
| PROCEDURAL | 0x01000000 | Bitmap contents are generated by the procedural subsystem |

For an ordinary textured preview, multiply sampled RGB by a supplied lighting
color, retain the image's decoded alpha, and apply vertex alpha. A material's
alpha scalar and an optional object fade add opacity factors. Draw opaque
surfaces with depth testing/writes, then transparent surfaces with depth testing;
ordinary blending uses source-alpha and one-minus-source-alpha, while additive
blending uses source-alpha and one. A viewer can sort transparent surfaces
back-to-front and disable their depth writes. The current game's hardware path
instead queues ALPHA/SATURATE faces after ordinary faces within each subobject
without sorting that queue or explicitly disabling its depth writes. That
ordering is a compatibility detail, not a requirement for reconstructing the
mesh. The exact selected engine alpha modes are constant×vertex for ALPHA or
object-fade faces, texture×vertex otherwise, and saturate-constant×vertex for
SATURATE; SATURATE takes precedence. Do not interpret these flags as extra bytes
in SOBJ.

GAM's texture RGB emission values are not a replacement albedo color for every
textured face. The object's lighting, fog, lightmaps, specular effects, and
custom tint are supplied outside OOF. A reproducible geometry viewer can start
with unlit textures and stored flat RGB. The game's simple Gouraud path instead
uses the authored vertex normal n and a supplied light direction l in local
coordinates to compute **brightness = (1 − dot(l,n)) / 2**, multiplied by the
supplied static RGB and optional object tint. LIGHT textures override this with
white illumination. OOF's trailing face lightmap factors do not contain baked
lightmap pixels or secondary UV arrays; those arrive with level/object lighting
data.

For nonnegative elapsed game time t, an animated texture with F frames and GAM
speed s seconds per forward traversal selects **j = trunc(t · F / s)**. Ordinary
playback uses j modulo F. Ping-pong uses r = j modulo 2F, then r if r < F or
F−1−(r modulo F) otherwise. Thus three frames repeat as **0,1,2,2,1,0**. Require
F > 0 and s > 0. This path uses GAM speed, not the OAF header frame time.
Texture sliding adds a phase independently to U/V: for a nonzero slide period p,
the phase is **(t − trunc(t/p)·p) / p**. These texture animations are independent
of the subobject pose animation below.

### Evaluating subobject animation

OOF stores tracks, not a self-contained list of named clips with durations in
seconds. Gameplay chooses a frame range, duration, weapon-battery state, and a
normalized parameter ui for each subobject. A standalone viewer may supply a
shared slider u in [0,1], but that is not necessarily the pose produced by an
idle, walking, firing, or turret state in the game.

For ordinary timed game-object tracks, the game's current animation frame x
becomes **ui = clamp((x − rot-mini)/(rot-maxi − rot-mini), 0, 1)** for a
nonzero rotation-track span. The endpoint comparisons are performed before
division, so a zero span is not divided by. The caller then supplies that same
ui to both RANI and PANI evaluation. Weapon-battery-local animation can supply
its own frame instead of x, and turret nodes receive their aiming fraction.
The animation grid and durations in a generic-object GAM page choose x; the
OOF does not identify which of those game states is currently active.

**Rotation key accumulation.** Normalize each nonzero stored axis a. Interpret
the angle's low 16 bits as θ = 2π·angle/65536. In this section's column-vector
notation, its delta rotation is the usual axis-angle rotation:

**D v = cosθ·v + sinθ·(a × v) + (1−cosθ)·a·dot(a,v).**

Starting from identity, build accumulated matrices **A0 = D0** and
**Ak = Dk Ak−1**. The multiplication order matters when axes differ. There is
no additional identity key inserted before key zero. A zero axis with zero
angle yields identity; a zero axis with nonzero angle does not define a valid
rotation and should produce a diagnostic instead of an arbitrary axis.

**Timed tracks, versions 2200/2300.** Evaluate rotation and position separately,
using each track's own minimum m and maximum M. The same ui is passed to both
tracks, so their evaluated tick values can differ. For ui < 1, the current tick
is **τ = m + ui(M−m)**. The engine's remap chooses the last key whose start tick
is no greater than **trunc(τ)**; integer conversion truncates toward zero.
For a key k before the last, interpolate with
**f = (τ − startk) / (startk+1 − startk)**. If that key is already last, hold
the last value. At ui = 1, select the last key explicitly, without indexing a
remap entry at the exclusive maximum tick. Validate positive interpolation
intervals, an applicable first key, and track bounds rather than divide by zero
or read uninitialized remaps.

PANI interpolation is **di = (1−f) pk + f pk+1**, in parent coordinates. These
are displacements added to oi. In the timed engine path, zero or one position
key gives **zero displacement**, even if a single stored position is nonzero.
Zero or one ordinary rotation key likewise gives **identity rotation**. With
no normalized-time input, the timed path sets every displacement/rotation to
zero/identity, including rotators and turrets.

For two or more ordinary rotation keys, the engine first linearly interpolates
the **nine components** of accumulated matrices: **L = (1−f) Ak + f Ak+1**.
It then extracts pitch/heading/bank and rebuilds a rotation; it does not apply
the generally nonorthogonal L directly to vertices. Quaternion slerp is a valid
viewer alternative, but produces different intermediate poses and is not the
reference interpolation method.

For independent reproduction of that rebuild, let r,u,v be L's right, up,
forward **columns**, with v used here to avoid confusing forward with the
interpolation fraction f. Using conventional atan2(y,x):

1. Away from the vertical singularity, h = atan2(vx,vz). Let ch = cos(h),
   sh = sin(h). Choose c = vx/sh when |sh| > |ch|, otherwise c = vz/ch.
2. Compute p = atan2(−vy,c) and b = atan2(ry/c,uy/c).
3. Rebuild **Ri = Ry(h) Rx(p) Rz(b)**, with the positive rotation signs
   specified above.
4. If both |vx| and |vz| are below 0.00001, use p = −π/2 when vy > 0 or
   +π/2 otherwise; b = 0; h = atan2(−rz,rx), then perform the same rebuild.

The engine quantizes extracted angles to its 16-bit angle namespace and uses
table-based trigonometry. The equations give its geometric method with ordinary
floating-point trig; subpixel/angle-quantization differences remain without
matching those legacy math helpers. Interpolation between rotations separated
by a half turn can produce a degenerate L; report this case or expose an
explicit alternative interpolation policy rather than silently treating L as
an orthogonal rotation.

**Untimed tracks, versions 1807–2199.** For K keys, place them uniformly over
[0,1], choose k = trunc(ui(K−1)), and use the fractional remainder to interpolate
adjacent keys, with ui = 1 selecting the final key. Rotation accumulation and
matrix interpolation/rebuild follow the rules above. Position has a compatibility
quirk: in the first interval the engine uses **di = f p1**, ignoring p0; later
intervals interpolate pk to pk+1 normally. Missing tracks give zero/identity.
An untimed one-key track holds its sole stored position or accumulated rotation,
unlike the timed path's zero/identity rule. Handle this case before computing
1/(K−1); the reference code reaches its last-key branch after intermediate
floating-point division by zero.

**Rotators and turrets.** When normalized-time input is present, $rotate
overrides the ordinary rotation track with an axis-angle rotation about stored
axis **1**, using a turn fraction from elapsed game time divided by the
property's seconds-per-revolution. The angle passed is trunc(fraction·65535),
not a PANI tick. A $fov turret similarly rotates about axis 1, with ui as its
turn fraction; gameplay chooses ui using turret/battery state. These branches
still evaluate PANI. Require a usable second axis for compatibility; the loader
only clears these property flags when the rotation-key count is zero, which
does not protect against a one-key out-of-range access. The untimed no-input
fallback differs from the timed path: rotators continue spinning, and turrets
spin once per second using axis 0. A viewer may offer a fixed rest pose instead,
but should name that choice rather than confuse it with an evaluated game pose.

### Special subobjects and drawing passes

| Property / condition | Ordinary game rendering behavior |
| --- | --- |
| $shell, $frontface | Door helper geometry; omitted from this polygon-model path |
| No faces | No ordinary drawable geometry; see traversal behavior below |
| $facing | Replace its one-face polygon with a camera-facing textured rectangle in the second pass |
| $glow, $thruster | Use procedural glow/thruster effects in the second pass rather than draw the authored polygon |
| $custom | Draw only when an external custom-texture effect is supplied; use that material in place of the base texture |
| $jitter | Optional runtime deformation of vertex positions; no additional disk vertices |
| $monitor01–08, $viewer, $layer | Semantic markers used by other systems; the ordinary model renderer does not hide these merely because of those names |

The reference renderer returns **before recursing** for a shell, frontface, or
faceless node, thereby pruning that subtree. A scene viewer can instead retain
such nodes as transform-only helpers and expose their children; label this as
a viewer policy. Within hierarchical traversal, a visibility-mask bit suppresses
that node's own drawing while traversal continues to its children, so it is not
equivalent to pruning the node. Neither operation should apply a second copy of
its transform. The high-level DrawPolygonModel API can select the isolated-
fragment path for partial masks unless its overlay argument enables whole-model
traversal; do not use that fragment path to infer ordinary child placement.

For $facing, compute the area A of its single authored polygon. The loader uses
**half-width = √A / 2** and **half-height = half-width · image-height/image-width**.
Place the rectangle at the node's **local origin**, transformed through Gi,
and orient its edges along the camera's right/up axes. The computed polygon
centroid is not used as the billboard center. The rectangle's full width/height
are twice these half-extents. It uses that face's material, full-bright shading,
and normal or additive transparency according to SATURATE, with depth writes
disabled. Treating the authored polygon as a permanently oriented quad misses
this behavior.

For glows/thrusters, the single authored face supplies an outward normal from
its corner geometry; the property supplies RGB and size. The effect is anchored
at the transformed local origin. The gradient-ball image comes from the game's
effect assets, not an extra embedded OOF image. The default glow draws an
additive, camera-facing gradient with half-width equal to the property size,
aspect-correct half-height, opacity 0.8, and depth writes disabled. An external
glow-scalar effect switches to a cone represented by progressively smaller
gradient billboards along the transformed normal, with opacity 0.3 and external
length/size multipliers. In this renderer, $thruster alone does not select that
cone branch: it shares the glow path until the external effect enables it.
Custom effect RGB can override the property color. A standalone viewer may use
a generated radial gradient and a default length, clearly identifying those
as substitutes for missing effect assets/state.

The game also gives a glow a negative depth bias equal to its size, and applies
the corresponding negative half-width bias to each thruster billboard. This
helps the additive effect avoid intersecting its emitting surface; it is a
render-state offset, not a translation to bake into the stored node transform.

The renderer traverses roots for ordinary surfaces first, then traverses again
for facing/glow/thruster effects. Reuse the evaluated pose in both passes. Do not
advance animation time once per pass, otherwise effects detach from moving
parts. GPNT/ATCH/GRND markers remain optional debugging overlays, not meshes.

### Worked assembly and rendering checks

These examples are synthetic and require no commercial assets. All rotations
below use the column-vector convention and ideal trigonometry; object-to-world
is identity unless stated otherwise.

| Setup | Expected result |
| --- | --- |
| Root offset (10,0,0); child offset (0,2,0); both rotations identity; child vertex (1,0,0) | Model point (11,2,0) |
| Same, but root rotates +90° about Z | Model point (8,1,0) |
| Same root rotation, and child also rotates +90° about Z | Model point (7,0,0) |
| Same rotations, plus child PANI displacement (0,1,0) | Model point (6,0,0); the displacement uses the parent's axes before the root rotation |
| Change that child's geometric-center field to (4,0,0), keeping all other fields | Same point (6,0,0); no extra center subtraction |
| Rotation deltas +90° Z followed by +90° X; transform (1,0,0) | (0,0,1), because the accumulated matrix is Rx Rz |
| Timed PANI range 10–30; keys at 10:(0,0,0), 20:(0,4,0), 30:(0,4,2); u = 0.25 | Tick 15; displacement (0,2,0), before adding SOBJ offset |
| Same PANI at u = 1 | Final displacement (0,4,2), without an out-of-bounds remap lookup |
| Halfway between identity and a +90° Z accumulated rotation | Rebuilt rotation maps (1,0,0) to approximately (0.707107,0.707107,0); applying raw matrix interpolation would incorrectly give (0.5,0.5,0) |
| Face corners (0,0,0), (1,0,0), (0,1,0); stored normal (0,0,1); local eye (0,0,5) | Face passes the normal-facing test; eye (0,0,−5) fails |
| A 2×2 decoded image with four different colors, nearest filtering | UV (0.25,0.25) selects its top-left texel; (0.25,0.75) selects its bottom-left texel |
| Camera point (1,1,4), 90° vertical FOV, 800×400 viewport | Pixel (450,150) in a top-down framebuffer |
| Facing square of area 4, texture 128×64 | Billboard full width 2, full height 1, centered at the transformed pivot |

The transform, rotation-sign/order, interpolation-rebuild, face-normal, and
billboard-area examples were checked with a temporary harness linked against
this checkout's existing vector/matrix and fixed-angle math libraries. It also
compared the Euler reconstruction equations above with the engine routines for
500 interpolated rotation cases; the largest matrix-component difference was
approximately 0.000272, consistent with the documented legacy math approximation.
These are numerical checks, not a claim of pixel-identical scene rendering.

If parts appear exploded, check accumulated offsets and transform order. If
rotating parts orbit the wrong point, check local pivots and whether a center
was subtracted unnecessarily. Mirrored or inside-out geometry suggests an
unhandled coordinate reflection or winding conversion. An upside-down image
usually indicates a duplicated/missing V or row flip. A mesh that shrinks during
rotation interpolation usually means the nonorthogonal matrix blend was applied
directly. Correct geometry with missing glows or cockpit displays can instead
indicate absent external effect/material state.

### Rendering sources

[Hierarchy, axis-angle matrices, track evaluation, and point transforms](../model/polymodel.cpp),
[mesh drawing, special passes, lighting, and effects](../model/newstyle.cpp),
[basis/vector operations](../lib/vecmat_external.h),
[matrix composition and Euler conversion](../vecmat/vector.cpp),
[legacy angle helpers](../fix/fix.cpp),
[nested view transforms](../renderer/HardwareInstance.cpp),
[projection and viewport conventions](../renderer/HardwareSetup.cpp),
[billboard geometry](../renderer/HardwareDraw.cpp),
[material lookup and texture animation](../Descent3/gametexture.cpp),
[material flags](../Descent3/gametexture.h), and
[texture uploads and blending](../renderer/HardwareOpenGL.cpp).
