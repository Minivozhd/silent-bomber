# part2 (mission/level data) format — Silent Bomber (SLUS_009.02 USA)

Status legend: **[VERIFIED]** = proven by disassembly + byte-level evidence;
**[HYPOTHESIS]** = strong indication, not fully proven; **[UNRESOLVED]**.

Companion parser: `experiments/part2_parse.py` (dumps any package;
sample output: `experiments/out/P01.part2_parsed.txt`).
Code extents per package: `decomp/configs/USA/overlays/codemap.json`.

## 0. Container recap [VERIFIED]

part2 of a 9-word package = ONE raw CD read of `w3+w4+w5` bytes at
`file_off = desc.w0 + desc.w2`, loaded at `packageArenaBase` (vram
`0x800DD4F0` for P00–P27/A00–A03; `0x801A0400` for ARENAP). In RAM:

```
[base          , base+w3)        header + tables + strings + .text + rodata
[base+w3       , base+w3+w4)     sound-bank stream A ("registration records")
[base+w3+w4    , base+w3+w4+w5)  sound-bank stream B, then pad
```

Loader: `func_80012E10` (asm/USA/main/pkgload.s). `.text` (MIPS overlay)
lives inside the w3 region (per-package extents in codemap.json; P01:
`0x6C34..0x1ACC0`).

## 1. Region map for P01 (part2 size 0x4CFE4, w3=0x1B6BC, w4=0x4A4C, w5=0x2CEDC)

| offset | content | status |
|---|---|---|
| 0x0000 | u32 = 7, then three 0x2C-byte records @0x04/0x30/0x5C | [UNRESOLVED] semantics |
| 0x0088 | mixed block: numbers, `01 01 01 01`, u32 3, u32 3, self-relative ptrs (`->0x1ACD8`, `->0x88`, `->0x94`, `->0xA0`, `->0x1ACE4`) | [UNRESOLVED] |
| 0x00C0 | 0x10-byte records `{x,y,z,0}` s32 16.16 fixed-point (e.g. `001d7100 ffe92500 00001000 0` = (29.44, -22.86, 16.0)) | [HYPOTHESIS] waypoints/camera points |
| 0x0180 | 5 function ptrs into overlay code (`->0x6F40 704C 70FC 7208 748C`) + more coord records | [HYPOTHESIS] script/event handlers |
| 0x0630 | structs with color quads + ptr (`->0x1614`), `3f3f`="??" placeholders | [UNRESOLVED] |
| 0x0740 | groups of 5 fn-ptrs (`->0xEDAC EDD8 EDD0 EDD8 ...`) | [HYPOTHESIS] trigger handlers |
| 0x08F4 | ptr table -> 0x14000-0x14700 region | [UNRESOLVED] |
| 0x0958 | 0x10-byte records with self-ptrs | [UNRESOLVED] |
| 0x0D88 | 7 x 0x18-byte records (see §3) | [HYPOTHESIS] trigger zones |
| 0x0E30 | `{->0x1AE20, ->0x1AE2C, ->0x1AE38, 3}` | [UNRESOLVED] |
| 0x0E40 | entity-list header: `{->0x0E30, 7, ->0x0D88, ->0x0E58, ->0x16090}` (0x16090 = MIPS fn, `27bdffe8 afbf0014` prologue) | [HYPOTHESIS] |
| 0x0E58 | **33 x 0x24 entity/spawn records** (see §2) | [VERIFIED] |
| 0x12FC | terminator: `{->0x1300 ("gm01bara"), 0}` | [HYPOTHESIS] |
| 0x1300 | object-name strings: `gm01bara gmItem gm01smkg gm01ocar gm01dcan gm01pole gm01wal3 gm01wal2 gm01wal1`, then `ERROR` | [VERIFIED] |
| 0x1380 | ptr table (13 ptrs -> 0x17558-0x182C4) | [UNRESOLVED] |
| 0x13BC | u16-pair table (`000d0015 0000000f ...`) | [UNRESOLVED] |
| 0x13FC | animation-sequence tables: `{u16 frame,u16 id}*` terminated by `0xFFFFFFFE`/`0xFFFFFFFD` + one ptr (e.g. ->0x5DC8) | [HYPOTHESIS] |
| 0x14DC | ptr table (12 ptrs -> 0x19E34-0x1A240) | [UNRESOLVED] |
| 0x1510 | s16-pair pool + `{ptr,ptr,0x00010001}` triplets | [UNRESOLVED] |
| 0x602C | dialogue strings ("all I knew of the world.", "Jutah! Jutah! Come in!", ...) | [VERIFIED] |
| 0x6C34 | **.text** MIPS overlay (codemap: text_start 0x6C34, text_end 0x1ACC0) | [VERIFIED] |
| 0x1ACC0 | rodata: jump tables, color tables, **dialogue/script pointer table** @0x1AFD0+ (ptrs -> strings @0x602C+) | [VERIFIED] |
| 0x1B6BC | w4 region: sound stream A | [VERIFIED] |
| 0x20108 | w5 region: sound stream B | [VERIFIED] |

