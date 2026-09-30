#!/bin/bash
# Fast match-iteration harness: compile a snippet with the retail pipeline
# (clang -E -> cc1 2.8.1 -> maspsx ASPSX 2.77) and objdump the result.
# Usage: try.sh file.c [funcname]
set -e
cd "$(dirname "$0")/.."
SRC=$1
W=$(mktemp -d)
clang -E -P -xc -Iinclude -D_LANGUAGE_C -o "$W/x.i" "$SRC"
tools/compilers/macos/gcc-2.8.1-psx/cc1 -O2 -G8 -mips1 -w -funsigned-char \
    -fpeephole -ffunction-cse -fpcc-struct-return -fcommon -msoft-float \
    -mgas -fgnu-linker -quiet -o "$W/x.s" "$W/x.i"
python3 tools/maspsx/maspsx.py --aspsx-version=2.77 --run-assembler \
    --gnu-as-path=mipsel-linux-gnu-as -EL -O2 -march=r3000 -mtune=r3000 \
    -no-pad-sections -Iinclude -G8 -o "$W/x.o" "$W/x.s" 2>/dev/null
mipsel-linux-gnu-objdump -dr "$W/x.o"
rm -rf "$W"
