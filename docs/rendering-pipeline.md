# QMD runtime rendering pipeline (from the SLUS disassembly)

The main exe converts QMD records into GPU packets at draw time, per poly.
Key functions (main exe, `asm/USA/main/20074.s`):

- `func_80076140` — model draw dispatcher: loads the block descriptor,
  sets up GTE (block fields +0x1C/+0x20 go into GTE data regs — a per-block
  vector), walks chunk streams, dispatches per record type.
- `func_80078F8C` / `func_80079480` / `func_80078ADC` / `func_80078F1C` —
  packet builders for textured quad/tri (flat + gouraud variants):
  - record vertex indices (u16, x2) index the pools: 4B/vertex (x,z) pool and
    2B/vertex (y) pool — vertex positions are fetched as raw file-space
    (x, z) pairs + y and loaded into the GTE via lwc2 (VXY0/VZ0 pattern);
  - uv words get cba/tpage RELOCATION deltas added ($t2/$t3) — textures are
    remapped in VRAM at load time;
  - the rgb+cmd trailer word goes into the packet;
  - **the 8B record tail is copied verbatim into the runtime packet**
    (e.g. record +0x14/+0x18 -> poly +0x20/+0x30 for tris): it is draw-time
    data consumed by the gouraud/lighting pass, not dead weight.
- `func_800794FC` / `func_80079B10` (with `nccs` GTE calls) — the subdivision
  + gouraud pass: big polys are split into 4 (midpoints averaged incl. the
  per-vertex u8 colors at vertex +0/+1), normals recomputed via GTE.

Consequences for the format:
- Type 0/1 records carry per-corner baked colors AND normals; the baked
  colors are authoring-time and get recomputed by the GTE at draw (that's why
  raw record colors look dark in a static viewer).
- Type 8/9 tails (4 x u16 per-corner indices into the block's f3..f4 pool)
  feed the lighting/second-uv pass. The pool entries are 2 x s16 per vertex.
- The runtime poly = a GT3/GT4-shaped packet with file-space 3D positions
  pending GTE transform + the extra tail words.
