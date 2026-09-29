# Image analysis — Silent Bomber (USA)

Date of analysis: 2026-09-29. Tools: custom Python harnesses
(`spimdisasm`, `rabbitizer`).

## Disc

- Source: `Silent Bomber (USA).bin/.cue` — MODE2/2352, 145 802 sectors.
- Converted to plain ISO9660 (2048-byte sectors) by stripping the 24-byte
  sector headers/subheaders.

### ISO9660 contents

| File | Size | Purpose |
|---|---|---|
| `SYSTEM.CNF` | 60 B | Boot config: `BOOT=cdrom:\SLUS_009.02;1`, `STACK=801FFF00` |
| `SLUS_009.02` | 843 776 B | **Main executable** (PS-X EXE) |
| `DATA.BIN` | 22 904 832 B | Game package archive (~43 packages) |
| `XA.BIN` | 131 072 000 B | XA interleaved ADPCM audio streams |
| `STR.BIN` | 143 425 536 B | STR (MDEC) full-motion video |

## Main executable `SLUS_009.02`

PS-X EXE header:

| Field | Value |
|---|---|
| text address | `0x80010000`, size `0xCD800` (841 728 B) |
| entry point (pc0) | `0x8008D620` |
| initial SP | `0x801FFFF0` |
| region string | `Sony Computer Entertainment Inc. for Nor(th America)` |
| GP (splat auto-detect) | `0x800B5838` |

First splat split (splat 0.50): **67 % recognized as code** (565 KB), rest data.

Compiler: not yet fingerprinted; period candidates GCC 2.6.3 / 2.7.2 / 2.8.1 +
PsyQ 3.6–4.5 (game is 1999–2000, CyberConnect2's first title).

## `DATA.BIN` package archive

### Package names

The executable holds a name table (12 bytes per name, descending order):
`P00P.BIN`…`P27P.BIN` (36), `A00P`…`A03P.BIN` (4), `ARENAP.BIN`, `DEMOP.BIN`,
`ENDINGP.BIN`. Names are followed by a table of pointers to runtime
(zero-initialized) descriptor structs — so the static index→offset mapping is
NOT in the exe; it lives in `DATA.BIN` itself.

### Header

The first three u32 are read by the loader (`0x800127B4`, opens the
`"DATA.BIN"` string at `0x8009A1C4`) and stored at `gp+0x3AC/0x3B0/0x3B4`:

```
0x00000ACC   0x00002B30   0x00004E20
```

Working hypotheses (open):

- `0xACC` (2732) could be a table entry count; a 4-byte-entry table starting at
  offset `0x80` would then end exactly at `0x2B30` — but the bytes there are not
  plain u32 offsets, so the table is likely **bit/nibble-packed** (the region
  `0x0C..0xACC` is dominated by nibbles 0/1/4/7 — consistent with
  delta-encoded offsets).
- `0x4E20` marks the start of the (compressed) package data region.
- Packages are very likely compressed with a custom CyberConnect2 LZ variant.

Next steps: decode the table bitstream, identify the decompression algorithm,
extract a package, verify (e.g. by finding code/TIM/model data inside), then
split code-bearing packages as decomp overlays.

## `XA.BIN` / `STR.BIN`

- `XA.BIN`: XA interleaved ADPCM audio (standard players can decode).
- `STR.BIN`: MDEC video; frames start with the `60 01 01 80` sync word.
