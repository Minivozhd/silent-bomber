#!/usr/bin/env python3
"""part2_parse.py — decode the part2 (mission/level data) of a Silent Bomber
DATA.BIN package into readable JSON-ish text.

Usage:
    part2_parse.py [PKG]          # PKG = P01 (default), P02, ..., A00, ...
    part2_parse.py P01 --json     # machine-readable JSON instead of pretty text

Reads:
    out/<PKG>.part2_raw.bin                       (raw part2, one read of w3+w4+w5)
    codemap.json (optional, for w3/w4/w5 + code extent) — searched at
    ../silent-bomber/decomp/configs/USA/overlays/codemap.json by default.

Format reference: experiments/part2_format.md.
Pure Python 3 stdlib.
"""
import json
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
CODEMAP = os.path.normpath(os.path.join(
    HERE, "..", "..", "silent-bomber", "decomp", "configs", "USA",
    "overlays", "codemap.json"))

# Fallback geometry for P01 if codemap.json is unavailable.
FALLBACK = {"p01": {"vram": 0x800DD4F0, "w3": 112316, "w4": 19020,
                    "w5": 184028, "text_start": 0x6C34, "text_end": 0x1ACC0}}

MAIN_LO, MAIN_HI = 0x80010000, 0x800B0000  # SLUS_009.02 resident address range


def f16(v):
    """s32 as 16.16 fixed point."""
    v = v - 0x100000000 if v >= 0x80000000 else v
    return v / 65536.0


