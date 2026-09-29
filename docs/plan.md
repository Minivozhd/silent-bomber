# Silent Bomber (USA) — RE project: decompilation, mod tools, randomizer

Reference project: [GabeRealB/parasite-eve-2-decomp](https://github.com/GabeRealB/parasite-eve-2-decomp)
(kept locally at `tools/pe2-mod-tools/file-manager/experiments/decomp`, used as the
structure/toolchain template).

## 1. Image inventory (extracted and identified)

Source image: `Silent Bomber (USA).bin/.cue` (MODE2/2352, 145,802 sectors). Converted to
`silent_bomber.iso` (2048-byte sectors; the converter moves to `tools/`).

ISO9660 contents (all extracted):

| File | Size | What it is |
|---|---|---|
| `SYSTEM.CNF` | 60 B | Boot config: `BOOT=cdrom:\SLUS_009.02;1`, `STACK=801FFF00` |
| `SLUS_009.02` | 843,776 B | **Main executable** (PS-X EXE) |
| `DATA.BIN` | 22,904,832 B | Game package archive (~43 packages, see §2) |
| `XA.BIN` | 131,072,000 B | XA audio (interleaved ADPCM streams; playable by standard players) |
| `STR.BIN` | 143,425,536 B | STR video (MDEC movies; frame sync `60 01 01 80`) |

**`SLUS_009.02`** (the only executable on disc; overlays live inside DATA.BIN):

- PS-X EXE: text @ `0x80010000`, size `0xCD800` (841,728 B), entry `0x8008D620`,
  stack `0x801FFFF0`, region "for North America".
- First splat split done: 67% (565 KB) recognized as code
  (`decomp/slus_009.02.yaml`, auto-detected `gp_value = 0x800B5838`).
- Compiler: almost certainly GCC 2.7.2/2.8.1 + PsyQ ~4.x (1999, CyberConnect2) —
  exact version to be determined by codegen fingerprinting (maspsx trials, as PE2: GCC 2.8.1).

## 2. Packages inside DATA.BIN (decompilation overlay targets)

The exe holds a package name table (12 bytes per name, descending order):
`P00P.BIN`…`P27P.BIN` (36), `A00P`…`A03P.BIN` (4), `ARENAP.BIN`, `DEMOP.BIN`,
`ENDINGP.BIN` (plus `DATA.BIN`/`XA.BIN`/`STR.BIN` references). The names sit next to
a table of pointers to runtime descriptor structs.

- Purpose (preliminary): `Pxx` = world/mission packages (P00–P27 ≈ 28+ levels etc.),
  `A0x` = arenas/bosses, `DEMOP` = attract demo, `ENDINGP` = credits, `ARENAP` = arena.
- Loader: function @ `0x800127B4` opens `DATA.BIN` (string `0x8009A1C4`),
  reads the first values (gp+0x3AC/0x3B0/0x3B4).
- DATA.BIN header: first 3 u32 = `0xACC, 0x2B30, 0x4E20`.
  Hypothesis #1: 0xACC = table entry count (2732); a 4-byte-entry table at offset 0x80
  would end exactly at 0x2B30 (verified arithmetically: 0x80+2732*4 = 0x2B30),
  but entries do not look like u32 offsets → the table is most likely
  **bit/nibble-packed** (the 0x0C..0xACC region is dominated by nibbles 0/1/4/7 —
  delta-encoded offsets). 0x4E20 = start of the (compressed) package data region.
- Packages are almost certainly compressed (custom CyberConnect2 LZ scheme).
  Table + decompressor analysis is a dedicated experiment track in the PE2 style
  (Python harnesses + render/dump verification).

## 3. Roadmap

### Phase 1. Decompilation (PE2 style)

1. **Repository** `decomp/` (created): `assets/USA/SLUS_009.02`, `slus_009.02.yaml`
   (working first split), `asm/`, `src/`, `linkers/USA/`, `tools/`.
   Next: migrate the config to the PE2 shape (`configs/USA/main.yaml`, sym/rel files,
   hasm pattern, Makefile, toolchain Dockerfile — taken from the PE2 repo).
2. **Toolchain**: Docker (ubuntu + binutils-mipsel + gcc cross, per the PE2 Dockerfile),
   splat 0.50 (venv ready), asm-differ, m2c, maspsx — copied/adapted from
   `experiments/decomp/tools/`.
3. **Compiler**: codegen fingerprint matching (maspsx trials over 3–5 functions).
   Candidates: GCC 2.6.3 / 2.7.2 / 2.8.1 + PsyQ 3.6–4.5.
4. **Workflow**: split → mark TU boundaries in `subsegments` → m2c scaffolding →
   function matching → symbols (`configs/USA/sym.main.txt`).
5. **Overlays**: once the DATA.BIN table and decompressor are cracked — split every
   code-bearing package (P00–P27, A00–A03, ARENA/DEMO/ENDING) as overlay configs
   (template `configs/USA/overlay.template.yaml` from PE2).

### Phase 2. Mod tools (extract/reinsert)

1. **ISO level**: bin↔iso converter (done, moving to `tools/`), image patcher with
   ECC/EDC recalculation (based on `tools/pe2-mod-tools/file-manager/rom_patcher.py` —
   it already does PSX ECC/EDC) + an ISO9660 volume generator for whole-file replacement.
2. **DATA.BIN level**: crack the table (nibble packing?) and the package decompressor →
   extractor with names from the exe + repacker (preserve offsets/alignment, rebuild
   the table). Critical for the randomizer: package replacement must not break sector
   offsets, or the table must be fully recomputed.
3. **Verification**: emulator runs (DuckStation/PCSX) on key packages.

### Phase 3. Randomizer (Randomized Silent Bomber)

Targets (refined as formats are cracked): weapon/chip drop tables, upgrade module
placement on maps, enemy/boss stats, (optional) arena wave composition.

1. Decode the internal package formats (after Phase 1.5): missions, drops, stats.
2. Randomization logic + seed/log (as in Randomized Eve II — see `pe2randomizer/`).
3. Injection via Phase 2 (DATA.BIN repack + ISO patch).

## 4. Open questions (by priority)

1. DATA.BIN table format (nibble-packed offsets?) + package decompression algorithm.
2. Exact compiler/PsyQ version (fingerprint).
3. Contents of packages P00–P27: code/data/maps/models (after decompression).
4. Randomizable content structures (drops/upgrades/stats).

## 5. Local artifacts

- `silent_bomber.iso` — converted image (working copy).
- `decomp/` — decomp project skeleton (first split done: 67% code).
- `venv/` — python3.13 + splat 0.50, spimdisasm, rabbitizer (disasm harnesses).
