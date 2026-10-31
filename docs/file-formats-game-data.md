# Descent 3 containers and game-data formats

## Reading this specification

This companion to [graphics and models](file-formats-graphics-and-models.md)
describes the Descent-specific containers and data formats implemented at commit
**8b4c924ceebe284d06604b2e985cd839ae4329d5**. Field tables and numbered sequences
are in physical file order. Counts refer to file entries, not currently installed
resources. Numeric resource IDs should be retained and resolved through the
file's name tables; a decoder does not need to load the game to return them.

Use the companion's U8, I8, U16, I16, U32, I32, F32, Vec3, FourCC, ZString,
and FixedString definitions. Numeric primitives are little-endian, with no
implicit alignment. Mat3 means nine F32 values: right vector XYZ, up vector
XYZ, then forward vector XYZ. LengthString16 means I16 byte count N followed
by N bytes; whether N includes a terminator is stated for each use. A boolean
occupies one byte only where the layout says U8; an I32 boolean occupies four.

**Version profiles matter.** Detailed portable layouts below target HOG2,
OSF1, modern GAM pages at the listed versions, D3LV 127/128/132, ORF 4, group 3,
pilot 0x2B, ambient patterns 0, and their explicitly stated variants. Older
layouts must not be decoded by blindly applying a newer table. Unknown
length-delimited chunks/pages can be retained as opaque records. That is a
lossless structural decode, not a semantic decode of their contents.

Savegames and demos contain raw native structures without a complete ABI
descriptor. Their independently readable prefixes and limitations are documented
separately; the portable layouts in this document do not claim to decode every
native snapshot member on every historical build.

### Contents

