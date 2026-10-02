# QMD model/scene format (Silent Bomber, part3 of DATA.BIN packages)

Status legend: **[VERIFIED]** / **[HYPOTHESIS]** / **[UNRESOLVED]**.
Reference parser: `experiments/qmd_parse.py` (walks, renders, exports OBJ).

## Container

- `part3` of a package = `pQES` container: 16-byte header
  (`"pQES"`, u32 `0x01000000`, u32 `0x710BE001`, u32 `0x000203B0`), then one
  or two **script sections** (see below), then the QMD block chain.
  [VERIFIED across P01/P02/P05/A00]
- Each script section starts with its own `pQES` header (identical words) and
  ends with terminator `FF 2F 00 00` (padded to 4). P01: sections at
  0x10..0x478 and 0x488..0x924, QMD chain at 0x924. P02: 0x10..0xBA4,
  0xBB4..0x1A24. A00 (arena): single section 0x10..0x2A8.
- The script sections are a compact bytecode/record stream (opcodes like
  C0..C5 with u16 args in the prologue; a regular `XX 5A ...` body with
  7/8-byte records). [UNRESOLVED — interpreter not located yet; likely poses
  model parts / animates scene objects]
- Unit packs (`grpA_XX.part2_raw.bin`) hold a bare QMD chain (no pQES). [VERIFIED]

## QMD block

```
+0x00  char[4]  "QMD "
+0x04  char[8]  name (SMP01B01 scene piece / GMTANK01 unit / EMBTNK00 boss…)
+0x0C  u32 kind: low u16 = 1 → simple block; N > 1 → complex (N parts)
+0x10  (simple) u32 f0  offset of section A (face chunks)   rel. to +0x10
+0x14         u32 f1  offset of section B (vertices: s16 x, s16 z per vertex)
+0x18         u32 f2  offset of section C (s16 y per vertex)
+0x1C         u32 f3  offset of the extra section (== f4 when absent)
+0x20         u32 f4  offset of block end (next block = block + f4 + 0x10)
+0x24         u32 f5  vertex count
+0x28         u32 f6  offset to related (animation?) data [UNRESOLVED]
+0x2C         u32 g0, g1                             [UNRESOLVED]
```
Block size chain verified across P01/P02/A00 (every block's next-tag lands at
`block + f3 + 0x10`). [VERIFIED]

### Section B/C — vertices (simple blocks)

Per vertex: B holds `(s16 x, s16 z)`, C holds `(s16 y)` — split pool pairs.
[VERIFIED by renders]

### Section A — face chunks (simple blocks)

Chunked: `u16 count, u16 type`, then `count` records; `0,0` ends the section.
Vertex indices are stored ×2 (read `u16 >> 1`). Record layouts:

| type | GPU sense | size | layout |
|---|---|---|---|
| 0 | quad, gouraud | 36B | 4×u16 idx, 4×3B normals, 4×4B colors |
| 1 | tri, gouraud | 28B | 3×u16 idx, 3×3B normals(+pad), 3×4B colors |
| 2 | quad, flat | 24B | 4×u16 idx, 4×3B normals, 1×4B color |
| 3 | tri, flat | 20B | 3×u16 idx, 3×3B normals(+pad), 1×4B color |
| 4 | quad, gouraud, no normals | 24B | 4×u16 idx, 4×4B colors (color layout approximate) |
| 5 | tri, gouraud, no normals | 24B | 3×u16 idx, 3×4B colors (approximate) |
| 6 | two-quad strip | 28B | 2×(4×u16 idx), 1×4B color (approximate) |
| 8 | quad, textured gouraud (GT4, cmd 0x3C) | 32B | 4×u16 idx, uv2,uv3, uv0,cba, uv1,tpage (12B), rgb+cmd, 4×u16 nidx×2 [VERIFIED] |
| 9 | tri, textured gouraud (GT3, cmd 0x34) | 28B | 3×u16 idx, uv2, uv0,cba, uv1,tpage (10B), rgb+cmd, 3×u16 nidx×2 + pad [VERIFIED] |
| 10 | quad, textured flat (FT4, cmd 0x2C) | 32B | 4×u16 idx, uv2,uv3, uv0,cba, uv1,tpage (12B), rgb+cmd, 8B tail |
| 11 | tri, textured flat (FT3, cmd 0x24) | 28B | 3×u16 idx, uv2, uv0,cba, uv1,tpage (10B), rgb+cmd, 8B tail |

