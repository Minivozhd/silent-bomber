#!/usr/bin/env python3
"""Silent Bomber randomizer prototype: shuffle gmItem drop contents.

Patches item ids in-place inside DATA.BIN package part2 blobs — no
repacking needed (sizes unchanged). For ISO injection pair with an
ECC/EDC-aware sector patcher (see pe2-mod-tools/file-manager/rom_patcher.py).

Usage: randomize.py <DATA.BIN> <seed> [--dry-run]
"""
import random
import struct
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
from part2_parse import Part2  # entity decoding (offsets below)

# --- DATA.BIN package directory: 9-word descriptors in SLUS_009.02 --------
# D_800AC9E8 (vram) -> file offset in SLUS_009.02; 36 slots of 9 u32:
# [off1, size1, align800(size1), s2a,s2b,s2c, delta3, s3a,s3b]
# (see research/data_bin.md). part2 of slot N starts at its descriptor's
# data-region offset; the mission package name is P<slot:02d> for slots 0-27,
# A0<slot-32> for 32-35.
SLUS = __file__.rsplit('/', 2)[0] + '/SLUS_009.02'
SLUS_BASE = 0x80010000
SLUS_HDR = 0x800
DESCRIPTORS_VRAM = 0x800AC9E8

def load_descriptors(slus_path=SLUS):
    """D_800AC9E8 is a table of 36 POINTERS to 9-word descriptors.
    9-word: w0=part1 file offset, w1=part1 compressed size, w2=align800(w1)
    (part2 delta), w3+w4+w5 = part2 size (w3=w3 region, w4/w5 = sound banks),
    part3 at w0+w2+w6 (size w7+w8)."""
    d = open(slus_path, 'rb').read()
    off = DESCRIPTORS_VRAM - SLUS_BASE + SLUS_HDR
    out = []
    for i in range(36):
        ptr = struct.unpack_from('<I', d, off + i * 4)[0]
        if ptr < SLUS_BASE or ptr >= SLUS_BASE + len(d) - SLUS_HDR:
            out.append(None)  # empty slot
            continue
        doff = ptr - SLUS_BASE + SLUS_HDR
        w = struct.unpack_from('<9I', d, doff)
        out.append(w)
    return out

def package_part2_range(w):
    """part2 offset/size in DATA.BIN from a 9-word descriptor."""
    return w[0] + w[2], w[3] + w[4] + w[5]

def items_in(part2):
    """Yield (record_offset, item_id) for every gmItem entity record."""
    p = Part2(part2, 0x800DD4F0, {})
    for ent in p.entity_runs():
        pass  # entity_runs yields runs; decode below

def find_items(part2, base_vram=0x800DD4F0):
    """Scan for gmItem records directly: gmItem ctor = main:0x8006f750,
    name ptr -> 'gmItem'. Records are 0x24 bytes; item id = param byte1."""
    items = []
    # name string 'gmItem\0' occurrences
    start = 0
    while True:
        i = part2.find(b'gmItem\x00', start)
        if i < 0:
            break
        name_vram = base_vram + i
        # find records whose +0x20 ptr == name_vram
        raw = struct.pack('<I', name_vram)
        rs = 0
        while True:
            j = part2.find(raw, rs)
            if j < 0:
                break
            rec = j - 0x20
            if rec >= 0:
                param = struct.unpack_from('<I', part2, rec + 0x1C)[0]
                if param & 0xFF == 1:  # gmItem records end in 01
                    item_id = (param >> 8) & 0xFF
                    items.append((rec, item_id))
            rs = j + 1
        start = i + 1
    return items

def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return 1
    path, seed = sys.argv[1], int(sys.argv[2])
    dry = '--dry-run' in sys.argv
    data = open(path, 'rb').read()
    out = bytearray(data)
    rng = random.Random(seed)
    descs = load_descriptors()
    total = 0
    for slot, w in enumerate(descs):
        if w is None:
            continue
        off1, size1 = w[0], w[1]
        if off1 == 0 or size1 == 0:
            continue
        p2off = off1 + w[2]  # align800(size1)
        p2 = bytes(data[p2off:p2off + w[3]])
        items = find_items(p2)
        if not items:
            continue
        name = 'P%02d' % slot if slot < 28 else 'A%02d' % (slot - 32)
        ids = [iid for _, iid in items]
        shuffled = ids[:]
        rng.shuffle(shuffled)
        for (rec, old), new in zip(items, shuffled):
            if old != new:
                print('%s @0x%X: item %d -> %d' % (name, p2off + rec, old, new))
                if not dry:
                    # param word: 0x0000NN01 -> replace NN
                    struct.pack_into('<B', out, p2off + rec + 0x1C + 1, new)
                total += 1
    print('patched %d items across packages (seed %d)%s' %
          (total, seed, ' [dry-run]' if dry else ''))
    if not dry and total:
        open(path, 'wb').write(out)
        print('written:', path)
    return 0

if __name__ == '__main__':
    sys.exit(main())
