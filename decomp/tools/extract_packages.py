#!/usr/bin/env python3
"""Extract Silent Bomber DATA.BIN packages as splat-ready binaries.

DATA.BIN has no file table: the package directory is a set of static
descriptors in SLUS_009.02 .data (see research/data_bin.md):

  D_800AC9E8: 36 pointer slots -> 9-word descriptors
      slots 0..27 = P00..P27, slots 32..35 = A00..A03
  D_800AC31C / D_800AC340 / D_800AC364: DEMOP / ENDINGP / ARENAP

9-word descriptor:
  w0 = part1 file offset      w1 = part1 size (SB-RLE stream of TIMs)
  w2 = align800(w1)           w3,w4,w5 = part2 sub-sizes
  w6 = align800(w3+w4+w5)     w7,w8 = part3 sub-sizes

Loader (func_80012E10) reads part2 as ONE raw read of w3+w4+w5 bytes to
0x800DD4F0, runs a registration pass over [w3, w3+w4), then reads part3
over [w3+w4, w3+w4+w7+w8). ARENAP is the exception: it loads at
0x801A0000 (constant stored in its loader thread struct, 0x80017790).

For every package this writes, under --out:
  <name>.part1.tim.bin   decompressed part1 (bundle of TIMs)
  <name>.part2.bin       raw part2 (w3+w4+w5 bytes; the splat target)
  <name>.part3.bin       raw part3 (QMD models/scenes)
and a codemap JSON with the per-package code extents used to generate
the splat overlay configs.

Code extents are measured, not hard-coded: function-prologue (addiu sp,
sp, -N) / jr-ra marker density plus a >=96% valid-opcode ratio per
0x100 window finds code runs; the .text span then snaps its start to
the first real prologue and its end past the last jr-ra / self jump
target. Verified: every code-bearing package starts .text on a
prologue, and j/jal targets computed against the load base land inside
the measured span (j-out ~ 0 for all 33 code-bearing packages).
"""
import argparse
import hashlib
import json
import os
import struct
import sys

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(REPO, "research"))

from sb_rle import decompress_stream  # noqa: E402
from rabbitizer import Instruction  # noqa: E402

BASE_DEFAULT = 0x800DD4F0   # D_800B5BB8, set once at boot (0x8001015C store)
BASE_ARENAP = 0x801A0000    # thread+0x50 constant at 0x80017790

STANDALONE = (("demop", 0x800AC31C), ("endingp", 0x800AC340), ("arenap", 0x800AC364))


def v2f(v):
    return v - 0x80010000 + 0x800


def package_descriptors(slus):
    def words(addr, n):
        return list(struct.unpack_from(f"<{n}I", slus, v2f(addr)))

    out = {}
    slots = words(0x800AC9E8, 36)
    names = {i: f"p{i:02d}" for i in range(28)}
    names.update({32 + i: f"a{i:02d}" for i in range(4)})
    for i, ptr in enumerate(slots):
        if ptr:
            out[names[i]] = (i, words(ptr, 9))
    for name, addr in STANDALONE:
        out[name] = (-1, words(addr, 9))
    return out


def find_code_runs(buf, base):
    """0x100 windows with >=62/64 valid opcodes near prologue/jr-ra markers."""
    n = len(buf)
    W = 0x100
    wins = (n + W - 1) // W
    valid = [0] * wins
    marker = [0] * wins
    for wi in range(wins):
        for i in range(wi * W, min((wi + 1) * W, n - 3), 4):
            w = struct.unpack_from("<I", buf, i)[0]
            if (w >> 16) == 0x27BD and (w & 0x8000):
                marker[wi] += 1
            if w == 0x03E00008:
                marker[wi] += 1
            if Instruction(w, vram=base + i).isValid():
                valid[wi] += 1
    per = W // 4
    cand = [valid[w] >= per - 2 for w in range(wins)]
    code = [False] * wins
    for w in range(wins):
        if marker[w]:
            for d in range(-3, 4):
                if 0 <= w + d < wins and cand[w + d]:
                    code[w + d] = True
    runs = []
    w = 0
    while w < wins:
        if code[w]:
            s = w
            gap = 0
            e = w
            while w < wins:
                if code[w]:
                    e = w
                    gap = 0
                else:
                    gap += 1
                    if gap > 2:
                        break
                w += 1
            runs.append((s * W, min((e + 1) * W, n)))
        else:
            w += 1
    return runs