Sizes VERIFIED by chunk-landing analysis (records must land on the next chunk
header / terminator / section end) across P01, A00, P02 and unit packs — zero
mismatches. Types 8–11 embed literal PS1 GPU `cba`/`tpage` words — VERIFIED
against part1 TIM destination rects, e.g. GMDORA10 → TIM11 (cba 0x6017 = clut
(368,384), tpage 0x001D = image (832,320)), GMOCAR10 → TIM20/21 (cba 0x5014 =
clut (320,320), tpage 0x001C), grpA_00 unit → cba 0x3E37 = clut (880,248),
tpage 0x000D = image (832,0). UV order is `[uv0][uv1][uv2][cba][uv3][tpage]`
for quads and `[uv0][uv1][cba][uv2][tpage]` for tris [VERIFIED by TIM rect
match]. The single modulation color is the `rgb+cmd` trailer word (offset 20
in quad records 8/10, offset 16 in tri records 9/11) [VERIFIED].

**UV order in the block is rotated vs. the vertex order** [VERIFIED 2026-10-02
via connected-vertex UV consistency (14/45 vs 44/45 mismatched verts on
CMFANR00) + visual proof: the girl's face renders perfectly]: stored =
`uv2 uv3 uv0 uv1` for quads, `uv2 uv0 uv1` for tris. Rationale: the game copies
the last two words `uv0+cba`, `uv1+tpage` verbatim into the GPU packet (that's
exactly the PS1 GT/FT packet word layout), the remaining uvs precede them.
Normal indices in the 8B tails of types 8/9 (u16 x2, per corner) are confirmed
per-corner indices into the part's normal pool (dot 0.997). Type 10/11 8B tails
[UNRESOLVED].

**Extra section (f3..f4)** — present when f3 < f4 [VERIFIED]:
- blocks with gouraud-textured types 8/9: `vertCount x 4B` per-vertex
  SECOND UV coordinates: 2 x s16 fixed point (value/16 = texel), indexed by
  the record-tail nidx (u16 x2). [VERIFIED 2026-10-02: values/16 cover 0..255;
  mirror-symmetric verts get mirror-identical UVs; rendering the face with
  this pool shows the symmetric hair/brows/lashes detail layer on top of the
  skin — the character face is two-layer textured: skin via uvblk, dark hair
  details via the tail pool] The lab has a "uv layer: main/second" switch.
  For complex blocks the analogous pools are per-vertex normals (verified,
  dot 0.997) — the tail roles serve different pool semantics per family.
- blocks with only flat textured types 10/11: `primCount x 4B` per-prim data
  (mostly zeros; sparse 2 x s16 values ~4096-scale - likely face normals).
  [PARTIALLY RESOLVED]

Quads triangulate as `(v0,v1,v2)+(v1,v2,v3)` — GPU packet order; the polygon
boundary is the zigzag `v0-v1-v3-v2`. [VERIFIED: convexity + 3D<->UV
edge-ratio consistency + closed-manifold winding on EMBTNK00]

### Complex blocks (kind_lo = N > 1 parts)

Part table of N × 0x24 entries right after the kind word. **All offsets in an
entry are self-relative to that entry's own address** — the absolute offset
(from block+0x10) is `stored + 0x24 * part_index`. [VERIFIED on EMBTNK00:
every fixed-up offset lands exactly on its section boundary]

```
+0x00 u32 prim   per-part face-chunk stream (same chunk/record layout as
                 simple blocks: u16 count + u16 type chunks, (0,0) terminator,
                 4-byte aligned; consecutive streams pack back to back)
+0x04 u32 xy     -> count x (s16 x, s16 y) vertex pairs  [VERIFIED]
+0x08 u32 z      -> count x s16 vertex depths            [VERIFIED]
+0x0C u32 nrm    -> 4-byte normal slots: one per PRIM for flat types
                    (10/11), one per VERTEX for gouraud types (8/9)
+0x10 u32 f4     -> u16 slots, same multiplicity as nrm
+0x14 u32 count  vertex count of this part
+0x18 u32 f6     offset into the prim region (runtime packet link?) [UNRESOLVED]
+0x1C 4xs16      bounding sphere (cx, cy, cz, r)                    [VERIFIED]
```

Vertices are part-local: `v[j] = (xy[j].x, xy[j].y, z[j])`, and record vertex
indices (stored ×2) are local to the part (max index = count-1 for every part
of EMBTNK00). [VERIFIED]

**Axis mapping differs between block families** [VERIFIED]:
- simple blocks (levels SMP*, props GM*, simple units): `v = (x, y, z)` with
  `(x, z)` in the pair and `y` in the pool — poolC = height
- complex blocks (EMBTNK, EM4TNK, CMHANR, ...): `v = (x, pair1, poolC)` —
  the pair holds `(x, y)` and poolC is `z`. Proven by exact stored-normal
  matches on EMBTNK00 (see below) and by proportions (the boss-tank hover
  skirt part4/5 is a flat horizontal ring: 960x819 wide, only 218 tall;
  EM4TNK00 renders as a flat textured tank)

**Normals** (complex blocks): the full per-slot normal is a 3D unit vector in
12-bit fixed point (length 4096) split across the `nrm` and `f4` slots:
`n = (nx = nrm.s16lo, ny = nrm.s16hi, nz = f4.s16)` in the complex-block world
frame (x, y=pair1, z=poolC). [VERIFIED exactly on EMBTNK00 prims 0-1; every
slot is unit-length 4096 +/- 1]. The bbox field is likewise `(cx, cy, cz, r)`
with cy = pair1-pool center, cz = poolC center. The per-slot ordering vs. prim
order for flat types 10/11 and their 8B tails remain [UNRESOLVED] (flat faces
use geometric normals in the viewer). Gouraud types 8/9 use the decoded
stored normals per corner (smooth shading).

**Quad corner order**: records store corners in GPU packet order — the GPU
draws quads as `(v0,v1,v2)+(v1,v2,v3)`, i.e. the polygon boundary is the
zigzag `v0-v1-v3-v2`. [VERIFIED: it is the only convex boundary on all 93
EMBTNK00 quads, and the 3D<->UV edge-ratio variance is minimal for the
identity mapping; winding is consistent across the whole closed mesh]

**Face color**: for the textured types the single modulation color lives in
the record's `rgb+cmd` trailer word (offset 20 for quad types 8/10, offset 16
for tri types 9/11), usually `80 80 80` = neutral. [VERIFIED — reading it
from the wrong offset produced purple/red-modulated renders]

