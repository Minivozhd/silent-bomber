# Silent Bomber (PS1) — reverse engineering project

A reverse-engineering project for *Silent Bomber* (Sony PlayStation, USA,
`SLUS-00902`, CyberConnect2 / Bandai, 1999–2000), in the spirit of
[GabeRealB/parasite-eve-2-decomp](https://github.com/GabeRealB/parasite-eve-2-decomp):

1. **Matching decompilation** of the main executable (`SLUS_009.02`) and the
   data packages inside `DATA.BIN`.
2. **Mod tools** — extract and re-insert any game file (ISO level and
   `DATA.BIN` package level).
3. **Randomizer** — seeds for drops / upgrades / enemy stats (later phase).

> **You must own the game.** This repository contains no ROMs, disc images,
> or extracted game assets. Obtain a legal dump of your own disc.

## Documentation

- [Project plan](docs/plan.md) — inventory, roadmap, open questions.
- [Image analysis](docs/image-analysis.md) — disc contents, the executable,
  the `DATA.BIN` package container and its loader.

## Layout

```
decomp/
  slus_009.02.yaml   splat configuration (first split done: 67% code)
  configs/USA/       PE2-style splat configs (main.yaml, overlays/)
  linkers/USA/       linker scripts
  asm/               split output (disassembly)
  src/               decompiled C sources (to come)
  tools/             extract_packages.sh/.py + gen_overlay_configs.py
docs/                reverse-engineering notes (see above)
```

## Toolchain (local, not committed)

- Python 3.13 venv with `splat64`, `spimdisasm`, `rabbitizer`, `n64img`,
  `crunch64`.
- Docker toolchain image for the matching build (PsyQ GCC — version TBD by
  codegen fingerprinting; PE2 uses GCC 2.8.1 + maspsx).

## License / disclaimer

Project code: CC0. This is an unofficial fan project, not affiliated with
Bandai Namco or CyberConnect2. *Silent Bomber* is a trademark of its owners.