def text_span(buf, base, runs, w3):
    """Snap [first run .. last run] to prologue start / last jr-ra or jump target."""
    r0s = runs[0][0]
    ts = None
    for i in range(max(0, r0s - 0x400), min(r0s + 0x200, len(buf) - 3), 4):
        w = struct.unpack_from("<I", buf, i)[0]
        if (w >> 16) == 0x27BD and (w & 0x8000):
            if i < r0s:
                n = (r0s - i) // 4
                nv = sum(1 for j in range(i, r0s, 4)
                         if Instruction(struct.unpack_from("<I", buf, j)[0], vram=base + j).isValid())
                if n and nv < n * 0.9:
                    continue
            ts = i
            break
    if ts is None:
        ts = r0s
    rls, rle = runs[-1]
    last_jr = 0
    for i in range(rls, min(rle, len(buf) - 3), 4):
        if buf[i:i + 4] == b"\x08\x00\xe0\x03":
            last_jr = i
    max_jt = 0
    for i in range(ts, min(rle, len(buf) - 3), 4):
        w = struct.unpack_from("<I", buf, i)[0]
        if (w >> 26) in (2, 3):
            t = (w & 0x03FFFFFF) << 2 | ((base + i + 4) & 0xF0000000)
            fo = t - base
            if ts <= fo < w3 and fo > max_jt:
                max_jt = fo
    te = (max(last_jr + 8, max_jt + 4) + 0xF) & ~0xF
    return ts, min(te, w3)


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sibling = os.path.join(REPO, "..", "silent_bomber")
    ap.add_argument("--data-bin", default=os.environ.get("SB_DATA_BIN", os.path.join(sibling, "DATA.BIN")))
    ap.add_argument("--slus", default=os.environ.get("SB_SLUS", os.path.join(sibling, "SLUS_009.02")))
    ap.add_argument("--out", default=os.path.join(REPO, "decomp", "assets", "USA", "packages"))
    ap.add_argument("--codemap", default=os.path.join(
        REPO, "decomp", "configs", "USA", "overlays", "codemap.json"))
    args = ap.parse_args()

    data = open(args.data_bin, "rb").read()
    slus = open(args.slus, "rb").read()
    os.makedirs(args.out, exist_ok=True)
    os.makedirs(os.path.dirname(args.codemap), exist_ok=True)

    codemap = {}
    for name, (slot, w) in sorted(package_descriptors(slus).items()):
        off = w[0]
        dec1, blocks, _ = decompress_stream(data[off:off + w[1]])
        p2 = data[off + w[2]:off + w[2] + w[3] + w[4] + w[5]]
        p3 = data[off + w[2] + w[6]:off + w[2] + w[6] + w[7] + w[8]]
        base = BASE_ARENAP if name == "arenap" else BASE_DEFAULT

        for suffix, payload in (("part1.tim", dec1), ("part2", p2), ("part3", p3)):
            with open(os.path.join(args.out, f"{name}.{suffix}.bin"), "wb") as f:
                f.write(payload)

        runs = find_code_runs(p2, base)
        entry = dict(
            slot=slot, file_off=off, part1_csize=w[1],
            w3=w[3], w4=w[4], w5=w[5],
            part3_off=off + w[2] + w[6], part3_size=w[7] + w[8],
            part2_size=len(p2), vram=base, tim_blocks=len(blocks),
            sha1_part2=hashlib.sha1(p2).hexdigest(),
            code_runs=[[a, b] for a, b in runs],
            text_start=None, text_end=None,
        )
        if runs:
            ts, te = text_span(p2, base, runs, w[3])
            entry.update(text_start=ts, text_end=te)
        codemap[name] = entry
        span = f"text={entry['text_start']:#x}..{entry['text_end']:#x}" if runs else "data-only"
        print(f"{name:<8} part2={len(p2):#7x} tims={len(blocks):>4} {span}")

    with open(args.codemap, "w") as f:
        json.dump(codemap, f, indent=1)
    print(f"\nwrote {len(codemap)} packages to {args.out}")
    print(f"codemap: {args.codemap}")


if __name__ == "__main__":
    main()
