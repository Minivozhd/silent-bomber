# QMD model/scene format (Silent Bomber, part3 of DATA.BIN packages)

Status legend: **[VERIFIED]** / **[HYPOTHESIS]** / **[UNRESOLVED]**.
Reference parser: `experiments/qmd_parse.py` (walks, renders, exports OBJ).

## Container

- `part3` of a package starts with magic `pQES` + header, then a chain of
  QMD blocks. [VERIFIED]
- Unit packs (`grpA_XX.part2_raw.bin`) hold a bare QMD chain (no pQES). [VERIFIED]

## QMD block

```
+0x00  char[4]  "QMD "
+0x04  char[8]  name (SMP01B01 scene piece / GMTANK01 unit / EMBTNK00 boss…)
+0x0C  u32 kind: low u16 = 1 → simple block; N > 1 → complex (N parts)
+0x10  (simple) u32 f0  offset of section A (face chunks)   rel. to +0x10
+0x14         u32 f1  offset of section B (vertices: s16 x, s16 z per vertex)
+0x18         u32 f2  offset of section C (s16 y per vertex)
+0x1C         u32 f3  offset of block end (next block = block + f3 + 0x10)
+0x20         u32 f4  == f3 (duplicated)
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
| 4 | quad, gouraud, no normals | 24B | 4×u16 idx, 4×4B colors |
| 5 | tri, gouraud, no normals | 24B | 3×u16 idx, 3×4B colors |
| 6 | two-quad strip | 28B | 2×(4×u16 idx), 1×4B color |
| 8 | quad, textured gouraud (GT4, cmd 0x3C) | 32B | 4×u16 idx, uv0,uv1,uv2,cba,uv3,tpage (12B), rgb+cmd, 4×u16 nidx×2 |
| 9 | tri, textured gouraud (GT3, cmd 0x34) | 28B | 3×u16 idx, uv0,uv1,cba,uv2,tpage (10B), rgb+cmd, 3×u16 nidx×2 + pad |
| 10 | quad, textured flat (FT4, cmd 0x2C) | 32B | 4×u16 idx, uv0,uv1,uv2,cba,uv3,tpage (12B), rgb+cmd, 8B tail |
| 11 | tri, textured flat (FT3, cmd 0x24) | 28B | 3×u16 idx, uv0,uv1,cba,uv2,tpage (10B), rgb+cmd, 8B tail |

Sizes VERIFIED by chunk-landing analysis (records must land on the next chunk
header / terminator / section end) across P01, A00, P02 and unit packs — zero
mismatches. Types 8–11 embed literal PS1 GPU `cba`/`tpage` words — VERIFIED
against part1 TIM destination rects, e.g. GMDORA10 → TIM11 (cba 0x6017 = clut
(368,384), tpage 0x001D = image (832,320)), GMOCAR10 → TIM20/21 (cba 0x5014 =
clut (320,320), tpage 0x001C), grpA_00 unit → cba 0x3E37 = clut (880,248),
tpage 0x000D = image (832,0). UV order is `[uv0][uv1][uv2][cba][uv3][tpage]`
for quads and `[uv0][uv1][cba][uv2][tpage]` for tris [VERIFIED by TIM rect
match]. Normal indices in tails of types 8/9 point into section D [HYPOTHESIS]:
textured blocks carry an extra per-vertex array at f3..f4 (CMFANR00: 45×4B =
s8 nx,ny,nz + pad). Type 10/11 8B tails [UNRESOLVED] (first u16 looks like
2×record ordinal).

Quads triangulate as `(v0,v1,v2)+(v1,v2,v3)`. [VERIFIED on X1LJUT00]

### Complex blocks (kind_lo = N > 1 parts)

Part table of N × 0x24 entries right after the kind word:

```
+0x00 u32 f0  prim-stream payload offset (rel. block+0x10)
+0x04 u32 f1  slice offset → s16 (x,z) pool
+0x08 u32 f2  slice offset → s16 y pool
+0x0C u32 f3  slice offset → normals pool (s16, ~4096 scale)
+0x10 u32 f4  slice offset → second ~4096-scale pool
+0x14 u32 count
+0x18 u32 f6
+0x1C 4×s16  bounding sphere (cx,cy,cz,r) in part-local space
```

The prim stream is a global contiguous sequence of `[u32 rgb+cmd][payload]`
records, starting right after the part table and ending at the lowest f1
target; part f0 = offset of the part's first payload (its rgb+cmd word sits
4 bytes earlier, i.e. in the previous part's span; part0's first payload is
cmd-less). Sections are separated by ~8B gaps. Record payloads
[VERIFIED on EMBTNK00 / C1LJUT00 / CMHANR00]:

| cmd | GPU sense | payload |
|---|---|---|
| 0x3C | GT4 quad | 4×(u16 v, u16 n) + uv0,uv1,uv2,cba,uv3,tpage (12B) |
| 0x34 | GT3 tri | 3×(u16 v, u16 n) + uv0,uv1,uv2,cba,uv3,tpage (12B) |
| 0x2C | flat-colored N-gon | u16 v0, u16 n0, u32 color, (N-1)×(u16 v, u16 n), uvblk 12B |
| 0x24 | flat-colored tri | u16 v0, u16 n0, u32 color, 2×(u16 v, u16 n), uvblk 10B |

Vertex indices are raw u16 (no ×2) into **global** pools: f1 slice → s16
(x,z)-pair pool, f2 slice → s16 y pool; vertex = (pair.x, ypool[v], pair.z).
[VERIFIED: EMBTNK00 part3 renders as a coherent armor plate; CMHANR00 and
EMBTNK00 render as tank hulls.] The n fields of (v,n) pairs index the f3/f4
~4096-scale pools (normals) [HYPOTHESIS]; occasional n0 values like 0x5C15
remain [UNRESOLVED]. Meaning of per-part `count`, `f6`, and the exact role of
the f3/f4 pools and the file-tail blob (skeleton/joint data — contains
X-block-like vertex values) [UNRESOLVED].

## Verification renders

`experiments/render/`: `SMP01B01.png/.obj` (scene shell, 1148 verts / 1683
tris — coherent level geometry), `GMTANK01.png` (tank hull + barrel),
`X1LJUT00.png/.obj` (unit), `CMFANR00.png/.obj` (textured fan blade,
types 8/9/10), `EMBTNK00_complex.png` / `CMHANR00_complex.png` /
`C1LJUT00_complex.png` (complex multi-part models — tank/unit hulls).