class Part2:
    def __init__(self, buf, vram, meta):
        self.buf = buf
        self.vram = vram
        self.meta = meta

    def u32(self, o):
        return struct.unpack_from("<I", self.buf, o)[0]

    def is_self_ptr(self, v):
        return self.vram <= v < self.vram + len(self.buf)

    def is_main_ptr(self, v):
        return MAIN_LO <= v < MAIN_HI

    def cstr(self, o):
        e = self.buf.find(b"\x00", o)
        if e < 0:
            e = len(self.buf)
        return self.buf[o:e].decode("ascii", "replace")

    def ptr_off(self, v):
        return v - self.vram

    # ------------------------------------------------------------------
    # header: u32 + records of 0x2C while the 0x0001xxxx marker holds
    # ------------------------------------------------------------------
    def header(self):
        out = {"count_marker": self.u32(0), "records_0x2c": []}
        o = 4
        while o + 0x2C <= len(self.buf):
            w0 = self.u32(o)
            if (w0 >> 16) != 0x0001:
                break
            words = [self.u32(o + 4 * k) for k in range(11)]
            rec = {
                "off": o,
                "tag": w0 & 0xFFFF,
                "w1": words[1],
                "fixed16_16": [round(f16(w), 6) for w in words[2:]],
            }
            out["records_0x2c"].append(rec)
            o += 0x2C
        out["homogeneous_end"] = o
        return out

    # ------------------------------------------------------------------
    # string table: every NUL-terminated printable string (>=4 chars)
    # ------------------------------------------------------------------
    def strings(self):
        out = []
        i = 0
        buf = self.buf
        while True:
            # find next run of >=4 printable chars followed by NUL
            j = i
            while j < len(buf):
                c = buf[j]
                if 32 <= c < 127:
                    j += 1
                    continue
                if c == 0 and j - i >= 4:
                    out.append({"off": i, "str": buf[i:j].decode()})
                i = j + 1
                break
            else:
                break
            if j >= len(buf):
                break
        return out

    # ------------------------------------------------------------------
    # entity/spawn records: 0x24-byte structs
    #   +0x00 u32 tag   (0x000100NN — hi u16 always 1; lo u16 = group/class?)
    #   +0x04 u32       (always 1 in detected runs)
    #   +0x08 u32 flags (byte3 looks like facing: 0/4/8/0x0C/0x0E)
    #   +0x0C ctor      (function ptr: overlay code or main exe)
    #   +0x10..0x1B     3x s32 16.16 fixed-point position
    #   +0x1C u32 param (gmItem: 0x0000NN01 -> item id NN)
    #   +0x20 ptr       -> object-name string (gmXX..., gmItem, enXX...)
    # ------------------------------------------------------------------
    def looks_like_entity(self, o):
        w0 = self.u32(o)
        w1 = self.u32(o + 4)
        fn = self.u32(o + 0x0C)
        nm = self.u32(o + 0x20)
        if (w0 >> 16) != 0x0001 or not (1 <= (w0 & 0xFFFF) <= 8):
            return False
        if not (1 <= w1 <= 4):
            return False
        if not (self.is_self_ptr(fn) or self.is_main_ptr(fn)):
            return False
        if not self.is_self_ptr(nm):
            return False
        off = self.ptr_off(nm)
        if not (0 <= off < len(self.buf)):
            return False
        e = self.buf.find(b"\x00", off, off + 24)
        return e > off and all(32 <= c < 127 for c in self.buf[off:e])

    def decode_entity(self, o):
        w = [self.u32(o + 4 * k) for k in range(9)]
        fn = w[3]
        if self.is_self_ptr(fn):
            fn_s = f"overlay+{fn - self.vram:#07x}"
        else:
            fn_s = f"main:{fn:#010x}"
        return {
            "off": o,
            "tag": w[0] & 0xFFFF,
            "u1": w[1],
            "flags": w[2],
            "ctor": fn_s,
            "pos": [round(f16(w[4]), 4), round(f16(w[5]), 4),
                    round(f16(w[6]), 4)],
            "param": w[7],
            "name": self.cstr(self.ptr_off(w[8])),
        }

    def entity_runs(self):
        recs = [o for o in range(0, len(self.buf) - 0x24, 4)
                if self.looks_like_entity(o)]
        runs, cur = [], []
        for o in recs:
            if cur and o - cur[-1] != 0x24:
                if len(cur) >= 2:
                    runs.append(cur)
                cur = []
            cur.append(o)
        if len(cur) >= 2:
            runs.append(cur)
        return [[self.decode_entity(o) for o in r] for r in runs]

    # ------------------------------------------------------------------
    # w4/w5 regions: [u32 count][count u32]... streams, 0-terminated.
    # Registered pairwise by func_80012D34/func_80019224 (sound banks:
    # stream A blobs carry the "VABp" magic, stream B = waveform data).
    # ------------------------------------------------------------------
    def count_stream(self, off, limit):
        groups = []
        o = off
        while o + 4 <= limit:
            n = self.u32(o)
            if n == 0:
                return groups, o
            if o + 4 + 4 * n > limit + 4:
                break
            first = self.buf[o + 4:o + 8]
            groups.append({
                "off": o, "words": n, "bytes": 4 * n,
                "magic": first.decode("ascii", "replace")
                if all(32 <= c < 127 for c in first) else first.hex(),
            })
            o += 4 + 4 * n
        return groups, o

    # ------------------------------------------------------------------
    # pointer-density map (4KiB buckets) — helps spotting table regions
    # ------------------------------------------------------------------
    def ptr_map(self):
        buckets = {}
        for i in range(0, len(self.buf) - 4, 4):
            v = self.u32(i)
            if self.is_self_ptr(v):
                buckets.setdefault(i // 0x1000 * 0x1000, 0)
                buckets[i // 0x1000 * 0x1000] += 1
        return [{"region": b, "self_ptrs": n} for b, n in sorted(buckets.items())]

    def parse(self):
        m = self.meta
        w3, w4 = m.get("w3"), m.get("w4")
        ts, te = m.get("text_start"), m.get("text_end")
        res = {
            "package": m["name"],
            "size": len(self.buf),
            "vram": m["vram"],
            "regions": {
                "w3 (header+tables+strings+.text+rodata)": [0, w3],
                "w4 (sound-bank stream A)": [w3, w3 + w4],
                "w5 (sound-bank stream B + pad)": [w3 + w4, len(self.buf)],
                ".text (MIPS overlay)": [ts, te],
            },
            "header": self.header(),
            "entity_runs": self.entity_runs(),
            "strings": self.strings(),
        }
        if w3 and w4:
            ga, term = self.count_stream(w3, w3 + w4)
            gb, endb = self.count_stream(w3 + w4, len(self.buf))
            res["sound_streams"] = {
                "stream_A (w4)": {"groups": ga, "terminator": term},
                "stream_B (w5)": {"groups": gb, "end": endb},
            }
        res["ptr_map"] = self.ptr_map()
        return res


def pretty(res):
    L = []
    a = L.append
    a(f"== {res['package']} part2 == size={res['size']} "
      f"({res['size']:#x}) vram={res['vram']:#010x}")
    a("-- regions --")
    for k, (s, e) in res["regions"].items():
        if s is not None and e is not None:
            a(f"  {s:#07x}..{e:#07x}  {k}")
    h = res["header"]
    a(f"-- header -- u32@0 = {h['count_marker']} "
      f"(homogeneous 0x2C records end @ {h['homogeneous_end']:#x})")
    for r in h["records_0x2c"]:
        a(f"  @{r['off']:#06x} tag={r['tag']} w1={r['w1']:#010x} "
          f"f16={r['fixed16_16']}")
    for run in res["entity_runs"]:
        a(f"-- entity/spawn run @ {run[0]['off']:#07x}, {len(run)} records --")
        for r in run:
            a(f"  @{r['off']:#06x} {r['name']:<14} tag={r['tag']} "
              f"flags={r['flags']:#010x} ctor={r['ctor']:<16} "
              f"pos={r['pos']} param={r['param']:#010x}")
    ss = res.get("sound_streams")
    if ss:
        a("-- sound-bank streams (func_80012D34 / func_80019224) --")
        for k, v in ss.items():
            a(f"  {k}: {len(v['groups'])} groups")
            for g in v["groups"][:8]:
                a(f"    @{g['off']:#07x} {g['bytes']:>7} bytes "
                  f"magic/first4={g['magic']!r}")
            a(f"    terminator/end @ {v.get('terminator', v.get('end')):#x}")
    a(f"-- strings ({len(res['strings'])}) --")
    for s in res["strings"]:
        a(f"  @{s['off']:#07x} {s['str']}")
    a("-- self-pointer density (4KiB buckets) --")
    for p in res["ptr_map"]:
        a(f"  {p['region']:#07x}: {p['self_ptrs']}")
    return "\n".join(L)


def main():
    pkg = sys.argv[1] if len(sys.argv) > 1 else "P01"
    as_json = "--json" in sys.argv
    path = os.path.join(HERE, "out", f"{pkg.upper()}.part2_raw.bin")
    if not os.path.exists(path):
        sys.exit(f"no such file: {path}")
    buf = open(path, "rb").read()
    meta = None
    if os.path.exists(CODEMAP):
        cm = json.load(open(CODEMAP))
        meta = cm.get(pkg.lower())
    if meta is None:
        meta = FALLBACK.get(pkg.lower())
    if meta is None:
        sys.exit(f"no geometry for {pkg} (codemap {CODEMAP} missing); "
                 f"only these fallbacks: {sorted(FALLBACK)}")
    meta = dict(meta)
    meta["name"] = pkg.upper()
    res = Part2(buf, meta["vram"], meta).parse()
    if as_json:
        print(json.dumps(res, indent=1))
    else:
        print(pretty(res))


if __name__ == "__main__":
    main()
