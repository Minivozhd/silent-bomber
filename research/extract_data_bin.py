#!/usr/bin/env python3
"""Silent Bomber DATA.BIN extractor.

Package table lives STATICALLY in SLUS_009.02 .data (not in DATA.BIN):
  - D_800AC9E8: 36 ptr slots -> 9-word descriptors (P00..P27 at slots 0..27,
    A00..A03 at slots 32..35 by elimination)
  - D_800AC448 / D_800AC538: 12+12 ptrs -> 4-word descriptors
  - D_800AC300/31C/340/364: standalone descriptors (boot/demo/ending/arena?)
  - D_800AC2E8: (0, 0x2B38) boot TIM stream; D_800AC2F0/2F8: raw pairs

Descriptor layouts (verified in func_80012E10 / func_800132D8 / func_80013000):
  9-word: w0=part1_file_off, w1=part1_csize, w2=align800(w1) -> part2 delta,
          w3,w4,w5 = part2 sub-sizes (loaded as one raw read of w3+w4+w5 at w0+w2),
          w6 = part3 delta, w7,w8 = part3 sizes (raw read w7+w8 at w0+w2+w6)
  4-word: w0=off, w1=csize (compressed stream), w2=delta, w3=raw size
  7-word (boot): w0=off, w1=load size; compressed stream at off (len = consumed),
          raw tail follows immediately; w2..w6 = in-RAM layout offsets
"""
import os, struct, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from sb_rle import decompress_stream

ROOT = "/Users/alexeykrasnopolsky/Desktop/silent_bomber"
OUT = os.path.join(ROOT, "experiments", "out")
DATA = open(os.path.join(ROOT, "DATA.BIN"), "rb").read()
SLUS = open(os.path.join(ROOT, "SLUS_009.02"), "rb").read()

def v2f(vaddr):
    return vaddr - 0x80010000 + 0x800

def words(addr, n):
    return list(struct.unpack_from(f"<{n}I", SLUS, v2f(addr)))

def ptr_list(addr, n):
    return [w for w in words(addr, n)]

def align800(x):
    return (x + 0x7FF) & ~0x7FF

def classify(buf):
    if not buf:
        return "empty"
    tags = []
    if buf[:4] == b"\x10\x00\x00\x00" and buf[4] in (8, 9, 2, 0, 1):
        flags = struct.unpack_from("<I", buf, 4)[0]
        bpp = flags & 7
        tags.append(f"TIM(bpp={bpp},clut={bool(flags & 8)})")
    if buf[:4] == b"QMD " or b"QMD " in buf[:0x40]:
        tags.append("QMD-model")
    # MIPS code density: addiu sp,sp,-N = 27 BD xx FF; jr ra = 08 00 E0 03
    n = min(len(buf), 0x8000)
    pro = sum(1 for i in range(0, n - 4, 4) if buf[i + 3] == 0x27 and buf[i + 2] == 0xBD)
    jr = sum(1 for i in range(0, n - 4, 4) if buf[i:i+4] == b"\x08\x00\xe0\x03")
    if pro >= 4 or jr >= 8:
        tags.append(f"MIPS-code(pro={pro},jrra={jr})")
    # ascii strings
    strs = []
    cur = b""
    for b in buf[:0x10000]:
        if 32 <= b < 127:
            cur += bytes([b])
        else:
            if len(cur) >= 6:
                strs.append(cur.decode())
            cur = b""
            if len(strs) >= 6:
                break
    if strs:
        tags.append("str:" + " | ".join(strs[:4])[:90])
    return "; ".join(tags) if tags else "data"

packages = []  # (name, parts=[(part_name, file_off, raw_bytes or None, decompressed or None)])

def add_9word(name, desc_addr):
    w = words(desc_addr, 9)
    off = w[0]
    parts = []
    span = w[2] + w[6] + align800(w[7] + w[8])
    # part1 compressed
    raw1 = DATA[off:off + w[1]]
    try:
        dec1, blocks, consumed = decompress_stream(raw1)
        p1 = ("part1.rle", off, w[1], dec1, len(blocks))
    except Exception as e:
        p1 = ("part1.rle", off, w[1], f"FAIL: {e}", 0)
    parts.append(p1)
    s2 = w[3] + w[4] + w[5]
    if s2:
        parts.append(("part2.raw", off + w[2], s2, DATA[off + w[2]:off + w[2] + s2], (w[3], w[4], w[5])))
    s3 = w[7] + w[8]
    if s3:
        parts.append(("part3.raw", off + w[2] + w[6], s3, DATA[off + w[2] + w[6]:off + w[2] + w[6] + s3], (w[7], w[8])))
    packages.append((name, off, span, w, parts))