Complex part symmetry check (EMBTNK00): every part is mirror-symmetric across
the z (pool) axis where expected — hull part0 28/28 verts paired, panel part6
24/24, wedge part3 10/10, turret+gun part2 16/24 (the gun barrel along +x
legitimately breaks symmetry); parts 4/5 are the left/right hover pods
(mirror images of each other). EMBTNK01 is the damaged variant: same hull,
parts 4-6 empty (count=0). [VERIFIED]

Complex models are multi-pose / multi-piece assemblies stored UNASSEMBLED:
parts overlap around the origin in model space (e.g. EMBTNK00 = hull +
turret+gun + two mirrored hover-skirt rings + hatches). No assembly transform
exists in the block — the table ends exactly at the last pool byte; the parts
are posed at runtime by the overlay code / animation data. [VERIFIED: part
bbox centers equal the raw geometry centers] The viewer exposes a per-part
selector.

## Axis conventions (world mapping of the vertex pools)

The vertex pools give two s16 per vertex in the pair array + one s16 in the
single pool. Naming: `a` = pair 1st half, `b` = pair 2nd half, `c` = pool.

There is NO single global mapping — asset classes were authored in different
orientations (and skeletal parts are posed at runtime):
- levels (SMP*): world = (x=a, y=c, z=b) — c = height (26 distinct floor
  heights with 0 = ground, VERIFIED)
