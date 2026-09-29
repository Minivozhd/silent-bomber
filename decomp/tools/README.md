# Vendored tools

Everything in this directory is a copy of an upstream open-source project
(or binaries built from one), vendored so the build is self-contained.
Provenance and licenses:

| Path | Upstream | Revision | License |
|---|---|---|---|
| `maspsx/` | https://github.com/mkst/maspsx | `111a2c9` (as used by the PE2 decomp) | MIT (see `maspsx/LICENSE`) |
| `asm-differ/` | https://github.com/simonlindholm/asm-differ | `6299ebf` | CC0 / public domain (see `asm-differ/LICENSE`) |
| `m2c/` | https://github.com/matt-kempster/m2c | `3478473` | GPL-3.0 (see `m2c/LICENSE`) |
| `splat_ext/` | PE2 decomp project (`tools/splat_ext`: `libsrc.py`, `variant src`) | local copy | CC0 (PE2 project license) |
| `compilers/*/gcc-2.8.1-psx` | https://github.com/decompals/old-gcc (stock gcc 2.8.1 + decompals psx patches) | binaries copied from the PE2 decomp project `tools/{macos,linux}` | GPL-2.0 (binaries; source at the linked repo) |
| `compilers/*/gcc-2.7.2-cdk` | https://github.com/decompals/old-gcc (gcc 2.7.2 CDK) | binaries copied from the PE2 decomp project `tools/{macos,linux}` | GPL-2.0 |
| `compilers/linux/gcc-2.6.3-psx` | https://github.com/decompals/old-gcc release 0.17 (`gcc-2.6.3-psx.tar.gz`) | release 0.17 | GPL-2.0 |
| `fingerprint/` | written for this project | — | CC0 |

Notes:

- The `macos` compiler builds are native arm64 Mach-O binaries; the `linux`
  ones are statically linked 32-bit x86 ELF binaries (run them in the
  amd64-pinned Docker container, see `../Dockerfile`).
- These old `cc1` binaries segfault on comments in their input — always feed
  them preprocessed source (cpp -> cc1 -> maspsx), exactly like the PE2
  pipeline does.
- The PE2 project applies four local patches to maspsx
  (`maspsx-lo-load-nop.patch`, `maspsx-label-load-nop.patch`,
  `maspsx-li-d-register-pair.patch`, `maspsx-bss-align.patch`). They were
  PE2-specific workarounds and are **not** applied here; the vendored maspsx
  is pristine upstream.