## 2. Entity/spawn records (0x24 bytes) [VERIFIED]

Detected in **every** mission package (P01–P27, A00) as runs of 0x24-byte
records; each record ends with a pointer to an object-name string:

```
+0x00 u32   tag = 0x000100NN   hi u16 always 1; lo u16 = activation group
            (records sharing NN spawn/despawn together — P01: group 2 =
            a dcan + 2 ocar + 3 gmItem; group 3 = 4 poles + gm01bara) [HYPOTHESIS]
+0x04 u32   always 1
+0x08 u32   flags; byte3 = facing (values 0/04/08/0C/0E ~ 45° steps) [HYPOTHESIS]
+0x0C ptr   constructor function — overlay code (e.g. P01 gm01pole ->
            0x9C08) or main exe (gmItem -> 0x8006F750, allocates a
            0x4C-byte object and installs vtable 0x8006F724/0x8006F204)
+0x10 s32   x   ) 16.16 fixed-point world position
+0x14 s32   y   )  (verified: gm01wal1 = 0x00180000/0xffeb0000 = 24.0, -21.0)
+0x18 s32   z   )
+0x1C u32   class parameter — gmItem: 0x0000NN01 (see §5)
+0x20 ptr   -> object-name string (gm01pole, en02mgun, gmWarp, gmItem, ...)
```

P01 evidence: 33 records @0x0E58..0x12FC (`33*0x24 = 0x4A4`), names resolve
to the string table @0x1300. Other packages: P02 61 records @0x12D8
(`gm02box` x14, `gmItem` x14, `en02mgun` x9, `gm02tank` x8, ...),
P04 93, P07 87, P08 87 (incl. `en08Boss`), P11 97 (incl. `en11Boss`).

Object-name prefixes: `gmXX*` = scenery/gadgets (mission-specific code),
`enXX*` = enemies/turrets/bosses, `gmItem` = pickups, `gmWarp` = warp points.

## 3. Trigger-zone records (P01 @0xD88, 7 x 0x18) [HYPOTHESIS]

```
+0x00 u32   0x00280400 / 0x00280600  (flags/type)
+0x04 4x u8 channel/speaker ids?  (01 01 02 01 / 02 01 01 02 / ...)
       or ASCII labels: 01 01 'A' 'A', 02 01 'B' 'B', 03 01 'C' 'C'
+0x08 u32   e.g. 0x000A2801 / 0x00000001
+0x0C s32   x (16.16)   \ zone center
+0x10 s32   y (16.16)   / (0 when unused)
+0x14 u32   0
```

The count `7` sits in the entity-list header @0x0E44, pointer @0x0E48.

## 4. Sound-bank "registration records" (w4/w5) [VERIFIED]

The w4 fixup pass is **not** entity registration — it registers VAB sound
banks. Disassembly chain: `func_80012E10` → `func_80012D34(a0=base+w3,
a1=base+w3+w4, a2=2)` → per group `func_80019224(A_i, B_i, idx)` →
`func_800964E0` (SPU driver) returning an s16 handle stored into the 4-slot
table `D_800B5C88` (count at `D_800B5C96`); index base 2 for missions, 0 for
boot_main (`func_80013000`, args swapped).

Both streams share one format: `[u32 count][count u32 payload]` groups,
terminated by a zero count:

- stream A (w4 region): payloads start with **`pBAV`** (VAB header magic;
  P01 @0x1B6C0: `70 42 41 56`). P01: 2 groups of 2056/2696 words,
  terminator @0x20104 = exactly w3+w4-4.
- stream B (w5 region): same grouping, waveform/sample data
  (P01: 2 groups of 22768/23236 words; second group ends exactly at
  part2 end-4).

So per mission: stream A[i] = VAB header i, stream B[i] = its waveforms.

## 5. Randomizer-relevant tables

### 5.1 gmItem — pickups/drops [VERIFIED structurally]

- gmItem records use the main-exe class `func_8006F750`; their position is
  (0,0,0) and the record immediately **follows its container** record
  (`gmXXbox`/`gmXXcbox`, which carries the real position) inside the same
  tag group. The container spawns the item on destruction. [HYPOTHESIS for
  the linkage mechanism; adjacency verified in P02/P06/P07 dumps]