- complex models (EMBTNK, EM4TNK, CMHANR, ...): world = (x=a, y=b, z=c) —
  b = height (hull flat, hover-skirt rings flat; VERIFIED vs screenshots)
- character parts (CMFANR00 girl's face): world = (x=c, y=-b, z=a) — the face
  is mirror-symmetric across c (lateral), -b = up (eye-band faces at b=-46,
  mouth at b=-13; TIM7 = the left half-face texture with the eye on top),
  a = nose-forward. [VERIFIED by texture-region correlation]
Stored normals are always in POOL order (a, b, c) and must be transformed with
the same mapping as the vertices. For complex blocks the 12-bit normal slot is
(nrm.s16lo = a, nrm.s16hi = b, f4.s16 = c). [VERIFIED: dot 0.997]

The viewer defaults: simple blocks "a,c,b", complex "a,b,c".

### Model lab

`sb-viewer --lab [part3 file] [block]` (or File > Open model lab) opens an
interactive format workbench: live axis mapping (source + sign per axis),
quad/tri corner orders, UV swap/mirror, index shift, stored-normals toggle,
textures/wireframe/normals/baked-colors render switches, plus:

- a hex view of the block with offset rulers and region highlighting;
- a UV-space view (the texture page with the faces' UV polygons overlaid,
  click selects a face);
- a record inspector: record N -> decoded field table + its bytes highlighted
  in the hex view;
- a struct-style record-format editor per record type: fields are
  `role:type` with B/b/H/h/I/i (u8/s8/u16/s16/u32/s32, '>' for big-endian).
  Roles: v0..v3 (vertex indices, stored x2), uK/wK (uv), cba, tp, rgb (flat
  color word), c0..c3 (per-corner colors), n0..n3 (u16 normal-pool indices),
  nKa/nKb/nKc (inline s8 normal triplets), x (padding). Default specs are the
  verified layouts from this document; custom specs reparse live.

## Verification renders

`experiments/render/`: `SMP01B01.png/.obj` (scene shell, 1148 verts / 1683
tris — coherent level geometry), `GMTANK01.png` (tank hull + barrel),
`X1LJUT00.png/.obj` (unit), `CMFANR00.png/.obj` (textured fan blade,
types 8/9/10), `EMBTNK00_complex.png` / `CMHANR00_complex.png` /
`C1LJUT00_complex.png` (complex multi-part models — tank/unit hulls).
Types 4/5/6 render as coherent geometry (GM4TNK01 walker, SMP02B01 level,
GMGARE40 rock dome) — record sizes verified; exact color sub-layouts remain
approximate (flat-shaded approximation in the viewer).

Headless viewer renders (`tools/sb-viewer --render <part3> <BLOCK> out.png`,
`SB_PART=n` selects a single part of a complex block, `SB_NOTEX=1` disables
textures, `SB_WIRE=1` overlays a wireframe, `SB_ROTX=deg` pre-pitches the
model around X): EMBTNK00 parts render as clean closed solids — part 0 = hull,
part 2 = turret + gun barrel, parts 4/5 = the two mirrored triangular tread
rings, matching the in-game boss tank.
