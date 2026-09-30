#!/usr/bin/env python3
"""Instruction-level diff of a built C function vs its original asm.

Usage: tools/diff_func.py <tu> <func>     (e.g. tools/diff_func.py sbrle func_80012BB0)

Compares build/USA/src/main/<tu>.c.o (<func>) against
asm/USA/main/nonmatchings/<tu>/<func>.s, normalizing addresses/symbols the
same way tools/fingerprint/trial.py does. Prints a unified diff and a score.
"""
import re
import subprocess
import sys
from pathlib import Path

DECOMP = Path(__file__).resolve().parent.parent
OBJDUMP = "mipsel-linux-gnu-objdump"


def parse_original(tu, func):
    text = (DECOMP / f"asm/USA/main/nonmatchings/{tu}/{func}.s").read_text()
    m = re.search(rf"glabel {func}\n(.*?)\nendlabel", text, re.S)
    out = []
    for line in m.group(1).splitlines():
        mm = re.match(r"\s+/\*\s*[0-9A-F]+\s+[0-9A-F]+\s+([0-9A-F]{8})\s*\*/\s+(.*)", line)
        if mm:
            out.append(normalize(mm.group(2)))
    return out


def parse_built(tu, func):
    r = subprocess.run(
        [OBJDUMP, "-drz", str(DECOMP / f"build/USA/src/main/{tu}.c.o")],
        capture_output=True, text=True)
    out = []
    in_func = False
    for line in r.stdout.splitlines():
        hdr = re.match(r"^[0-9a-f]+ <([\w.]+)>:", line)
        if hdr:
            in_func = hdr.group(1) == func
            continue
        mm = re.match(r"\s+[0-9a-f]+:\s+[0-9a-f]{8}\s+\t?(.*)", line)
        if mm and in_func:
            out.append(normalize(mm.group(1)))
    return out


def normalize(insn):
    insn = insn.split("#")[0].strip()

    def shifthi(m):
        return str(int(m.group(1), 16) >> 16)

    def andlo(m):
        return str(int(m.group(1), 16) & 0xFFFF)

    insn = re.sub(r"\((0x[0-9A-Fa-f]+)\s*>>\s*16\)", shifthi, insn)
    insn = re.sub(r"\((0x[0-9A-Fa-f]+)\s*&\s*0xFFFF\)", andlo, insn, flags=re.I)
    insn = re.sub(r"%(hi|lo)\([^)]*\)", "0xHI", insn)
    insn = re.sub(r"%gp_rel\([^)]*\)", "SYM", insn)
    insn = re.sub(r",-?\d+\(gp\)", ",SYM(gp)", insn)
    insn = re.sub(r"\bD_[0-9A-Fa-f]+\b", "SYM", insn)
    insn = re.sub(r"\bfunc_[0-9A-Fa-f]+\b", "SYM", insn)
    insn = insn.replace("$", "")
    insn = re.sub(r"0x[0-9a-fA-F]+|\b\d+\b", lambda m: str(int(m.group(0), 0)), insn)
    insn = re.sub(r"<[^>]*>", "", insn)
    insn = re.sub(r"\s*,\s*", ",", insn)
    insn = re.sub(r"\s+", " ", insn).strip()
    if re.match(r"^(beq|bne|beqz|bnez|bgez|bgezal|bgtz|blez|bltz|bltzal)\b", insn):
        insn = re.sub(r",[^,]+$", ",LBL", insn)
    insn = re.sub(r"^j \.?\w+", "j LBL", insn)
    insn = re.sub(r"\.L\w+", "LBL", insn)
    insn = re.sub(r"^jal .+$", "jal SYM", insn)
    insn = re.sub(r"^li (\w+),(-?\d+)$", r"addiu \1,zero,\2", insn)
    insn = re.sub(r"^move (\w+),(\w+)$", r"addu \1,\2,zero", insn)
    return insn


def main():
    tu, func = sys.argv[1], sys.argv[2]
    orig = parse_original(tu, func)
    built = parse_built(tu, func)
    import difflib
    for line in difflib.unified_diff(orig, built, "original", "built", lineterm="", n=3):
        print(line)
    ratio = difflib.SequenceMatcher(a=orig, b=built).ratio()
    print(f"\nsimilarity: {ratio*100:.1f}% ({len(orig)} original vs {len(built)} built instructions)")


if __name__ == "__main__":
    main()
