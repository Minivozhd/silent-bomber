# DATA.BIN (Silent Bomber, SLUS_009.02 USA) — reverse engineering report

Status legend: **[VERIFIED]** = proven by disassembly + byte-level evidence; **[HYPOTHESIS]** = strong indication, not fully proven.

## 0. TL;DR

- There is **no file table inside DATA.BIN**. The whole package directory is **static data in
  SLUS_009.02 `.data`** (descriptor structs + pointer tables). [VERIFIED]
- DATA.BIN is a flat concatenation of **packages**. Every package starts with a **compressed
  stream** (custom nibble/byte **RLE**, not LZ), optionally followed by raw (uncompressed) parts.
  [VERIFIED]
- The mysterious "header" `0xACC / 0x2B30 / 0x4E20` is **not a header**: it is the first
  `skip`-word + the first block header (`csize=0x2B30`, `dsize=0x4E20`) of the boot TIM stream.
  [VERIFIED]
- The earlier belief "`gp+0x3AC/0x3B0/0x3B4` = the three header u32s" is **wrong**: the loader
  `func_800127B4` stores there the **CD LBAs of DATA.BIN / STR.BIN / XA.BIN** (ISO9660 directory
  lookups of the strings `"DATA.BIN"` @0x8009A1C4, `"STR.BIN"` @0x800B5858, `"XA.BIN"` @0x800B5860).
  [VERIFIED — disassembly of func_800127B4/func_800128E0]

## 1. CD access layer [VERIFIED]

- `func_800127B4` (boot init): resolves `DATA.BIN`/`STR.BIN`/`XA.BIN` via
  `func_80088690` (directory search) + `func_80088610` (record→LBA), stores LBAs at
  `gp+0x3AC/0x3B0/0x3B4` (= D_800B5BE4/D_800B5BE8/D_800B5BEC).
- `func_800128E0(file_off)`: `CdRead((file_off+0x7FF)>>11 + DATA_LBA, scratchpad 0x1F800000)`.
- `func_80012930(start,size,buf,buf_end)` / `func_800136A4`: ranged synchronous read with retry
  (uses func_8008B630/func_8008B7D0 CD wrappers, "data" error string @D_800B5868).

## 2. Compression: custom RLE ("SB-RLE") [VERIFIED]

Decompressor = `func_80012BB0` (one block), driver = `func_80012AEC` (stream).

### 2.1 Stream layout (driver func_80012AEC)

```
p = stream_start
loop:
    skip = u32(p)                 # if skip == 0 -> end of stream
    p += 4
    block = p
    csize = u32(block)            # compressed size INCLUDING the 10-byte block header
    dsize = u32(block+4)          # decompressed size (driver advances dst by this, see §2.4)
    esc   = u8 (block+8)          # escape value
    mode  = u8 (block+9)          # 0 = nibble RLE, !=0 = byte RLE
    data  = block[10 : csize]
    emit decompress_block(...) at dst
    dst += dsize
    p = block + skip*4            # skip*4 == align4(csize); block padded to 4
```

### 2.2 Block payload, nibble mode (mode == 0)

Nibbles are read MSB-first within each byte; the input pointer advances only after the low
nibble. Output is assembled high-nibble-first (the high nibble is stored immediately, the low
nibble is OR-ed in afterwards).

```
while in_pos < block_end:
    n = next_nibble()
    if n != esc: emit_nibble(n)              # literal
    else:
        if in_pos >= block_end: break        # lone trailing escape: decoder returns (§2.4)
        cnt = next_nibble(); val = next_nibble()
        emit_nibble(val) * cnt               # cnt == 0 -> emit nothing
```

### 2.3 Block payload, byte mode (mode != 0)

```
while in_pos < block_end:
    b = next_byte()
    if b != esc: emit(b)
    else:
        cnt = next_byte(); val = next_byte()
        emit(val) * cnt                      # cnt == 0 -> emit nothing
```

To emit a literal equal to `esc` the encoder uses a run with count 1 (`esc,1,esc`).

### 2.4 dsize is advisory (edge cases) [VERIFIED]

The driver trusts `dsize`, not the actual decoder output:
- P08 (file 0x6ED000) block #11: data ends with a lone low-nibble escape (`... 5a 5b`, esc=0xB);
  the decoder returns early → 358 bytes written, `dsize=360` (2-byte hole, stale RAM in-game).
- P01 block #27: 2113 bytes of nibble output vs `dsize=2112` (odd trailing nibble; the extra
  half-byte is overwritten by the next block).
Extractor policy: truncate to `dsize`, zero-pad shortfalls.

### 2.5 Evidence

- Boot stream @0x0000: `cc 0a 00 00 | 30 2b 00 00 | 20 4e 00 00 | 07 00 | ...`
  skip=0xACC, csize=0x2B30, dsize=0x4E20, esc=0x07, mode=0. Decodes to exactly 0x4E20 bytes =
  a TIM: flags=8 (4bpp+CLUT), CLUT→VRAM(608,240,16×16), image 64×152→(960,0), total 20000=dsize.
