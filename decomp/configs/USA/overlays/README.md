# DATA.BIN package overlay configs (splat)

One splat config per DATA.BIN package **part2** — the only package part that
carries MIPS code. `part1` is an SB-RLE-compressed bundle of TIM textures,
`part3` is QMD model/scene data (see `research/data_bin.md`).

- `codemap.json` — measured per-package code map (written by the extractor).
- `overlay.template.yaml` — template every config is rendered from.
- `overlays.toml` — human-readable manifest (same data as codemap.json).
- `<name>.yaml` — the 35 generated splat configs.

Everything here is **generated**. Do not edit by hand; regenerate with:

```sh
decomp/tools/extract_packages.sh   # needs DATA.BIN + SLUS_009.02 (see below)
```

The script extracts packages into `decomp/assets/USA/packages/` (gitignored),
writes `codemap.json`, then re-renders every yaml + `overlays.toml`.
Game-file paths default to `../silent_bomber/{DATA.BIN,SLUS_009.02}`;
override with `SB_DATA_BIN` / `SB_SLUS`, python with `SB_PYTHON`
(needs `splat64`, `spimdisasm`, `rabbitizer`).

Split one overlay (from `decomp/`):

```sh
python -m splat split configs/USA/overlays/p01.yaml
```

## part2 anatomy (9-word descriptor, loaded by `func_80012E10`)

```
disc:  [w3 bytes][w4 bytes][w5 bytes]   one raw CD read of w3+w4+w5 to vram
RAM:   [w3: header/strings + .text + rodata]
       [w4: registration records, processed by func_80012D34/func_80019224]
       [w5: scratch — immediately overwritten by the part3 read]
part3 loads at vram+w3+w4 (w7+w8 bytes), so the final runtime image is
[w3 region][w4 region][part3]; the w5 disc bytes never survive
(A00–A03 excepted: their part3 is smaller than w5, so a live tail of
w5 remains past part3 — content not yet identified, open question).
```

The `.text` span always lies inside the w3 region. Everything between
`text_end` and w3 is rodata (jump tables live here — reached via function
pointers, not `jr`-indexed tables, so spimdisasm emits no `jlabel`s), and
the regions past w3 are the registration records and the scratch zone.

## Load addresses — VERIFIED

| package(s) | vram | evidence |
|---|---|---|
| P00–P27, A00–A03, DEMOP, ENDINGP | `0x800DD4F0` | `D_800B5BB8` (part2 destination of `func_80012E10`) is written once at boot (`0x8001015C`) from `func_8001243C` = constant `0x800DD4F0`; the arena top is `0x801FF000` (`0x80010154`). Cross-checked per overlay: `j`/`jal` targets computed against this base land inside the measured .text span (0 outliers for 24 packages, ≤6 for the rest — those were extent tails, fixed by extending to the max jump target) and `lui/ori` pointer clusters match the package's own vram range. |
| ARENAP | `0x801A0000` | Dedicated loader thread (`func_8001312C` via `0x80017740`): constant `0x801A0000` stored at thread+0x50 (`0x80017790`). Cross-checked: `j`/`jal` targets cluster in `0x801B5300..0x801CDE00` = base+code span. |

DEMOP/ENDINGP threads (`0x80016830` / `0x800176D8`) use the generic loader
(entry `func_80012E10`) with no base override, hence `0x800DD4F0` —
confirmed by their `j`-target analysis.

## Per-package code map

`file@` = part2 offset in DATA.BIN. `.text` offsets are relative to the
part2 file (= vram − load base). `tims` = SB-RLE blocks in part1 (one TIM
each). P00 is data-only (292 bytes of header records, no code).