- [Format directory](#format-and-extension-directory)
- [HOG2 and MN3 archives](#hog2-archives--hog-and-mn3)
- [OSF audio wrapper](#osf1--outrage-streaming-audio)
- [GAM / ITL tables](#gam--itl--game-resource-tables)
- [D3L levels](#d3l--level-data)
- [ORF rooms](#orf--outrage-room-files)
- [GRP prefabs](#grp--editor-room-groups)
- [PLT / PLD pilots](#plt--pld--pilot-data)
- [Ambient patterns](#aspf--ambient-sound-patterns)
- [Pilot-picture index](#ppicsidx--pilot-picture-index)
- [MSN missions](#msn--text-mission-descriptions)
- [Music and HUD configuration](#omf-and-inf-style-text)
- [STR localization](#str--localized-string-tables)
- [MSG module messages](#msg--osiris-module-messages)
- [BRF briefings](#brf--telcom-briefings)
- [Savegames](#savegames--native-snapshots-and-count-companions)
- [DEM recordings](#dem--recorded-gameplay)
- [Editor and user metadata](#editor-and-user-metadata)
- [Validation and compatibility boundaries](#decoder-validation-and-coverage-boundaries)

## Format and extension directory

| Format / typical name | Identification | Nature |
| --- | --- | --- |
| HOG, MN3 | HOG2 at offset zero | Indexed archive; MN3 is a mission package using the same archive format |
| GAM / ITL | No global signature; sequence of typed pages | Resource definitions and table overlays |
| D3L | D3LV at offset zero | Level geometry, objects, terrain, and associated data |
| ORF | Numeric chunk ID 5 or legacy 0 at offset zero | Editor's standalone room format |
| GRP | D3GP at offset zero | Editor room group/prefab |
| OSF | OSF1 at file length minus 128 | Outrage streaming-audio wrapper |
| OMF | First line [theme file] | Text music sequencing instructions |
| MSN | Case-insensitive keyword lines | Mission description inside or outside MN3 |
| BRF | Dollar-prefixed directives | TelCom briefing screens and effects |
| STR | Language tags, notably !=! | Localized, ordered string tables |
| MSG | Message-key lines | OSIRIS module messages |
| INF-style HUD/cockpit data | Caller-specific opening tag | Engine text configuration dialect |
| PLT / PLD | I32 file version, currently 43 | Pilot or pilot-default template |
| saveg000 through saveg007 | No fixed magic; 32-byte description | Savegame slots, with optional .cnt restore-count companion |
| DEM | ZString D3DM1 or D3DEM | Initial snapshot followed by typed events |
| Ambient sound patterns | ASPF at offset zero | Named sound-pattern database |
| PPics.idx | I32 count | Pilot-picture metadata; images live separately in PPics.Hog |
| table.lok / TableVersion | Fixed records / one I32 | Editor resource-table coordination metadata |
| MPS | Keyword/value lines | Saved multiplayer settings |
| .Descent3Registry | Section and quoted-value lines | Linux engine preferences |
| game-assets.cfg | One UTF-8 path line | This checkout's separately stored asset-location preference |

The following are **not new Descent-specific binary encodings**: WAV, PCX, TGA,
PNG, TrueType, ELF/shared objects, Windows PE/DLL, and Interplay ACM/MVE.
A multiplayer **.d3m** is a platform dynamic module, not an MN3/HOG archive merely
because it has a game-specific extension. Native OSIRIS modules likewise retain
their platform executable format. Older POF models belong to the preceding
Descent format family; the active D3 OOF layout is in the companion document.

## HOG2 archives — HOG and MN3

### Fixed header and directory

| Absolute offset | Bytes | Field |
| ---: | ---: | --- |
| 0 | 4 | ASCII HOG2, without terminator |
| 4 | 4 | U32 file count N |
| 8 | 4 | U32 absolute start offset of member data D |
| 12 | 56 | Reserved header bytes; directory starts at offset 68 |
| 68 | 48 × N | Directory entries |
| D | variable | Concatenated member data in directory order |

Each **48-byte** directory entry contains FixedString(36) filename, U32 flags,
U32 byte length, and U32 timestamp. No per-entry data offset is present.
Member zero starts at D; every later member starts at D plus the sum of the
preceding lengths. Members are uncompressed byte ranges; flags do not enable
an archive compression codec in this reader. The timestamp is stored as an
integer time value; preserve it without treating it as a filename component.

Normal writer output sets D to 68 + 48 × N. A decoder should honor the stored D,
require the directory to fit before it, and validate all member ranges against
the containing file. There is no implicit member padding or directory footer.
The filename field is fixed width even for short names. Do not look for a new
entry by scanning for zero bytes in member data.

The game searches member names without case distinctions. Writer tools sort
directory entries case-insensitively; preserve the directory's actual order
when calculating member offsets. For extraction, treat names as archive names
and reject path traversal independently of parsing the file.

MN3 packages contain a mission's MSN and associated assets; the archive has no
special MN3 header beyond HOG2. The mission loader derives the MSN name from the
MN3 basename. Platform-specific script HOG files are also ordinary HOG2 archives.
Legacy Descent HOG files with the DHF signature have a different format and are
not the HOG2 profile specified here.

Sources: [header and entry definitions](../cfile/hogfile.h),
[archive reader](../cfile/cfile.cpp), and
[writer layout](../tools/HogMaker/HogFormat.h).

## OSF1 — Outrage streaming audio

OSF puts its fixed-size header at the **end**, not the start. The final 128 bytes
are the following footer; offsets are relative to file length minus 128:

| Footer offset | Encoding | Meaning |
| ---: | --- | --- |
| 0 | FourCC | OSF1 |
| 4 | U8 | Stream type: 0 for digital audio |
| 5 | U8 | Compression: 0 raw, 1 Interplay ACM |
| 6 | U8 | Format flags |
| 7 | U8 | Rate code: 11 = 11025 Hz, 22 = 22050 Hz, 44 = 44100 Hz |
| 8 | U32 | Digital auxiliary value: samples per musical measure |
| 12 | U32 | Stream-data byte length L |
| 16 | 48 bytes | Reserved |
| 64 | FixedString(32) | Title |
| 96 | 32 bytes | Reserved |

The audio payload starts at offset zero. Require file length at least 128 and
L no greater than file length minus 128. Decode only L payload bytes; reserved
or trailing bytes are not audio. Rate code zero is not one of the writer's
supported rates and should be reported rather than interpreted as 0 Hz.

Format bit 0 selects 16-bit (set) or 8-bit (clear) samples; bit 4 selects stereo
(set) or mono (clear). The four named combinations are 0x00, 0x01, 0x10, 0x11.
For raw streams, interpret the payload as PCM with the indicated sample width
and channel count: 8-bit samples are unsigned with silence at 128; 16-bit samples
are signed little-endian with silence at zero. Stereo is interleaved left/right.
For compression 1, pass the payload to an Interplay ACM decoder; ACM is a
separate shared codec, not the OGF/terrain repeat-run encoding. The footer's
rate/channel metadata and decoded audio should be checked for consistency.

The writer zero-fills the footer, but a decoder should not require reserved
bytes to be zero. Read the title within its 32-byte bound even if it lacks a
terminator; the implementation's writer computes a terminated temporary title
but writes from the original title buffer.

Sources: [OSF reader/writer](../stream_audio/osfarchive.cpp),
[flags](../stream_audio/streamaudio.h), and
[playback interpretation](../stream_audio/streamaudio.cpp), with
[PCM sample convention](../sndlib/sdlsound.cpp).

## GAM / ITL — game resource tables

### Page framing

Modern tables have no single magic or global file version. Repeated pages run
to end of file. At page start P, read U8 page type, then I32 length L, then the
payload. **L includes the four bytes occupied by the length itself**, but excludes
the preceding type byte. Thus payload length is L − 4 and the next page begins
at P + 1 + L. There is no implicit alignment. Require L ≥ 4 and a valid end
boundary. A type-zero page with length four is a placeholder with no payload.

Every supported nonempty page below starts with an I16 **page-specific version**.
Its following fields depend on both page type and that version. Tables/overlays
contain resource definitions and external asset names, not embedded OGF/OOF data.

| Type | Resource | Detailed current profile below |
| ---: | --- | ---: |
| 0 | Unknown/placeholder | No fixed semantic payload |
| 1 | Texture | 7 |
| 2 | Weapon | 8 |
| 3 | Legacy robot | Historical command-page format, not modern generic layout |
| 4 | Legacy powerup | Historical command-page format, not modern generic layout |
| 5 | Door | 3 |
| 6 | Ship | 6 |
| 7 | Sound | 1 |
| 8 | Megacell | 2 |
| 9 | Gamefile | 2 |
| 10 | Generic object | 27 |

The following sequences include every current-profile field. Names are ZString.
Flags are retained as integer bit sets; only bits affecting physical layout are
required for structural decoding and are identified explicitly.

### Shared table records

**Physics**, 68 bytes, in order: F32 mass, F32 drag, F32 full thrust, U32 flags,
F32 rotational drag, F32 full rotational thrust, I32 bounce count, F32 initial
forward velocity, Vec3 rotational velocity, F32 wiggle amplitude, F32 wiggles
per second, F32 restitution coefficient, F32 hit/die dot threshold, F32 maximum
turn-roll rate, F32 turn-roll ratio. Only the forward component of linear
velocity is stored, not a full linear-velocity vector.

**Lighting**, 50 bytes: F32 light distance; three F32 primary RGB values; F32
time interval, F32 flicker distance, F32 directional dot; three F32 secondary
RGB values; U32 flags; U32 time bits; U8 angle; U8 lighting-render type.

**Weapon battery**, current shared profile 2, **211 bytes**:

1. F32 energy usage, F32 ammunition usage.
2. Eight I16 gunpoint weapon indices.
3. Eight mask records, each U8 gunpoint-fire mask, F32 fire wait, F32 animation
   time, F32 animation start frame, F32 animation fire frame, F32 animation end
   frame. These six fields are interleaved per mask.
4. U8 active mask count, I16 aiming gunpoint index, U8 aiming flags.
5. F32 3D aiming dot, F32 3D aiming distance, F32 XZ aiming dot.
6. U16 flags, U8 quad-fire mask.

The older battery profile 1 uses U8 flags instead of U16, reducing the record
by one byte. Ship pages below version 6 and generic pages below version 21 use
that older battery record; supporting those page versions also requires their
other version conditions. Do not guess solely from remaining payload length.

### Texture page, version 7

| Order | Field | Encoding |
| ---: | --- | --- |
| 1 | Version | I16 = 7 |
| 2 | Texture, bitmap, destroyed-texture names | Three ZStrings |
| 3 | Emitted red, green, blue, alpha | Four F32 |
| 4 | Animation speed, U slide, V slide, reflectivity | Four F32 |
| 5 | Corona type | U8 |
| 6 | Damage | I32 |
| 7 | Texture flags | U32 |
| 8 | Procedural data | Present only when flag 0x01000000 is set; sequence below |
| 9 | Associated sound name | ZString |
| 10 | Sound volume | F32 |

Procedural data: 255 U16 palette entries; U8 heat, U8 light, U8 thickness;
F32 evaluation time; F32 oscillation time; U8 oscillation value; I16 element
count E; then E eight-byte element records, each U8 type, frequency, speed,
size, X1, Y1, X2, Y2, in that order. The palette really contains **255** entries,
not 256. Sound fields follow the optional procedural block in both cases.

### Sound page, version 1

I16 version; ZString sound name; ZString sample filename; U32 flags; I32 loop
start; I32 loop end; F32 outer-cone volume; I32 inner-cone angle; I32 outer-cone
angle; F32 maximum distance; F32 minimum distance; F32 import volume.
This is metadata; audio sample bytes reside in the named external resource.

### Door page, version 3

I16 version; ZString door name; ZString model name; F32 open duration; F32 close
duration; F32 stay-open duration; U8 flags; I16 hit points; ZString opening sound;
ZString closing sound; ZString module name. Version 2 omits hit points; version
1 also omits module name. No script bytecode follows the module name.

### Gamefile page, version 2

I16 version; ZString logical resource name; ZString directory name. No size,
contents, or archive-member offset follows. The external directory/name must be
interpreted through resource lookup separately.

### Megacell page, version 2

I16 version; ZString name; **64 ZStrings** containing texture cell names; U8
active width; U8 active height. All 64 names exist even when active dimensions
are smaller than the 8×8 storage grid. The width/height bytes come **after**
the entire name array.

### Weapon page, version 8

Read the following numbered groups consecutively:

1. I16 version; ZStrings name, HUD image, firing image, particle image.
2. U8 particle count; F32 particle lifetime; F32 particle size; U32 weapon flags.
3. ZString spawned weapon; U8 spawn count; ZString spawned robot; ZString
   alternative spawn; U8 alternative chance.
4. Six F32 values: gravity duration, gravity size, homing FOV, custom size,
   physical size, thrust duration.
5. Shared Physics record.
6. F32 terrain-damage size; U8 terrain-damage depth; F32 alpha.
7. ZString explosion image; F32 explosion duration; F32 explosion size.
8. Eight F32 values: player damage, generic damage, impact size, impact time,
   impact player damage, impact generic damage, impact force, lifetime.
9. Shared Lighting record; F32 recoil force.
10. Seven ZString sound names; ZString smoke image; ZString scorch image;
    F32 scorch size; ZString icon name.

### Ship page, version 6

1. I16 version; seven ZStrings: ship name, cockpit configuration, HUD
   configuration, main model, dying model, medium-LOD model, low-LOD model.
2. F32 medium-LOD distance; F32 low-LOD distance; shared Physics record;
   F32 size; F32 armor scalar; U32 ship flags.
3. **21 weapon slots**, each containing U8 fire flags; ZStrings firing sound,
   release sound, spewed powerup; I32 maximum ammunition; shared battery
   profile 2; eight ZString fire-sound names; eight ZString weapon names.

The 21 slots are not preceded by a count. Do not interchange this slot's two
eight-name arrays; their order differs from the generic object's arrays below.

### Generic-object page, version 27

The object category is an independent byte after the page version. Category
**7** means powerup and enables the ammunition field below.

1. I16 version; U8 object category; ZStrings object name, main model,
   medium-LOD model, low-LOD model.
2. F32 impact size, F32 impact duration, F32 damage; I16 score; **I16 ammunition
   count only for category 7**.
3. ZStrings legacy script name, module name, script-name override; U8 description
   present; optional ZString description if nonzero; ZString icon.
4. F32 medium-LOD distance; F32 low-LOD distance; shared Physics record;
   F32 object size; shared Lighting record; I32 hit points; U32 object flags.
5. U32 AI flags; U8 AI class, U8 AI type, U8 movement type, U8 movement subtype;
   F32 FOV, F32 maximum velocity, F32 maximum velocity change, F32 maximum turn
   rate; U32 notification flags.
6. F32 values, in order: maximum turn-rate change, circle distance, attack velocity
   fraction, dodge fraction, dodge velocity fraction, flee velocity fraction,
   two melee damage values, two melee latency values, curiosity, night vision,
   fog vision, lead accuracy, lead variance, fire spread, fighting other team,
   fighting same team, aggression, hearing, frustration, roaming, self-preservation,
   friend-avoidance distance, biased-flight importance, biased-flight minimum,
   biased-flight maximum.
7. Two death-spew records: U8 spew flag, F32 probability, I16 quantity,
   ZString spawned object. The flag is repeated in both records.
8. Animation grid: five movement classes, each with 24 entries, each I16 first
   frame, I16 last frame, F32 seconds per cycle; 120 records in class-major order.
9. 21 shared battery profile-2 records.
10. 21 × 8 ZString weapon names, battery-major order; two object sound ZStrings;
    five AI sound ZStrings; 21 × 8 ZString fire-sound names, battery-major order;
    five × 24 ZString animation-sound names, class-major order.
11. F32 respawn scalar; I16 death-type count D; D records, each U32 flags,
    F32 minimum delay, F32 maximum delay, U8 probability. Current writer emits D = 4.

### Historical command-page framing

The old table method does not have modern page lengths. After U8 page type,
each command consists of U8 command ID, U8 payload byte length, and that many
bytes, through a page-type-specific END command. Unknown command payloads can
be skipped using the byte length. END IDs and meanings differ by page family;
this is not the modern versioned layout and is not safely auto-detectable from
a filename alone. Preserve such a table as legacy command records or reject it
explicitly unless a page-family command dictionary is implemented.

Sources: [page framing/shared records](../manage/manage.cpp),
[page IDs](../lib/manage.h), [textures](../manage/texpage.cpp),
[weapons](../manage/weaponpage.cpp), [ships](../manage/shippage.cpp),
[generic objects](../manage/generic.cpp), [doors](../manage/doorpage.cpp),
[sounds](../manage/soundpage.cpp), [megacells](../manage/megapage.cpp), and
[gamefile pages](../manage/gamefilepage.cpp).

## D3L — level data

### Version and framing

The header is FourCC **D3LV** followed by I32 level version. The normal engine
uses version **128**; a NEWEDITOR build uses **132**. Historical readers accept
many versions back to 13, with substantial conditional layouts. The detailed
field sequences below specify the shared **127/128/132** geometry profile, with
INFO and EDIT differences called out explicitly. Select the file's version
before applying these differences.

A chunk at P consists of FourCC ID followed by I32 length L. **L counts the
four-byte length field, its payload, and any final padding, but excludes the
FourCC.** The next chunk starts at P + 4 + L. The writer pads L to a multiple
of four with zero bytes. Require L ≥ 4, and do not decode these trailing padding
bytes as another record. A chunk with no payload has L = 4 and occupies eight
bytes total. This is different from OOF's payload-only length.

### Level compressed arrays

Compressed arrays occur within terrain, volume-light, and lightmap data. Their
uncompressed element count is supplied by the surrounding field, not the array.
Read U8 compression flag. Zero means exactly N raw elements. Any nonzero value
means repeat packets: U8 command followed by one element. Command zero emits
one element; commands 2–250 emit that many copies; all other commands are invalid.
Elements are U8 for a byte array and little-endian U16 for a short array.
Stop after exactly N output elements, without packet overrun or an end marker.
Each array has its own flag. OGF uses similar packets **without this flag**.

### Chunk directory

| ID | Contents |
| --- | --- |
| TXNM, GNNM, DRNM | Texture, generic-object, and door name/index tables |
| ROOM | Rooms, faces, and portals |
| RWND | Per-room wind vectors |
| OBJS | Object handles followed by level-object records |
| OHND | Handles of deleted slots that still carry generation information |
| TERR | Nested terrain chunks TERH, TETM, TSKY, TEND |
| NLMP | Lightmap textures and surface mapping information |
| PATH | Named paths and oriented nodes |
| PSTR | Player-start eligibility flags |
| TRIG | Trigger records |
| INFO | Human-readable metadata and global physical settings |
| TSND | Terrain sound bands |
| OSND | Two override sound names |
| FFTM | Force-field texture multipliers |
| NODE | Navigation-node graph |
| CBOA | Room/terrain connectivity and travel-cost tables |
| AABB | Face and room acceleration bounding boxes |
| CNBS | Binary-space-partition tree |
| MTCN | Object-production centers |
| LVLG | Level goals |
| LIFE | Ambient-life state |
| EDIT | Editor selection, camera, and lighting settings |
| RBNM, PWNM, LMAP, CBSP, SCPT, CODE, TLNK | Historical chunk IDs; retain/skip by chunk boundary unless their historical version profile is supported |

Read name tables and NLMP before resolving room/object references. Chunk order
can be preserved while a decoder defers reference resolution. References to
missing textures are not reasons to misalign the binary parse.

### Name tables and simple chunks

TXNM/GNNM/DRNM: I32 count N, then N ZStrings, including empty names for holes.
Index i refers to the ith string. Empty entries are unresolved resources, not
an instruction to stop the table.

| Chunk | Reader-profile payload, in order |
| --- | --- |
| RWND | I32 count; repeated I16 room index, Vec3 wind |
| OHND | I32 count; that many I32 object handles |
| PSTR | I16 count; that many U32 start flags; positions are in player objects, not this chunk |
| INFO | Four ZStrings: name, designer, copyright, notes; F32 gravity; I32 powerups-ignore-wind only if version ≥ 131; I32 always-check-ceiling; F32 ceiling height |
| OSND | ZString force-field sound; ZString breaking-glass sound |
| FFTM | Three entries, each ZString texture name and, only if name is nonempty, F32 bounce multiplier |
| TSND | I32 count; repeated ZString sound, U8 low altitude, U8 high altitude, F32 low volume, F32 high volume |

INFO's numeric trailer is 12 bytes for versions 127/128 and 16 bytes for 132.
There is a reader/writer discrepancy in this source revision: the legacy editor
writer conditionally emits powerups-ignore-wind for a version-128 output, while
the NEWEDITOR writer omits it for version 132, the reverse of the active reader's
version gate. The normative decode above follows the reader and existing files.
An editor-output compatibility mode may expose the extra/missing field as a
producer-specific variation; it must not silently shift all later values.
Sound overrides and force-field multiplier chunks are omitted by the NEWEDITOR
writer. Unknown optional chunks remain bounded byte ranges.

### ROOM

Prefix: five I32 totals: room count R, vertex total, face total, face-corner total,
portal total. Then R entries, each I16 room index followed by one room record.
Indices need not be contiguous. The four aggregate counts are not additional
arrays and are useful for validation.

**Room record**, in order:

1. I32 vertex count V, I32 face count F, I32 portal count P; ZString room name;
   Vec3 path point; V Vec3 vertex positions.
2. F face records; P portal records, as specified below.
3. U32 room flags; U8 pulse time; U8 pulse offset; I16 mirror-face index.
4. If room flag **0x00000002** is set: U8 doorway flags, U8 required keys,
   I32 door-name-table index, F32 door position.
5. U8 volume-light present. If exactly 1: I32 width, height, depth, followed
   by a level-compressed U8 array of width × height × depth elements.
6. F32 fog depth, red, green, blue; ZString ambient-pattern name; U8 reverb;
   F32 damage; U8 damage type.

**Face record**, in order:

1. U8 corner count C; C U16 vertex indices into the room position array.
2. C interleaved records: F32 U, F32 V, U8 alpha. These follow all indices.
3. U16 face flags; U8 portal index (0xFF means none); I16 texture-name-table index.
4. If flag **0x0001** is set: U16 lightmap-info index, then C pairs of F32 U2, V2.
5. U8 light multiplier; U8 special-face-present.
6. If special face present: U8 type, U8 specular-instance count S, U8 smoothing
   present. If smoothing present, next is U8 normal count N. Then S records of
   Vec3 bright center, U16 packed color; finally N Vec3 vertex normals if smoothing
   present. Writer emits four instances, but use the stored S.

There is no stored geometric face-normal vector in a D3L face record: the
engine computes it from the positions and winding. Do not confuse this layout
with OOF faces or standalone ORF faces, which do store normals.

**Portal record**, 32 bytes: U32 flags; I16 portal-face index; I32 connected
room; I32 connected-portal index; I16 navigation-node index; Vec3 path point;
I32 combined-portal master. Portal links refer to the level's room indices.

### OBJS and TRIG

OBJS begins with I32 object count. Each entry starts with I32 object handle,
then the following record. Handles include generation information; they are
not merely dense array ordinals.

1. U8 object type; I16 resource ID; ZString name; U32 flags.
2. I16 shields **only for door type 17**.
3. I32 room number; Vec3 position; Mat3 orientation.
4. U8 contained-object type, ID, count; F32 remaining lifetime.
5. For sound-source type **24**: ZString sound name and F32 volume.
6. U8 script-name byte count S, S bytes without terminator; U8 module-name byte
   count M, M bytes without terminator.
7. U8 object-lightmap present. If nonzero: U8 model count; for each model,
   I16 face count; for each face, U16 lightmap-info index, Vec3 right vector,
   Vec3 up vector, U8 UV count, then that many F32 U2/V2 pairs.

Robot type 2, powerup type 7, building type 16, and clutter type 11 use the
generic-name table for their resource ID; door type 17 uses DRNM. Preserve other
types/IDs directly. A decoded model name is obtained from the resource tables,
not from a hidden OOF string following the object.

TRIG: I32 trigger count, then each trigger's ZString name, I16 room index,
I16 face index, U16 flags, U16 activator mask. The trigger record contains no
native pointer or per-trigger length.

### TERR and its nested chunks

TERR contains another sequence using the same padded level-chunk framing.
TEND ends that nested sequence and has no payload. For version 128, terrain
dimensions are 256×256 cells; the texture grid is 32×32. Array order is the
stored linear cell order; group data into rows of the documented width.

| Nested ID | Payload |
| --- | --- |
| TERH | One compressed U8 array, 65536 height values |
| TETM | Compressed U16 texture-index array of 1024 elements; compressed U8 rotation array of 1024; compressed U8 cell-flags array of 65536 |
| TSKY | Sky prefix, satellites, and lighting arrays, below |
| TEND | Empty |

TSKY prefix: F32 fog scalar, F32 damage per second; U8 textured flag; I16 dome
texture index; U32 sky color, horizon color, fog color; U32 flags; F32 radius;
F32 rotation rate; I32 satellite count S. Each satellite: I16 texture index,
Vec3 direction/position vector, U8 flags, F32 size, F32 red, green, blue.

Next are **five separate compressed U8 arrays**, each 65536 elements, in order:
legacy light intensity, red, green, blue, dynamic-light table. Then I32 occlusion
checksum and a compressed U8 array of **8192** elements: 16 × 16 occlusion regions,
each with 32 bytes. The checksum is not an array length.

### NLMP — lightmaps

I32 texture count T. Each texture: I16 width, I16 height, then a compressed U16
array of width × height pixels in the engine's 1555 color representation. Current
writer bounds dimensions to 2–128. After all textures, read I32 info count I.
Each info record is U16 lightmap-texture index, I16 width, I16 height, U8 type,
I16 texture X origin, I16 texture Y origin, U8 X spacing, U8 Y spacing, Vec3
upper-left location, Vec3 normal. Room/object references index this **info table**,
not the preceding texture table. Multiple info records can share one texture.

### PATH, NODE, CBOA, AABB, and CNBS

PATH begins with I16 path count. Each path has ZString name, I32 node count,
U8 flags, then nodes containing Vec3 position, I32 room, U32 flags, Vec3 forward,
Vec3 up.

NODE begins with I16 maximum list index H; there are **H + 1** lists. Each has
U8 present; when set, I16 node count and that many records: Vec3 position,
I16 edge count, then edges containing I16 destination room, U8 destination
node index, U16 flags, I16 cost, F32 maximum radius. After all lists comes one
U8 graph-verified flag, once for the whole graph.

CBOA: I32 mine checksum, I32 visibility checksum, I32 maximum index H, I32
portal-column count P; an (H + 1) × (H + 1) U16 routing array in row-major order;
an (H + 1) × P F32 cost array; I32 mine count; I32 terrain-region count R; for each
region I32 connection count and that many pairs of I32 room and I32 portal.

AABB: I32 highest room index H, then H + 1 I32 checksums. For each index through
H: I32 used flag. Unused entries end there. Used entries contain I32 face count F;
F pairs of Vec3 minimum/maximum; I32 room checksum; Vec3 room minimum/maximum;
I16 region count R; R I16 face counts; then each region's U16 face-index array,
Vec3 minimum/maximum, U8 sector. The R counts all precede the region payloads.

CNBS: I32 checksum followed by one recursive preorder node. U8 node type 1 or 2
is an empty or solid leaf and has no more fields. Type 0 has four F32 plane
coefficients, I16 room, I16 face, U8 subface, then its front child followed by
its back child. Bound recursion and node count independently of input validity.

### MTCN — materialization/production centers

I32 center count, then complete records below. Each record begins with I32
version **3**; I16 spawn-slot count S; I16 product-slot count P; I16 sound-slot
count A. These are stored capacities, not active counts, and determine array
lengths later in the record.

1. LengthString16 name, count including its zero terminator.
2. U8 active product count, control type, center type, creation effect;
   I16 creation texture; I32 active spawn count; I32 room; Vec3 creation point;
   I32 creation room.
3. S records, each I32 spawn point, three F32 spawn-vector components,
   three F32 spawn-normal components. The writer mistakenly stores X, Y, X
   for both vectors; decode the six stored values without inventing a lost Z.
4. I32 maximum production; P product records, each LengthString16 object name
   including terminator, F32 production duration, I32 priority, I32 maximum
   production for that type. An empty name is normally length 1 plus a zero.
5. I16 maximum live children; if positive, I16 live count and that many I32
   child handles.
6. F32 pre-production duration; F32 post-production duration; A LengthString16
   sound names. Here zero length means no sound, with no following zero byte.
7. F32 speed multiplier; U8 production mode; F32 mode time; U32 status;
   I32 production count; I32 last product index; F32 last finish time;
   I32 cached product index; F32 cached product time; F32 next active-check time;
   U8 last active-check result; I32 last produced object handle.
8. P I32 per-type production counts; I32 active sound handle.

### LVLG and LIFE

LVLG begins with I16 version **4**, I16 goal count G. Each goal has U32 status,
I32 priority, I8 goal-list number; four LengthString16 fields whose counts
**exclude** a terminator: name, item name, description, completion message;
I16 item count; items of U8 type, I32 handle, U8 completed. After all goals,
one U32 global goal-flags value follows.

LIFE begins with I32 version **1**. Read six type definitions, each a
LengthString16 object name including its terminator; U8 total, flags, minimum,
maximum, next batch size; F32 next event time. Then read six separate live-object
lists: U8 count followed by that many I32 handles. Definition and live-list
records are not interleaved. Ambient-life flag 0x01 means inside.

### EDIT and historical support boundary

Version-128 EDIT: eight I16 current/marked room-face-edge-vertex selections;
I32 selected-room count followed by I16 room indices; four I32 values for
current object, current trigger, view mode, viewer ID; Vec3 camera target;
Mat3 orientation; F32 camera distance; **400 records** of four F32 values:
room multiplier and ambient RGB. Finally I32 light spacing, F32 global multiplier,
three F32 global ambient RGB values, I32 radiosity step limit.

Version 127 omits the final global multiplier, global ambient RGB, and radiosity
step limit; it still includes light spacing. The 400-room lighting array describes
the current source's writer capacity; editor builds with a different fixed room
capacity require that producer profile when interpreting this optional chunk.

Versions earlier than 127 alter numerous unframed room/object fields, including
vertex lighting, alpha widths, portals, scripts, and lightmap metadata. They
need explicit historical schemas; do not claim full compatibility merely
because their outer D3LV chunks can be walked. A decoder implementing this
profile should identify an unsupported version before interpreting its ROOM,
OBJS, or TERR payloads, while still allowing lossless chunk extraction.

Sources: [level reader/writer](../Descent3/LoadLevel.cpp),
[chunk/version constants](../Descent3/LoadLevel.h),
[terrain dimensions](../Descent3/terrain.h),
[production centers](../Descent3/matcen.cpp),
[goals](../Descent3/levelgoal.cpp), [ambient life](../Descent3/aiambient.cpp), and
[BSP serialization](../Descent3/bsp.cpp).

## ORF — Outrage room files

ORF has no FourCC magic. It is a sequence of I32 numeric chunk ID, I32 payload
length, and payload. Normal chunk lengths exclude both header integers and do
not introduce padding. Current room version is **4**.

| Chunk ID | Payload |
| ---: | --- |
| 5 | I32 room version; I32 vertex count V; I32 face count F |
| 0 | Historical header: I32 V, I32 F; no explicit version |
| 1 | V Vec3 positions |
| 4 | I32 texture count; that many ZString texture names |
| 2 | F face records below |
| 3 | End marker; writer emits length 4 but **no four-byte payload** |

Special-case ID 3 as a terminator: treating its declared length like a normal
payload would read past a valid file. Require a header before geometry. Unknown
nonterminal chunks can be skipped by their payload length.

Version-4 face: U8 light multiplier; I32 corner count C; Vec3 stored normal;
I16 index into the local texture table; C interleaved corner records, each
I16 position index, F32 U, F32 V, four legacy F32 lighting values, F32 alpha.
The four lighting values are still present although the writer emits zeroes.
No portal data, room flags, or OOF texture chunks are embedded here.

Versions below 4 omit light multiplier. Version zero omits per-corner F32 alpha;
versions 0 and 1 are displayed opaque by the loader. Do not discard the stored
normal merely because the editor recomputes normals after loading.

Source: [standalone room reader/writer](../editor/Erooms.cpp).

## GRP — editor room groups

Header: FourCC **D3GP**; I32 group version, currently **3**; I32 embedded
level-layout version; raw texture name table (I32 count and that many ZStrings,
without TXNM chunk framing); six I32 values: room count R, attachment room,
attachment face, object count O, door count D, trigger count T.

Next are R level room records using the declared level-layout version, without
the outer ROOM aggregate header or per-room index prefixes. Next O group-object
records, then T group-trigger records. D is descriptive metadata, not the count
of a separate following doorway array. Doorway fields already occur in rooms.

Group object: U8 type; I16 ID; U32 flags; I16 shields only for door type 17;
I32 room; Vec3 position; Mat3 orientation; U8 contained type, ID, count;
F32 life remaining. It is **not** the full D3L object record: no name, script
names, object handle, sound-source extension, or object-lightmap trailer.
Group trigger: four I16 values: room, face, flags, activator; no trigger name.

Group versions 1–2 predate the object/door/trigger counts. Version 0 is rejected
by the active group loader. The embedded level version is essential to decoding
the unframed room records.

Source: [group reader/writer](../editor/Group.cpp).

## PLT / PLD — pilot data

The current pilot file begins with I32 **43**, hexadecimal **0x2B**. There is
no separate magic. PLD files selected as control defaults use this pilot-data
family; a filename alone does not distinguish their byte encoding. All groups
are consecutive and have **no per-group length header**.

### Version-43 layout

| Order | Group | Fields |
| ---: | --- | --- |
| 1 | Version | I32 = 43 |
| 2 | Identity | ZString display name; ZString ship-model name |
| 3 | Custom multiplayer data | ZString logo; four ZString audio-taunt filenames; U16 picture ID |
| 4 | Difficulty/toggles | U8 difficulty; U8 profanity-filter enabled; U8 audio-taunts enabled |
| 5 | HUD | U8 HUD mode; U16 HUD-status mask; U16 graphical-status mask; I32 window width; I32 window height; U8 left rear view; U8 right rear view |
| 6 | Mission history | I32 count; repeated U8 highest level, U8 finished, ZString mission name, I32 restores, I32 saves, U32 ship permissions |
| 7 | Text taunts | U8 count; that many ZStrings; writer emits eight |
| 8 | Weapon selection | I16 primary count and that many U16 entries; I16 secondary count and that many U16 entries |
| 9 | Gameplay toggles | U8 count, followed by that many U8 booleans; current three are guided-main-view, show-reticle, ship-noises |
| 10 | Guide-Bot | U8 name-present; optional ZString name |
| 11 | Controls | Record specified below, through end of file |

Weapon-selection values must be kept as U16 bit patterns; do not use their
count as a byte length or strip high bits before returning decoded data.

Controls: U8 enabled-controller mask; U8 function count C; C **18-byte** records:
I32 stable action ID, I32 first input type, I32 second input type, U32 packed
configuration, U8 first input flags, U8 second input flags. Then U8 mouse-axis
count and that many F32 sensitivity values; U8 joystick-axis count and that many
F32 values; F32 keyboard ramping; U8 Mouselook enabled. Current writer emits
two mouse axes and six joystick axes. Do not substitute an array ordinal for
the stored action ID or overwrite unfamiliar bindings with current defaults.

Input-type values are: 0 none, 1 joystick axis, 2 POV, 3 joystick button,
4 key, 5 mouse axis, 6 mouse button, 7 POV2, 8 POV3, 9 POV4. The enabled-controller
mask uses 0x01 for joystick and 0x02 for mouse; keyboard bindings are separate.
Packed configuration contains two binding bytes and two controller-identity
bytes; retain its U32 value even when an input type is unfamiliar. It is not
an operating-system key code or a pointer.

In file byte order, the packed U32 consists of first binding, second binding,
first controller identity, second controller identity. Binding zero is unassigned;
controller identity 0xFF is null. Per-binding flag 0x01 inverts the input.
Axis IDs 1–6 mean X, Y, Z, R, U, V. Key values use the game's key-number namespace;
return them as game key IDs instead of interpreting them as ASCII characters.

### Older pilot versions

The version gates below describe field insertion points relative to the table.
They are cumulative, not chunk tags:

| Minimum version | Added fields |
| ---: | --- |
| 0x21 / 33 | Weapon-selection group |
| 0x22 / 34 | Third/fourth audio-taunt filenames; mission restore/save counts |
| 0x23 / 35 | Profanity-filter byte |
| 0x24 / 36 | Counted gameplay-toggle group |
| 0x25 / 37 | Final Mouselook byte |
| 0x26 / 38 | Guide-Bot name-present/name group |
| 0x28 / 40 | Audio-taunts-enabled byte |
| 0x29 / 41 | Keyboard-ramping F32 immediately before Mouselook |
| 0x2A / 42 | Per-mission ship-permission U32 |
| 0x2B / 43 | Two rear-view bytes at end of HUD group |

The older pilot.cpp serializer also contains a different legacy version family.
Do not identify that layout with this 0x21–0x2B profile simply because both use
the .plt extension. Reject versions outside the implemented profile rather
than allow a skipped insertion to shift every following control record.

Source: [pilot persistence](../Descent3/pilot_class.cpp).

## ASPF — ambient sound patterns

FourCC **ASPF**; I32 version **0**; I32 pattern count P. Each pattern contains
ZString name; F32 minimum delay; F32 maximum delay; I32 sound count S; then S
entries: ZString sound name, F32 minimum volume, F32 maximum volume, I32
probability. Probabilities are percentages; for a nonempty pattern, the engine
expects their sum to equal 100. No compressed waveform data follows these names.

Source: [ambient-pattern reader/writer](../Descent3/ambient.cpp).

## PPics.idx — pilot-picture index

I32 picture count P, followed by P entries: U8 pilot-name byte count N, N name
bytes; U16 picture ID; U8 bitmap-name byte count B, B bitmap-name bytes. The
lengths delimit the strings; they are not I32 lengths and must not be replaced
by ZString scanning. The loader appends its own terminator. Picture IDs are
stored values and need not equal the record's array index. The named images
are external resources, normally members of the separate HOG2 PPics.Hog.

Source: [pilot-picture index reader](../Descent3/PilotPicsAPI.cpp).

## MSN — text mission descriptions

MSN is line-oriented byte text. Commands are case-insensitive, whitespace separates
the first keyword from its operand, and the rest of a normal line is the operand.
It is not XML, JSON, or a binary header. Quoting and backslash escaping are not
a general MSN filename grammar; preserve spaces in the remainder of the line.

The parser begins in mission scope, then LEVEL N selects a one-based level
scope. NUMLEVELS must precede LEVEL and define its valid range. The following
dictionary is sufficient to decode the mission into metadata and level records:

| Keyword | Scope | Operand / effect |
| --- | --- | --- |
| NAME | Mission | Display name |
| AUTHOR, EMAIL | Mission | Author/contact text |
| DESCRIPTION | Mission | Description text; continuation behavior below |
| URL | Mission | Download/information URL |
| KEYWORDS | Mission | Mission keyword text |
| SINGLE, MULTI | Mission | Operand no disables that mode; other operands do not disable it |
| TRAINER | Mission | Marks training mission |
| SHIP | Mission | Default permitted ship name |
| NUMLEVELS | Mission | Positive decimal level count |
| LEVEL | Selection | Decimal one-based level number |
| MINE | Level | D3L filename |
| BRIEFING | Level | BRF filename |
| SCORE | Level | OMF filename |
| PROGRESS | Level | Progress-screen image filename |
| HOG | Either | Mission archive or level-specific extra archive |
| INTROMOVIE | Level | Intro movie filename |
| INTRODEFAULT | Level | Intro movie filename, used unless the runtime intro override is active |
| ENDMOVIE | Level | Ending movie filename |
| SECRET | Level | Decimal secret-level destination |
| BRANCH | Level | Decimal alternate destination within the mission |
| ENDMISSION | Level | Marks final level; no binary payload |

The reference implementation reads at most 127 bytes into its line buffer and
trims command/operand edges. Compatible commands start in column zero and use
keywords of at most 31 bytes. DESCRIPTION appends its operand to the existing
description; repeated DESCRIPTION lines do **not** automatically insert a
separator. A whitespace-only nonempty line while in description state appends
a newline; an empty physical line is ignored. A new nonempty command ends
description state before that command is processed. Preserve raw unknown lines
for diagnostics rather than interpret MSN as a freely indented block language.
Unknown nonempty commands are errors
for game-compatible input. MN3 derives its MSN filename from the package's
basename, with a special d3_2-to-d3 mapping in campaign handling.

Source: [mission parser and package lookup](../Descent3/Mission.cpp).

## OMF and INF-style text

### Shared InfFile lexical rules

These files use a caller-specific first-line tag, compared exactly. Following
lines contain command/operand pairs. Command token separators are spaces, tabs,
equals signs, and colons. Operands end at a comma or semicolon; subsequent pairs
may occur on the same physical line. Leading/trailing whitespace is removed
from commands and operands. This is not the Windows INI section/key grammar.

A command beginning with @ is a comment. A command beginning with # defines a
symbol, and an operand beginning with # substitutes a previously defined symbol.
Symbol names are case-sensitive. There is no general quoted-string escape
mechanism in this lexer. Preserve line/command order because definitions can
affect later operands and a line can group conditional music instructions.

### OMF theme grammar

First line is exactly **[theme file]**. The OMF command vocabulary is lowercase
and case-sensitive:

| Command | Operand / structural meaning |
| --- | --- |
| stream | Asset filename followed by $ and a symbolic stream name |
| region, endregion | Begin/end a numbered region; nested regions are invalid; numbering follows file order |
| section | One of intro, idle, combat, death, idle_combat, combat_idle; must be inside a region |
| endsection | Finish the section instruction list |
| label | Label name scoped to its region |
| goto, blt, bgt, beq, bnif | Target label in the same region |
| compare | Register reference prefixed r followed by decimal register number |
| llpt | Stream/track symbolic operand |
| seti, lcmp, lplc | Decimal numeric operand |
| inci | Increment operation, no numeric payload used |
| ifi | Decimal condition; applies to the rest of its physical line |
| play | Playback operation; engine creates internal play/wait instructions |
| wait | finished selects completion wait; other operands select the alternate wait mode |
| q2 | Recognized lexical keyword without an active emission branch in this parser |

A decoder can return ordered region/section/instruction records and external
stream names without executing the music sequencer. Resolving labels requires
a second pass or deferred references; a file's text does not contain the
engine's generated instruction addresses.

### HUD and cockpit configuration

The active HUD parser uses a **different line parser** from OMF, despite the
shared INF extension and a historical inactive implementation using InfFile.
First line is **[hud file]**, compared case-insensitively. On subsequent lines,
the first command is separated by whitespace or equals; the remainder is the
operand. Commands are case-insensitive in the main HUD parser. Lines whose
command begins @ are comments. Commas inside numeric operands separate values;
they do not introduce another command on the same line.

| Command | Operand |
| --- | --- |
| type | Decimal HUD item type |
| pos, posA, posB, textpos, reticleoffset | Two comma-separated decimal integers |
| grscale | Two comma-separated decimal floating-point scales |
| alpha, sat | Decimal opacity / saturation count |
| rgb, textrgb | Three comma-separated decimal RGB components |
| special | Mark the accumulating HUD item as special |
| create | Emit the accumulating HUD item, then reset its fields to defaults |
| reticleprefix | Bitmap-name prefix |
| ckptmodel | Cockpit OOF resource name |
| shipimg, afterburnimg, energyimg, invpulseimg | Resource filename |
| shieldimg0 through shieldimg4 | Shield image resource filename |
| fullhudinf | Another HUD configuration filename |

Cockpit-specific extension commands are compared case-sensitively except the
numbered shield commands, so lowercase spellings are the compatible choice.
If textpos is omitted when create is reached, the HUD item's text position uses
pos. A decoder can return the ordered items and resource declarations without
rendering gauges or opening the named resources. This format does not embed
bitmap bytes or an OOF model after a filename.

Sources: [InfFile lexer](../cfile/inffile.cpp),
[OMF parser](../music/omflex.cpp), [HUD](../Descent3/hud.cpp), and
[cockpit](../Descent3/cockpit.cpp).

## STR — localized string tables

STR contains ordered text records without a binary count or explicit string ID.
Language tags begin at the start of a line:

| Tag | Language / role |
| --- | --- |
| !=! | English |
| !G! | German |
| !S! | Spanish |
| !I! | Italian |
| !F! | French |
| !/! | Comment / terminates a current string |

A language-tagged line starts a new string for that language. Untagged nonempty
lines continue the current string and are concatenated **without automatically
inserting a newline**. Empty lines are ignored. A new language tag or comment
finishes the previous string. Supported backslash escapes, case-insensitively
for letters, are backslash-T for tab, backslash-N for newline, backslash-R for
carriage return, backslash-double-quote for a quote, and doubled backslash for a
backslash. A backslash followed by decimal digits consumes all consecutive digits
and emits the resulting value modulo 256 as one byte; it is decimal, not a
three-digit octal escape. Unknown/trailing escapes should be reported rather
than copied using the reader's uninitialized temporary value. Retain original
bytes if the desired export encoding is unknown.

Select one language and enumerate only that language's records in file order;
their ordinal becomes the string-table index. If a file has no strings for a
requested non-English language, the loader falls back to the English table as
a whole, not arbitrary per-string language mixing. Do not sort strings or treat
repeated identical text as the same resource.

Source: [localization parser](../Descent3/localization.cpp).

## MSG — OSIRIS module messages

MSG is line-oriented text with no magic or binary length. Strip leading and
trailing line whitespace. Ignore empty lines and lines starting with two forward
slashes. Split a message at the **first equals sign**: the bytes before it are
the message key, the bytes after it are the message text. Further equals signs
belong to the text. Lines without an equals sign are ignored by the generated
module reader. It does not implement JSON quoting or the STR language-tag grammar.

The reader trims the whole line before splitting, not both fields afterward;
for exact behavior, retain spaces adjacent to the equals sign as part of the
corresponding field. Message lookup is by key. The module chooses localized
filenames separately; language suffixes do not insert a header into the file.

Source: [generated level-module message reader](../scripts/level1.cpp).

## BRF — TelCom briefings

BRF is a text scene-description format. Directives begin with **$ in column
zero**. Directive and option names are case-insensitive. A semicolon in column
zero is a comment outside a text block. Whitespace separates tokens; commas
separate numeric tuple components. Double-quoted strings end at the next double
quote; the parser does not implement C/JSON escaped quotes. Preserve physical
newlines in text blocks.

| Directive | Decoder structure |
| --- | --- |
| $title | Quoted title, once per file |
| $static, $glitch | One decimal floating-point amount |
| $screen | Integer screen ID, optional parameters, then start |
| $endscreen | End current screen |
| $text | Optional text parameters ending in show; following literal lines form text |
| $endtext | End text block; literal lines are joined with newline characters |
| $bitmap | Optional bitmap parameters ending in show, followed by quoted asset filename |
| $movie | Optional movie parameters ending in show, followed by quoted filename |
| $sound | Options ending in play, followed by quoted filename |
| $voice | Options ending in play, followed by quoted filename; at most one per screen |
| $button | Integer X,Y followed by button options, show and quoted image names |
| $poly | Three comma-separated position floats, three orientation floats, three rotation-rate floats, then quoted model name |

### Option arguments and boundaries

In the following tables, integer and float mean decimal text tokens, not binary
fields. A flag consumes no argument. A quoted string consumes both quotes and
the bytes between them. Options may appear in any order before the directive's
start, show, or play terminator; those terminators are bare tokens. Tuples
require commas between components. No record length is stored, so a decoder
must recognize each option's argument count rather than split all words into
independent options.

| Shared option | Arguments | Accepted by |
| --- | --- | --- |
| desc | One quoted description | screen, text, bitmap, movie, sound, voice, button |
| isset | One integer bit number, 0–31 | screen, text, bitmap, movie, sound, button |
| isnset | One integer bit number, 0–31 | screen, text, bitmap, movie, sound, button |
| starttime | One float delay in seconds | text, bitmap, movie, sound |

Repeated isset and isnset options accumulate required-set and required-clear
mission-flag masks. A voice branch attempts mask handling using the wrong
token buffer; it has no dependable mask semantics. Preserve such input as an
unsupported voice option rather than reproduce an uninitialized-buffer read.

| Directive | Additional options and exact arguments |
| --- | --- |
| $screen | layout: one quoted layout filename; start: ends options |
| $text | id: one integer; tabstop: flag; scroll: flag; effect: one effect token; speed: one float; color: three comma-separated integers R,G,B; font: one token sm_brief or lg_brief; box: four comma-separated integers **left,right,top,bottom**; show: ends options and starts the following lines of text |
| $bitmap | no_early_rend: flag; effect: one effect token; speed: one float; position: two comma-separated integers X,Y; show: ends options and is followed by one quoted filename |
| $movie | position: two comma-separated integers X,Y; fps: one float; looping: flag; show: ends options and is followed by one quoted filename |
| $sound | once: flag; play: ends options and is followed by one quoted filename |
| $voice | 8bit, compressed, stereo: flags; play: ends options and is followed by one quoted filename |

Text effect tokens are none, flash, scroll_l2r, scroll_r2l, scroll_t2b,
scroll_b2t, fade_in_and_out, fade_in, fade_out. Bitmap tokens are none,
fade_in, fade_out, blur_in, blur_out, scan_in, scan_out, invert_in, invert_out,
stretch_in, stretch_out. Tokens are case-insensitive. The bitmap parser accepts
fade tokens but maps them to its static-bitmap path. Voice flags describe intended
playback; they do not change the encoding of the referenced file. The default
voice depth is 16 bits; this parser consumes the voice declaration but its
playback callback is disabled.

### Buttons

After $button, read two comma-separated integers X,Y, then options through show.
After show, read two quoted filenames: normal image, focused image. Button
options are desc, isset, isnset as above, plus:

| Option | Exact arguments |
| --- | --- |
| id, parent_id, sibling_id | One integer each |
| type | One token Down, Up, Next, Prev, Quit, or Jump; **Jump additionally consumes one integer destination-screen ID** |
| click | One token HoldDown, ClickDown, or ClickUp |
| flash | One float period, then two quoted filenames: alternate normal image and alternate focused image |
| glow | Flag |

If glow is present **without flash**, two further quoted filenames follow the
normal/focused filenames after show: glow-normal and glow-focused images. If
flash is also present, those alternate filenames were already supplied by flash
and no further pair follows show. Defaults are type Prev, click ClickUp, and
IDs −1. Image references are names, not embedded OGF records.

For a structural decoder, preserve each option's spelling, ordered arguments,
quoted strings, and directive/text boundaries. Rendering effects and resolving
named game assets are separate operations. Do not interpret dollar-prefixed
lines as literal text merely because they appear during a text block: the
reference parser dispatches directives before considering literal text.

Source: [briefing parser](../Descent3/BriefingParse.cpp).

## Savegames — native snapshots and count companions

Slot files are normally named saveg000 through saveg007 under savegame, without
a mandatory extension. A same-basename **.cnt** companion contains one I32
number of game restores. It does not contain another save or a checksum.

### Independently decodable save prefix, version 2

| Field | Encoding |
| --- | --- |
| Description | FixedString(32) |
| Save version | I16, currently 2 |
| Snapshot present | U8 |
| Snapshot | One embedded OGF if present; use exact OGF consumed length |
| Translation tables and world state | Version/build-dependent data described below |

There is no SAVE magic. A decoder can always bound-read the description/version
and, for supported OGF types, extract the preview. A source build's optional
verification macros can additionally instrument writes; do not assume every
developer build has retail-identical bytes.

The remaining section order is: resource translation tables; mission level and
mission filename; mission flags and clocks; waypoint and music region; weather;
active doors; rooms; triggers; objects; players; production centers; visual
effects; spew; OSIRIS state; goals; game-message console; special HUD state.
The mission portion uses I16 level, ZString mission filename, U32 game-state
flags, F32 game time, I32 frame count, I32 waypoint, I16 music region. Weather
is I32 byte length followed by that many raw bytes. Active doors contain I32
capacity, I32 active count, then capacity I32 door indices.

### ABI compatibility boundary

Save serialization writes native memory for player, AI, movement/control/render
unions, weather, lighting, visual effects, and other structures. Some records
are size-prefixed; others use the compiled structure's size without storing it.
Member widths, padding, pointer size, compile options, and structure revisions
can therefore affect the file despite the same save version. Little-endian
primitive helpers do not make those raw structures architecture-independent.

A full semantic snapshot decoder requires an **explicit ABI profile** for the
producer: platform, 32-/64-bit data model, structure layout/version, and optional
verification mode. There is no reliable byte-level inference of all these
parameters from the version-2 header. For a file without a matching profile,
return description/preview plus the remaining opaque snapshot bytes and a
clear unsupported-ABI result. Do not fabricate offsets, scan for guessed tags,
or deserialize pointer values as addresses in a tool's process.

This is an actual limitation of the format. The portable field tables here
are not a claim to specify the native structure interiors. Formats such as
D3L and GAM should be preferred for independently decoding game assets.

Sources: [save order and native writes](../Descent3/gamesave.cpp),
[save version](../Descent3/gamesave.h), and
[OSIRIS snapshot serialization](../Descent3/OsirisLoadandBind.cpp).

## DEM — recorded gameplay

The header begins with ZString **D3DM1**, or legacy **D3DEM**, followed by I16
save-format version, ZString mission filename, I32 level, F32 game time, I32
frame number. The current header then stores resource translation tables,
rooms, triggers, objects, players, visual effects, spew, OSIRIS state, and
I16 player number. These reuse native save routines, with the ABI limitations
above; the event stream does not necessarily begin at a fixed offset.

Once a matching snapshot profile has established the event-stream start,
each event begins with U8 type. Events do **not** have a universal payload
length. Unknown types cannot safely be skipped by scanning for the next byte
that looks like an event ID.

| ID | Meaning | ID | Meaning |
| ---: | --- | ---: | --- |
| 1 | Object state | 14 | Radius attachment |
| 2 | New frame | 15 | Detach |
| 3 | Weapon fire | 16 | Weapon-fire flags |
| 4 | HUD message | 17 | Player state |
| 5 | 3D sound | 18 | MSAFE script packet |
| 6 | Create object | 19 | Powerup packet |
| 7 | Object animation | 20 | Cinematics packet |
| 8 | Turret animation | 21 | Persistent HUD message |
| 9 | Object explosion | 22 | Mark object dead |
| 10 | Player death | 23 | Player rotating balls |
| 11 | Player/weapon collision | 24 | Player type change |
| 12 | Generic/weapon collision | 25 | Set remaining object lifetime |
| 13 | Attach | 26 | 2D sound |

### Event payloads

Each row lists the complete event payload after its U8 type. Vec3 and Mat3 use
the primitive float layout defined above, without native structure padding.
Object references use recorded object numbers; preserve a mapping as objects
are created and removed. Counts in the tables apply to the producer profile at
the stated baseline, including **21** player weapon-ammo slots.

| ID | Payload |
| ---: | --- |
| 1 | I16 object number; I32 room; Vec3 position; Mat3 orientation; **U32 player flags only if that object's current type is player (4) or observer (22)** |
| 2 | F32 game time, F32 frame duration |
| 3 | F32 game time; U16 firing-object number; U32 weapon-resource checksum; Vec3 position; Vec3 direction; U16 newly created weapon-object number; I16 gun index |
| 4 | U32 text color, U8 blink, ZString message |
| 5 | I16 object number, I16 sound index, F32 volume |
| 6 | U8 object type; U8 has-orientation; I16 resource ID; I32 room; Vec3 position; I32 parent handle; Mat3 orientation only if has-orientation is nonzero; U16 new object number |
| 7 | F32 game time; I16 object; F32 animation server time; I16 server animation frame; U8 start frame; U8 end frame; F32 animation time; F32 maximum speed; U8 flags; I16 animation sound index |
| 8 | F32 game time; I16 object; F32 turret time; U16 turret count T; T F32 keyframe values |
| 9 | I16 victim, U16 killer (0xFFFF = none), F32 damage, U32 death flags, F32 delay, I16 seed |
| 10 | I16 player-object number, U8 melee, I32 fate |
| 11, 12 | I16 target object; I16 weapon object; Vec3 collision point; Vec3 collision normal; U8 reverse-normal flag |
| 13 | I16 parent object; U8 parent attachment index; I16 child object; U8 child attachment index; U8 aligned flag |
| 14 | I16 parent object; U8 parent attachment index; I16 child object; F32 attachment radius |
| 15 | I16 child object |
| 16 | I16 object; U8 weapon-fire flags |
| 17 | I16 energy; I16 shields; 21 I16 weapon-ammo counts; I32 primary-weapon index; I32 secondary-weapon index; U32 weapon flags; F32 remaining afterburner time; F32 field of view |
| 18, 19, 20 | U16 byte length L; L packet bytes; preserve the MSAFE, powerup, or cinematics packet respectively as an opaque application payload |
| 21 | U32 color; I32 X; I32 Y; F32 duration; U32 flags; I32 sound index; I16 message byte length N including zero terminator; N message bytes |
| 22 | I16 object number |
| 23 | I16 player slot; U8 ball count B; if B is positive, F32 speed followed by B triples of F32 red, green, blue; if B is zero, no speed or color fields |
| 24 | U8 player slot; U8 new object type; type 4 adds U8 stop-observing flag; type 12 adds nothing; type 22 adds I32 observer mode and, **only if mode is 1**, I32 piggyback-object number |
| 25 | I16 object number; U8 uses-lifetime; F32 remaining lifetime only if uses-lifetime is nonzero |
| 26 | I16 sound index, F32 volume |

The type condition in event 1 depends on the decoded initial object table and
subsequent type-change events; it cannot be inferred from remaining bytes.
Event 24 defines tails only for player 4, ghost 12, and observer 22. Event 6's
writer produces robot, powerup, clutter, building, and camera creations; its
resource ID is translated according to object type. Retain handles and resource
checksums as stored values rather than treating them as host pointers.

The reader bounds rotating-ball counts to four. Reject negative message
lengths and lengths outside the containing input. Events 18–20 are fully
length-delimited at the recording layer; interpreting their application packet
contents is outside this file specification. A structural decoder can preserve
them and continue. Physical EOF ends the event stream; no explicit end event
is emitted. An unknown event ID or an unresolved event-1 type must stop decoding
with an offset diagnostic, since there is no generic event length to skip.

Sources: [event IDs](../lib/demofile.h) and
[header/event reader and writer](../Descent3/demofile.cpp).

## Editor and user metadata

### table.lok and TableVersion

The editor's table.lok is a binary sequence of **61-byte** records through EOF,
with no signature, count, terminator record, or padding. Each record is U8 page
type, FixedString(30) page name, FixedString(30) holder name. The strings can
occupy all 30 bytes; the engine adds a terminator in memory. Page types use the
GAM directory above. The reader maps legacy robot type 3 to generic type 10.
The holder string UNLOCKED conventionally marks a free lock. Preserve other
holder names as text; they are not resource IDs. Reject a partial final record.

TableVersion is a separate file containing one I32, currently **22**. It tracks
editor table-management compatibility, not a global GAM serialization version.
Do not use it in place of the I16 version at the beginning of each GAM page.

Sources: [lock/version serialization](../manage/pagelock.cpp) and
[fixed field length and page types](../lib/manage.h).

### MPS — saved multiplayer settings

MPS is a text file with one keyword and value per physical line. The writer
separates them with a tab. The reader splits on spaces and tabs and compares
keywords case-insensitively. There is no magic, section header, quoting, escape
syntax, or binary count. Unknown keywords are ignored. A blank physical line
ends this particular reader's loop; compatible output should avoid blank lines.

| Keywords | Value grammar |
| --- | --- |
| NAME, MISSION, SCRIPT | Rest of line as text; the reader reconstructs whitespace-separated tokens using single spaces |
| PPS, MAXPLAYERS, RESPAWNTIME, KILLGOAL, TIMELIMIT, DIFFICULTY | One decimal integer |
| PEERPEER, PERMISSABLE, RANDOMIZERESPAWN, ROTVEL, USESMOOTHING, BRIGHTPLAYERS, ACCWEAP, MLOOK | One token TRUE or FALSE, case-insensitive |
| OBJBAN, SHIPBAN | One banned object/ship name; repeat the keyword for further bans |

The spelling **PERMISSABLE** is part of the format. Ban values are read as the
first token only, despite the writer accepting resource names containing spaces.
A lossless decoder should retain the entire original value and expose that
reader limitation. Missing settings retain the caller's current values; the
file is an overlay, not a complete definition of all multiplayer state.

Sources: [MPS reader/writer](../Descent3/multi_save_setting.cpp) and
[extension selection](../Descent3/multi_ui.cpp).

### .Descent3Registry — Linux preferences

This is a text serialization of a two-level key/record database. A section line
starts in column zero with an opening square bracket and contains a key name
through the closing bracket. Following record lines start in column zero with
a double-quoted record name, an equals sign, and either a double-quoted string
or the literal **dword:** followed by hexadecimal digits. DWORD output has no
0x prefix and represents a 32-bit bit pattern. A section must precede its records.

Names and string values end at their next double quote; there is no backslash
escape convention. The writer emits adjacent delimiters without surrounding
spaces. The importer identifies dword by the lowercase d immediately after the
equals sign; preserve the emitted lowercase marker for compatibility. Lines
starting with neither bracket nor quote are ignored. Database key and record
lookup is case-insensitive. This is not Windows Registry's .reg export format
and does not carry its standard header.

Sources: [text reader/writer](../linux/registry.cpp) and
[preference filename](../linux/lnxdata.cpp).

### game-assets.cfg — this checkout's asset-location file

One UTF-8 directory path followed by LF. The reader reads only the first line
and removes a trailing CR when present, so CRLF also works. There are no field
names, quotes, escapes, comments, or INI sections. Spaces belong to the path.
The writer stores a canonical absolute path and rejects embedded CR/LF. An
empty/missing first line means no configured location. This is a modern
checkout-specific setting, not a commercial game-asset format.

Source: [asset-location serialization](../Descent3/game_assets.cpp).

## Decoder validation and coverage boundaries

The layouts were checked against the reader/writer source at the pinned commit.
An independent, read-only binary parser also checked the following installed
game-data samples. It used the field sequences in these documents and required
exact record consumption, allowing only the specified level-chunk padding.
The commercial files are not distributed with this documentation.

| Sample set | Count | Checks performed |
| --- | ---: | --- |
| OGF images | 2,437 | Headers, per-level repeat packets, exact pixel counts and file consumption |
| OOF models | 850 | Versions 2200/2300; known chunk fields, geometry, point tables, animation lengths and chunk boundaries |
| OAF animations | 62 | Headers and every embedded OGF boundary |
| FNT fonts | 7 | Optional tables and exact aggregate glyph-data size |
| GAM table | 1 file, 4,385 pages | Every texture-7, weapon-8, door-3, ship-6, sound-1, gamefile-2 and generic-27 record through its page boundary |
| D3LV levels | 42 | Versions 127/128/132; every encountered documented chunk layout, including rooms, objects, terrain, lightmaps, navigation, goals, production centers and editor data |
| OSF streams | 7 | Footer signature and payload bounds; compressed ACM audio was not decoded |

No megacell pages occurred in that sample table. Historical profiles, text
dialects, standalone ORF/GRP, pilot records, and native snapshot layouts were
reviewed from source rather than validated by that binary sample pass. Sample
coverage is evidence for the tested profiles, not a guarantee that every
historical exporter produced the same bytes.

For all supported binary profiles, validate multiplication/addition before
allocating, bound every string and record to its enclosing chunk/member, and
return exact consumed lengths for embedded images and compressed arrays. Keep
raw IDs, flags, unknown records, and text bytes when round-tripping. Do not
resolve resource names by opening arbitrary host paths merely to parse a file.

The portable schemas in these two documents can be implemented independently
for their stated versions. Historical command dictionaries, D3LV versions
earlier than 127, application packet interiors, and raw native save/demo
structures are outside that claim. Unsupported profiles should produce a
diagnostic with format/version/offset and preserve their enclosing bytes.
