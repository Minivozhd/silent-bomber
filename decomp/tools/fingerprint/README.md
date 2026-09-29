# Compiler fingerprint for SLUS_009.02

Goal: identify the exact compiler + flags the game was built with, by
compiling minimal C equivalents of hand-picked functions from the split asm
with candidate period toolchains and scoring the generated instructions
against the retail binary.

## Method

`trial.py`:

1. Nine target functions were picked from `asm/800.s` (`candidates/*.c`,
   one file each, with the target disassembly quoted in the header comment):
   - `f1` gp-relative u16 counter increment (`%gp_rel` small-data access)
   - `f2` mixed sh/lw/sw struct stores, store scheduling
   - `f3` zero-fill, store sinking into the `jr` delay slot
   - `f4` table lookup (`lui`/`addiu` absolute addressing, `sll` index)
   - `f5` `0x55555555` constant synthesis (`lui`/`ori`)
   - `f6` function-pointer store
   - `f7` u16 `mult` + clamp (branch around a store)
   - `f8` `mult`+`sra`, **store in a branch delay slot** (scheduler probe)
   - `f9` variable `div` + magic-multiply constant division (0x66666667)
2. Each C file is preprocessed (`clang -E -P` — the old `cc1` binaries
   segfault on comments in their input), compiled with each candidate
   `cc1` (gcc 2.8.1-psx / 2.7.2-cdk natively on macOS arm64, gcc 2.6.3-psx
   inside the amd64 docker container), then run through `maspsx` +
   `mipsel-linux-gnu-as`, exactly like the PE2 build pipeline.
3. The resulting objects are disassembled and compared instruction by
   instruction against the retail disassembly, after normalizing
   disassembler aliases (`li` vs `addiu`, label names, relocation targets).
   `exact` = positionally identical instructions; `sim` = sequence similarity.

Run it with:

```sh
python3 tools/fingerprint/trial.py [--verbose]   # ~1-2 min
```

## Results

| compiler | flags | exact | seq-sim |
|---|---|---|---|
| **gcc-2.8.1-psx** | **-O2 -G8** | **90/90** | **100.0%** |
| **gcc-2.8.1-psx** | **-O3 -G8** | **90/90** | **100.0%** |
| gcc-2.7.2-cdk | -O2 -G8 | 88/90 | 98.6% |
| gcc-2.7.2-cdk | -O3 -G8 | 88/90 | 98.6% |
| gcc-2.8.1-psx | -O2 -G0 | 71/90 | 93.3% |
| gcc-2.7.2-cdk | -O2 -G0 | 69/90 | 91.9% |
| gcc-2.8.1-psx | -O1 -G8 | 49/90 | 85.9% |
| gcc-2.7.2-cdk | -O1 -G8 | 42/90 | 84.0% |
| gcc-2.6.3-psx | -O2 -G8 | 52/90 | 80.2% |
| gcc-2.8.1-psx | -O1 -G0 | 30/90 | 79.2% |
| gcc-2.7.2-cdk | -O1 -G0 | 30/90 | 77.4% |

## Conclusion

**GCC 2.8.1 (decompals psx build) + ASPSX 2.77 (PsyQ ~4.x), `-O2 -G8`.**
Every trial function reproduces instruction-for-instruction. `-O2` is chosen
over the equally-scoring `-O3` as the era default (same choice as the PE2
project); if a future function only matches with `-O3` semantics
(`-finline-functions`), revisit.

Systematic evidence behind the choice:

- **-G8, not -G0**: the retail binary makes heavy use of `$gp`-relative
  small-data addressing (1452 `%gp_rel` accesses in the split text). PE2, by
  contrast, was built `-G0`.
- **2.8.1, not 2.7.2**: gcc 2.7.2 allocates a scratch register around
  `mfhi` differently (`mfhi $a3; sra $v0,$a3,10` where 2.8.1 and the retail
  binary use `mfhi $v0; sra $v0,$v0,10` in `f9`).
- **2.6.3 eliminated**: visibly different store scheduling and delay-slot
  filling (80% even at its best flags).
- **maspsx must NOT get `--expand-div`**: retail `div` sequences are bare
  `div $zero,rs,rt; mflo` with no `bnez`/`break 7` / `break 6` trap
  expansion (PE2 needs the opposite, hence this explicit note).
- ASPSX version 2.77 vs 2.81/2.86 made no difference on these trials;
  2.77 (PsyQ 4.x era for a 1999-2000 title) is the working assumption.

`work/` holds the intermediate `.i`/`.s`/`.o` files (gitignored).