def add_4word(name, desc_addr):
    w = words(desc_addr, 4)
    off = w[0]
    parts = []
    raw1 = DATA[off:off + w[1]]
    try:
        dec1, blocks, consumed = decompress_stream(raw1)
        parts.append(("part1.rle", off, w[1], dec1, len(blocks)))
    except Exception as e:
        parts.append(("part1.rle", off, w[1], f"FAIL: {e}", 0))
    if w[3]:
        parts.append(("part2.raw", off + w[2], w[3], DATA[off + w[2]:off + w[2] + w[3]], None))
    packages.append((name, off, w[2] + align800(w[3]), w, parts))

# --- standalone descriptors ---
add_9word("pkg_9E800", 0x800AC31C)   # candidate DEMO/ENDING/ARENA
add_9word("pkg_EB000", 0x800AC340)
add_9word("pkg_13A7800", 0x800AC364)

# --- 12+12 4-word groups ---
grpA = ptr_list(0x800AC448, 12)
grpB = ptr_list(0x800AC538, 12)
for i, p in enumerate(grpA):
    add_4word(f"grpA_{i:02d}", p)
for i, p in enumerate(grpB):
    add_4word(f"grpB_{i:02d}", p)

# --- main P/A table ---
slots = ptr_list(0x800AC9E8, 36)
names = {}
for i in range(28):
    names[i] = f"P{i:02d}"
for i in range(4):
    names[32 + i] = f"A{i:02d}"
for i, p in enumerate(slots):
    if p:
        add_9word(names.get(i, f"slot{i}"), p)

# --- boot packages ---
# boot TIM stream at (0, 0x2B38)
dec, blocks, consumed = decompress_stream(DATA[0:0x2B38])
packages.insert(0, ("boot_tim", 0, 0x2B38, None, [("part1.rle", 0, 0x2B38, dec, len(blocks))]))
# boot main at 0x3000, load 0x9B800: compressed stream + raw tail
w = words(0x800AC300, 7)
dec, blocks, consumed = decompress_stream(DATA[0x3000:0x3000 + w[1]])
tail_off = 0x3000 + consumed
tail = DATA[tail_off:0x3000 + w[1]]
packages.insert(1, ("boot_main", 0x3000, w[1], w,
                    [("part1.rle", 0x3000, consumed, dec, len(blocks)),
                     ("tail.raw", tail_off, len(tail), tail, None)]))
# "raw" pairs D_800AC2F0/D_800AC2F8 are actually standard RLE streams too
# (decompressed on demand via the single-block path func_80013588/func_80012BB0)
for name, off, sz in (("bundle_160800", 0x160800, 0x20968), ("bundle_181800", 0x181800, 0x1F2E0)):
    try:
        dec, blocks, consumed = decompress_stream(DATA[off:off + sz])
        packages.append((name, off, sz, None, [("rle", off, sz, dec, len(blocks))]))
    except Exception as e:
        packages.append((name, off, sz, None, [("raw", off, sz, DATA[off:off + sz], None)]))

# --- write out + report ---
os.makedirs(OUT, exist_ok=True)
print(f"{'package':<14} {'file@':>10} {'span':>9}  parts")
total = 0
for name, off, span, w, parts in packages:
    total += span
    print(f"{name:<14} {off:#010x} {span:#9x}")
    for pname, poff, psz, payload, extra in parts:
        fn = f"{name}.{pname.replace('.', '_')}.bin"
        with open(os.path.join(OUT, fn), "wb") as f:
            if isinstance(payload, bytes):
                f.write(payload)
        if isinstance(payload, bytes):
            cls = classify(payload)
            print(f"    {pname:<10} @{poff:#010x} {psz:#9x} -> {len(payload):#9x}  {cls}")
        else:
            print(f"    {pname:<10} @{poff:#010x} {psz:#9x}  {payload}")
print(f"total covered: {total:#x} of {len(DATA):#x}")