- Item id = `param & 0xFF00 >> 8` with `param = 0x0000NN01`
  (one exception: P10 has a gmItem with param=0 — "empty").
  Observed ids and counts across all missions:
  `{0:62, 1:20, 2:8, 4:31, 5:24, 6:22, 7:1, 8:1, 10:1, 11:1, 12:1, 13:1, 14:1}`
  → **item randomization = rewrite byte1 of the gmItem record's +0x1C word.**
- Container (`gmXXbox`) param field takes values 0..3 / 0x100 / 0x101
  [UNRESOLVED] (box variant/behaviour, not the item id).

### 5.2 Enemy/scenery spawn placement [VERIFIED]

All enemy/unit/prop placement lives in the §2 entity tables with plaintext
16.16 positions — directly editable (e.g. `en02mgun` turrets in P02,
`gmWarp` points, `enXXBoss` records). The ctor ptr selects behaviour;
swapping ctor pointers between same-class records is another rando axis.

### 5.3 Unit stats [UNRESOLVED]

Unit packs (§6) contain **no stats** — only TIM + QMD. Enemy HP/damage are
set by code: either the per-mission overlay ctors (`.text` in part2) or
main-exe class defaults. No data stat table found in part2.

### 5.4 Dialogue [VERIFIED structurally]

Mission text = plain ASCII strings before `.text` (P01 @0x602C+),
referenced by a pointer table in post-.text rodata (P01 @0x1AFD0+).
Script logic itself is compiled MIPS in the overlay.

### 5.5 Arena (A00) name table [VERIFIED]

A00 part2 starts differently: `u32 count (0x22)` + 34 NUL-terminated
C-strings @0x04 — boss/weapon display names ("NEKO PUNCH", "NEKO MISSILE",
"NEKO TANK", "HOLOGRAM", "BENOIT", "BUSTER RIFLE", "MERCURY", "PL-2313",
"PLASMA SHOT", "BLUE MIST", "BEAM SWORD", "SAMURAI", "HOMING SHOT",
"SOLID HUNTER", "MACHINE GUN", "GUNNER", "BEAM SLUG", "BEAM CAN...", ...).

## 6. Unit packs (grpA/grpB) [VERIFIED]

Descriptor tables in SLUS `.data`: `D_800AC448` (grpA = player 1) and
`D_800AC538` (grpB = player 2) — 12 pointers each to 4-word descriptors
`{file_off, csize, delta, tail_size}`; the 12 units in order:
**LJUT, BTNK, PWSN, 4TNK, BTOR, PWSS, PWSB, BIOM, LJHO, PWSM, LBEN, NEKO**
(identity proven by QMD block names `X1<NAME>00`).

Loader `func_800132D8`: thread+0x4C = unit index (0–11), thread+0x48 =
player flag (0 → grpA loaded at 0x80180000, else grpB at 0x80190000).

Pack anatomy (example LJUT grpA: off 0x13F8800, csize 0xC30, delta 0x1000,
tail 0x27E4):

```
file_off .. +csize            SB-RLE stream -> TIM texture(s)
                              (LJUT: `0a030000 250c0000 40180000 00 00` ->
                              0x1840 bytes = TIM 4bpp+CLUT, 96x64 @(832,0))
file_off+delta .. +tail_size  raw: bare QMD chain (no pQES wrapper)
                              LJUT: X1LJUT00, C1LJUT00
                              NEKO: E1NEKO00, C1ARIS00, C1ASTEA0, C1AFREA0,
                                    G1BOMB00, G1BOMB01, X1NEKO00, X1NMIS00,
                                    E1NEKO01  (chain end == tail end)
```

Block-name prefixes: `X1*` = unit body model, `C1*` = character/pilot
models, `E1*` = effects, `G1*` = bombs. grpB mirrors grpB with `X2*/C2*/E2*`
names (P2 palette/variant; sizes differ by a few bytes).

## 7. Open questions

- Semantics of the part2 header at +0x00 (u32 + 0x2C records; the three
  homogeneous P01 records contain 16.16 values in [-1,1), last data word ==
  3rd data word (0x1400) — possibly normalized vectors; count 7 does not match the 3 homogeneous
  records, so the table is heterogeneous). Who parses it: presumably
  main-exe mission init via `packageArenaBase` — not yet located.
- The 0xE30/0xE40 header structs (entity-list discovery) and the mixed
  block @0x88.
- How `enXX*` records bind to unit-pack models (no pack index found in the
  record; likely bound in the overlay ctor code).
- Script/event bytecode format of the 0x5000-0x6000 region (P01) and the
  `0xFFFFFFFE`-terminated frame tables @0x13FC.