- Package @0x3000: skip=0x30A, csize=0xC26, dsize=0x1840 → `skip*4 = 0xC28 = align4(0xC26+…)` ✓
- Decompressed part1 of every package starts with TIM magic `10 00 00 00`.
- All 216 blocks of `boot_main` part1, all 152 of P01, all 81 of P02 decode to TIMs with
  matching dsizes.

## 3. Package table = static descriptors in SLUS_009.02 [VERIFIED]

All in `.data` (file offsets below are DATA.BIN byte offsets; vaddr→SLUS file = v−0x80010000+0x800).

### 3.1 9-word descriptor (stride 0x24) — main packages

Parsed by the package loader `func_80012E10` (thread struct +0x48 = descriptor ptr):

```
w0 = part1 file offset
w1 = part1 size          (SB-RLE stream; decompressed in place by func_80012AEC)
w2 = align800(w1)        (delta: part2 file offset = w0 + w2)
w3,w4,w5 = part2 sub-sizes (ONE raw read of w3+w4+w5 at w0+w2; [w3..w3+w4) gets a
             fixup/processing pass func_80012D34→func_80019224)
w6 = delta: part3 file offset = w0 + w2 + w6
w7,w8 = part3 sub-sizes  (ONE raw read of w7+w8)
package span = w2 + w6 + align800(w7+w8)   # consecutive descriptors chain exactly ✓
```

Pointer table **D_800AC9E8** (36 slots):
- slots 0..27 → descriptors D_800AC568..D_800AC934 = **P00..P27**
  (offsets 0x1A1000 .. 0x1321000). [VERIFIED by content: part2 of slot N contains
  `gm{N:02d}…` / part3 `SMP{N:02d}B…` strings]
- slots 28..31 = NULL
- slots 32..35 → D_800AC958..D_800AC9C4 = **A00..A03**
  (0x149F800 / 0x1504000 / 0x154B000 / 0x1599000; content `SMA00B01`, `gmA01bgl`, `gmA03bgl`) ✓

### 3.2 4-word descriptor — unit model packs

```
w0 = off, w1 = csize (SB-RLE stream), w2 = delta, w3 = raw tail size
```

Loaded by `func_800132D8`. Two tables of 12:
- **D_800AC448** → 12 packs @0x13F8800… ("grpA"): QMD names `X1…/C1…/E1…`
- **D_800AC538** → 12 packs @0x144C000… ("grpB"): same packs with `X2…/C2…/E2…` names
  → player-1 / player-2 variants of 12 units:
  LJUT, BTNK, PWSN, 4TNK, BTOR, PWSS, PWSB, BIOM, LJHO, PWSM, LBEN, NEKO. [VERIFIED by QMD names]

### 3.3 Standalone descriptors

| vaddr | off | identity | evidence |
|---|---|---|---|
| D_800AC2E8 | (0, 0x2B38) | boot TIM (system font/HUD) | loaded+decompressed by func_80013428 |
| D_800AC300 | 0x3000 | **boot_main** (7-word variant) | func_80013000: SB-RLE stream (0x3D0E4, 216 TIMs) + raw tail 0x5E71C |
| D_800AC31C | 0x9E800 | **DEMOP** | loader stores ptr D_8009A3D8 = `"DEMOP.BIN"` next to it (0x80016830) [VERIFIED]; content: `TUTORIAL`, `OPENING`, `:ACCEPT` |
| D_800AC340 | 0xEB000 | **ENDINGP** | paired with D_8009A3FC = `"ENDINGP.BIN"` (0x800176D8) [VERIFIED]; content: rank-reward strings |
| D_800AC364 | 0x13A7800 | **ARENAP** | paired with D_8009A408 = `"ARENAP.BIN"` (0x80017778) [VERIFIED]; content: `BATTLE SIMULATION` |
| D_800AC2F0 | (0x160800, 0x20968) | TIM bundle (46 blocks) | SB-RLE stream, decompressed on demand (func_80013588 single-block path) |
| D_800AC2F8 | (0x181800, 0x1F2E0) | TIM bundle (31 blocks, byte+nibble modes) | same |

### 3.4 Package name strings in exe

12-byte entries at SLUS file 0x901D4 (vaddr ~0x8008F9D4), stored **in reverse**:
`A03P.BIN, A02P, A01, A00, P27P…P00P`, then `"ERROR IN WORLD"`.
`DEMOP.BIN` @0x8009A3D8, `ENDINGP.BIN` @0x8009A3FC, `ARENAP.BIN` @0x8009A408 (with
`"POINT : "`/`"BLOCK : "` arena-score strings nearby).

## 4. DATA.BIN layout map [VERIFIED]