| pkg | slot | file@ | part2 size | .text off | .text end | .text size | tims | kind |
|---|---|---|---|---|---|---|---|---|
| p00 | 0 | 0x01A1000 | 0x124 | — | — | — | 1 | data-only |
| p01 | 1 | 0x01BF000 | 0x4CFE4 | 0x6C34 | 0x1ACC0 | 0x1408C | 152 | code |
| p02 | 2 | 0x02AD800 | 0x37424 | 0x32B0 | 0x139E0 | 0x10730 | 81 | code |
| p03 | 3 | 0x034F800 | 0x423BC | 0x9810 | 0x1C240 | 0x12A30 | 99 | code |
| p04 | 4 | 0x0408000 | 0x45228 | 0x6A3C | 0x1E800 | 0x17DC4 | 64 | code |
| p05 | 5 | 0x04C4000 | 0x38EE8 | 0x96A4 | 0x1ACE0 | 0x1163C | 142 | code |
| p06 | 6 | 0x0568800 | 0x3D4D0 | 0x5214 | 0x18300 | 0x130EC | 58 | code |
| p07 | 7 | 0x0626800 | 0x3638C | 0x2DB8 | 0xF0A0 | 0xC2E8 | 150 | code |
| p08 | 8 | 0x06ED000 | 0x354E8 | 0x4500 | 0x12E60 | 0xE960 | 122 | code |
| p09 | 9 | 0x079A800 | 0x54B84 | 0x6EAC | 0x1FCF0 | 0x18E44 | 51 | code |
| p10 | 10 | 0x087C800 | 0x27060 | 0x21AC | 0x8470 | 0x62C4 | 99 | code |
| p11 | 11 | 0x08F4000 | 0x473F4 | 0x6A80 | 0x16090 | 0xF610 | 53 | code |
| p12 | 12 | 0x09CF000 | 0x4CD80 | 0x3A58 | 0x19480 | 0x15A28 | 72 | code |
| p13 | 13 | 0x0A6F800 | 0x335CC | 0x7434 | 0xF140 | 0x7D0C | 83 | code |
| p14 | 14 | 0x0B00000 | 0x31DCC | 0x23CC | 0xCD60 | 0xA994 | 56 | code |
| p15 | 15 | 0x0BBC800 | 0x49FB4 | 0x3F80 | 0x16FC0 | 0x13040 | 70 | code |
| p16 | 16 | 0x0C7A800 | 0x7B1C | 0x1830 | 0x41B0 | 0x2980 | 57 | code |
| p17 | 17 | 0x0CC7800 | 0x347C4 | 0x49D0 | 0x7F00 | 0x3530 | 109 | code |
| p18 | 18 | 0x0D60000 | 0x3917C | 0x4CC8 | 0x7FC0 | 0x32F8 | 49 | code |
| p19 | 19 | 0x0DEE800 | 0x495A0 | 0x4B9C | 0x7520 | 0x2984 | 24 | code |
| p20 | 20 | 0x0E84800 | 0x40B80 | 0x7790 | 0xAFD0 | 0x3840 | 50 | code |
| p21 | 21 | 0x0F77000 | 0x28F54 | 0x43E8 | 0x6490 | 0x20A8 | 33 | code |
| p22 | 22 | 0x101B000 | 0x35F34 | 0x3620 | 0x5BE0 | 0x25C0 | 66 | code |
| p23 | 23 | 0x10C2800 | 0x21DE4 | 0x3288 | 0x5E90 | 0x2C08 | 33 | code |
| p24 | 24 | 0x1174800 | 0x25C1C | 0x1AE8 | 0x66A0 | 0x4BB8 | 56 | code |
| p25 | 25 | 0x1200800 | 0x27FD4 | 0x2128 | 0x7A50 | 0x5928 | 22 | code |
| p26 | 26 | 0x125F000 | 0x36DDC | 0x6F78 | 0xA1F0 | 0x3278 | 30 | code |
| p27 | 27 | 0x1321000 | 0x2E1A0 | 0x680C | 0x9530 | 0x2D24 | 57 | code |
| a00 | 32 | 0x149F800 | 0x27D94 | 0x948 | 0x52F0 | 0x49A8 | 53 | code |
| a01 | 33 | 0x1504000 | 0x34200 | 0xD8 | 0x12C0 | 0x11E8 | 12 | code |
| a02 | 34 | 0x154B000 | 0x34468 | 0x2B8 | 0x15F0 | 0x1338 | 7 | code |
| a03 | 35 | 0x1599000 | 0x33FE4 | 0xD8 | 0x10C0 | 0xFE8 | 15 | code |
| arenap | -1 | 0x13A7800 | 0x4C4B4 | 0x14C48 | 0x4AB70 | 0x35F28 | 6 | code |
| demop | -1 | 0x009E800 | 0x2E538 | 0x28 | 0x1DE0 | 0x1DB8 | 27 | code |
| endingp | -1 | 0x00EB000 | 0x35A8 | 0xED0 | 0x30D0 | 0x2200 | 11 | code |
## Proof splits (splat 0.50 / spimdisasm 1.42.4)

| pkg | asm split | functions (glabel) | notes |
|---|---|---|---|
| p01 | 82 KB = 26.0% of part2 (73% of the live w3 region) | 192 | calls into SLUS (`func_80012478`, `func_8004EFA4`, …) and gp data (`D_800B5850`) resolve correctly |
| p02 | 67 KB = 29.8% of part2 (81% of w3) | 135 | dispatch via function pointers loaded from records — no `jlabel` tables, zero `.word` inside .text |
| a00 | 18 KB = 11.6% of part2 (68% of w3) | 32 | small arena-unit overlay |
| demop | 7 KB = 4.0% of part2 | 16 | code starts at 0x28, right after the count+strings header |

In all four, every .text byte disassembles into functions (no `.word`
islands), and .text starts exactly on a function prologue.

## Open questions

- The w4 registration records (`func_80012D34` → `func_80019224`) and the
  part2 header format (u32 count + records/strings) are not yet decoded —
  needed to name the overlay entry points.
- A00–A03: the live tail of the w5 region beyond part3 (up to 0x27C58
  bytes) is unexamined.
- P00 has no overlay code; where its mission logic lives is unknown
  (possibly it shares another package's code path).
- part3 (QMD) and part1 (TIM bundle) are not splat targets; they need
  their own asset tooling.
