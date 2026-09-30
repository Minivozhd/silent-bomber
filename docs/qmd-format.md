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
| 8 | quad, textured gouraud (GT4, 0x3C) | 42B | idx, normals, colors, UVs, cba, tpage |
| 10 | quad, textured flat (FT4, 0x2C) | 36B | idx, normals, color, UVs, cba, tpage |

Sizes VERIFIED by chunk-landing analysis (records must land on the next chunk
header / terminator / section end). The textured types embed literal PS1 GPU
`cba`/`tpage` words — VERIFIED against the part1 TIM rects; UV layouts and the
per-record field order for 8/10 are [HYPOTHESIS] pending textured renders.

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

The prim stream is global/contiguous: `[u32 rgb+cmd][payload]` records;
cmd 0x3C = 4 (v,n) u16 pairs + uv0,uv1,uv2,cba,uv3,tpage; cmd 0x34 = 3 or 6
pairs + uv0,uv1,uv2,cba,pad,tpage. cba/tpage are literal PS1 GPU words
[VERIFIED]. The exact part↔vertex-pool mapping is [UNRESOLVED] (complex_render
in qmd_parse.py is best-effort).

## Verification renders

`experiments/render/`: `SMP01B01.png/.obj` (scene shell, 1148 verts / 1683
tris — coherent level geometry), `GMTANK01.png` (tank hull + barrel),
`X1LJUT00.png/.obj` (unit).
