#!/usr/bin/env python3
"""Probe DATA.BIN header and table region statistics."""
import collections, sys

P = "/Users/alexeykrasnopolsky/Desktop/silent_bomber/DATA.BIN"
d = open(P, "rb").read()
print("size", len(d), hex(len(d)))
h0, h1, h2 = (int.from_bytes(d[i:i+4], "little") for i in (0, 4, 8))
print(f"header: {h0:#x} {h1:#x} {h2:#x}")

def nibble_stats(lo, hi, label):
    c = collections.Counter()
    for b in d[lo:hi]:
        c[b >> 4] += 1
        c[b & 15] += 1
    tot = sum(c.values())
    top = ", ".join(f"{k:x}:{v/tot:.1%}" for k, v in c.most_common(8))
    print(f"{label} [{lo:#x}..{hi:#x}] nibbles: {top}")

nibble_stats(0x0C, 0xACC, "region A (0x0C..0xACC)")
nibble_stats(0x80, 0x2B30, "region B (0x80..0x2B30) 'entries'")
nibble_stats(0x2B30, 0x4E20, "region C (0x2B30..0x4E20)")

# hexdump small windows
def hd(lo, n, label):
    print(f"--- {label} @{lo:#x}")
    for i in range(lo, lo + n, 16):
        chunk = d[i:i+16]
        print(f"{i:08x}  " + " ".join(f"{b:02x}" for b in chunk))

hd(0, 0x100, "header + start")
hd(0x2B30, 0x80, "0x2B30 start")
hd(0x4E20 - 0x40, 0x80, "before 0x4E20")
hd(0x4E20, 0x80, "0x4E20 start")
# arithmetic check
print("0x80 + 0xACC*4 =", hex(0x80 + h0 * 4))
print("0xACC*4 =", hex(h0 * 4), " 0x2B30-0xACC =", hex(h1 - h0), " 0x4E20-0x2B30 =", hex(h2 - h1))
print("0x2B30 = 4*0xACC? ", h1 == 4 * h0, " 0x2B30/0xACC =", h1 / h0)
print("0x4E20/0x2B30 =", h2 / h1)