```
0x0000000  boot_tim      SB-RLE (1 block)            -> TIM 0x4E20
0x0002B38  FF pad to 0x3000
0x0003000  boot_main     SB-RLE stream 0x3D0E4 (216 TIM blocks) + raw tail 0x5E71C (sound bank? §6)
0x009E800  DEMOP         9-word: part1 TIMs, part2 MIPS+`TUTORIAL/OPENING`, part3 0x31C
0x00EB000  ENDINGP       9-word: part1 TIMs, part2 MIPS+rank strings, part3 0x4A188 TIM
0x013F000  (gap 0x21800, unreferenced by descriptors; mostly non-FF interleaved data — §6)
0x0160800  bundle        SB-RLE (46 TIM blocks)
0x0181800  bundle        SB-RLE (31 TIM blocks, modes 0+1)
0x01A0AE0  FF pad to 0x1A1000
0x01A1000  P00 … P27     28 × 9-word packages (contiguous chain to 0x13A7800)
0x13A7800  ARENAP        9-word
0x13F5800  (pad 0x3000)
0x13F8800  grpA 12×4-word (unit packs P1)
0x144C000  grpB 12×4-word (unit packs P2)
0x149F800  A00 … A03     4 × 9-word packages
0x15D8000  EOF (A03 ends exactly at EOF)
```

## 5. Package inventory highlights (full dump: `out/_inventory.txt`)

Every 9-word package has the same anatomy:
- **part1** = SB-RLE → bundle of TIM textures (one block = one TIM).
- **part2** = level/mission data with embedded **MIPS overlay code**
  (`27 BD xx FF` prologues, `08 00 E0 03` jr-ra) + ASCII object names.
- **part3** = **QMD** model/scene data (`QMD ` magic), names like `SMPxxB01`, `GMTANK10`.

| pkg | @file | span | part1→TIMs | notable strings (part2/part3) |
|---|---|---|---|---|
| P00 | 0x1A1000 | 0x1E000 | 1 | `QMD SMP00B02/B01` |
| P01 | 0x1BF000 | 0xEE800 | 152 | `gm01bara gmItem gm01smkg`, `SMP01B01` |
| P02 | 0x2AD800 | 0xA2000 | 81 | `GM02GEN1 EN02SFLT gm02flor`, `GMTANK10` |
| P04 | 0x408000 | 0xBC000 | TIMs | `Error in SFX.C`, `GM04PIER`, `GMRAIL10` |
| P08 | 0x6ED000 | 0xAD800 | TIMs | `en08Boss gmWarp`, `SMP08B02` |
| P16 | 0xC7A800 | 0x4D000 | TIMs | `Do you read me?!  Jutah! Jutah!`, `GMDORS10` |
| P18 | 0xD60000 | 0x8E800 | TIMs | `Planning to burn your friends, too?` |
| P19 | 0xDEE800 | 0x96000 | TIMs | `Annri!`, `Hey! Pull yourself together!` |
| P25 | 0x1200800 | 0x5E800 | TIMs | `unit's been wiped out!`, `Colonel!` |
| A00 | 0x149F800 | 0x64800 | TIMs | `NEKO PUNCH NEKO MISSILE NEKO TANK HOLOGRAM` |
| A01–A03 | 0x1504000… | … | TIMs | `gmA01bgl`, `gmA03bgl`, `GMLIGHxx` |
| DEMOP | 0x9E800 | 0x4C800 | 27 blocks | `TUTORIAL`, `OPENING`, `:ACCEPT` |
| ENDINGP | 0xEB000 | 0x54000 | 11 blocks | `All missions cleared at Rank S/A…` |
| ARENAP | 0x13A7800 | 0x51000 | TIMs | `BATTLE SIMULATION` |

Per-package extraction artifacts (decompressed part1 + raw part2/part3): `out/*.bin` (162 files).

## 6. Open questions

- **boot_main raw tail** (0x400E4, 0x5E71C bytes): no MIPS/TIM/strings; processed at boot by
  `func_80012D34` → `func_80019224` → `func_800964E0` chain storing handles into a 4-slot table
  at gp+0x450 (D_800B5C88). [HYPOTHESIS] SPU sound-bank / sample data uploaded at boot.
- **Gap 0x13F000–0x160800** (0x21800): starts `05 00 00 00` + 0xFF-heavy interleaved data;
  no descriptor found in the exe. [HYPOTHESIS] streamed sample/audio area belonging to the
  SFX system (P04 references `SFX.C`).
- **QMD** model format, **part2** mission/overlay format, and the `func_80012D34` fixup pass
  semantics — next RE targets (needed for the randomizer).
- Exact role of bundle_160800/bundle_181800 TIM sets (referenced from 4 sites around
  0x8002BC28) — likely shared/effect textures.

## 7. Files in this experiment

- `sb_rle.py` — reference SB-RLE decoder (block + stream), validated against all packages.
- `extract_data_bin.py` — full extractor: parses the descriptor tables from SLUS_009.02,
  decompresses/extracts everything into `out/` with classification (`out/_inventory.txt`).
- `probe_header.py` — initial header/nibble-statistics probe.
