#!/usr/bin/env python3
"""Compiler fingerprint trials for SLUS_009.02.

Compiles minimal C equivalents of hand-picked leaf functions from the split
asm with candidate period toolchains (gcc 2.6.3 / 2.7.2-cdk / 2.8.1-psx, all
through maspsx with several ASPSX versions) and scores the result against the
original instructions.

gcc 2.6.3 only exists as a static i386 Linux binary; it runs inside docker
(cc1 only - the .s it emits is then processed natively by maspsx + GNU as,
exactly like the other compilers).

Usage: python3 trial.py [--verbose]
"""

import argparse
import difflib
import re
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
DECOMP = HERE.parent.parent
ASM_FILE = DECOMP / "asm" / "800.s"
MASPSX = DECOMP / "tools" / "maspsx" / "maspsx.py"
GNU_AS = "mipsel-linux-gnu-as"
OBJDUMP = "mipsel-linux-gnu-objdump"
DOCKER = "/usr/local/bin/docker"
WORK = HERE / "work"

# candidate C file -> target function in asm/800.s
CANDIDATES = {
    "f1_counter.c": "func_80010364",
    "f2_linknode.c": "func_80034C90",
    "f3_zerofill.c": "func_80055E78",
    "f4_tablecopy.c": "func_8005E938",
    "f5_constsynth.c": "func_8005F088",
    "f6_funcptr.c": "func_8005BAA0",
    "f7_multclamp.c": "func_8001AFB8",
    "f8_schedbranch.c": "func_80035330",
    "f9_magicdiv.c": "func_8005D558",
}

# PE2-proven cc1 flag base (gcc 2.8.1 + PsyQ); -O level and -G vary per trial.
CC1_BASE = (
    "-mips1 -w -funsigned-char -fpeephole -ffunction-cse "
    "-fpcc-struct-return -fcommon -msoft-float -mgas -fgnu-linker -quiet"
)

# PE2 passes --expand-div to maspsx because its retail div sequences carry
# the ASPSX zero-div/overflow trap sequences. SLUS_009.02's divs are bare
# `div $zero,rs,rt; mflo` with no traps, so maspsx must run WITHOUT
# --expand-div (it then rewrites 2-operand div to the trap-free 3-op form).
EXPAND_DIV = False

COMPILERS = {
    # name: (cc1 path, runner)  runner None = native, "docker" = via container
    "gcc-2.8.1-psx": (DECOMP / "tools/compilers/macos/gcc-2.8.1-psx/cc1", None),
    "gcc-2.7.2-cdk": (DECOMP / "tools/compilers/macos/gcc-2.7.2-cdk/cc1", None),
    "gcc-2.6.3-psx": (Path("/cc/cc1"), "docker"),
}

OPT_LEVELS = ["-O1", "-O2", "-O3"]
G_LEVELS = ["-G0", "-G8"]
ASPSX_VERSIONS = ["2.77", "2.81", "2.86"]

# Docker-based compilers pay ~2s per container start; only run the combos
# that matter for the report (the native compilers get the full matrix).
DOCKER_COMBOS = [("-O2", "-G8"), ("-O3", "-G8")]


def parse_original(func_name):
    """Return list of (word, normalized_insn) for the function in asm/800.s."""
    text = ASM_FILE.read_text()
    m = re.search(rf"glabel {func_name}\n(.*?)\nendlabel", text, re.S)
    out = []
    for line in m.group(1).splitlines():
        mm = re.match(r"\s+/\*\s*[0-9A-F]+\s+[0-9A-F]+\s+([0-9A-F]{8})\s*\*/\s+(.*)", line)
        if mm:
            out.append((mm.group(1), normalize(mm.group(2))))
    return out


def normalize(insn):
    """Canonical form: strip $, erase symbol names, unify number bases.

    Handles both spimdisasm style (`lhu $v0, %gp_rel(D_x)($gp)`,
    `lui $v1, (0x55555555 >> 16)`) and objdump style (`lhu v0,0(gp)`,
    `lui v1,0x5555`).
    """
    insn = insn.split("#")[0].strip()
    def shifthi(m):
        return str(int(m.group(1), 16) >> 16)
    def andlo(m):
        return str(int(m.group(1), 16) & 0xFFFF)
    insn = re.sub(r"\((0x[0-9A-Fa-f]+)\s*>>\s*16\)", shifthi, insn)
    insn = re.sub(r"\((0x[0-9A-Fa-f]+)\s*&\s*0xFFFF\)", andlo, insn, flags=re.I)
    insn = re.sub(r"%(hi|lo)\([^)]*\)", "0xHI", insn)
    insn = re.sub(r"%gp_rel\([^)]*\)", "SYM", insn)
    # objdump prints the gp-relative form as a bare offset (reloc is separate)
    insn = re.sub(r",-?\d+\(gp\)", ",SYM(gp)", insn)
    insn = re.sub(r"\bD_[0-9A-Fa-f]+\b", "SYM", insn)
    insn = re.sub(r"\bfunc_[0-9A-Fa-f]+\b", "SYM", insn)
    insn = insn.replace("$", "")
    def num(m):
        return str(int(m.group(0), 0))
    insn = re.sub(r"0x[0-9a-fA-F]+|\b\d+\b", num, insn)
    insn = re.sub(r"<[^>]*>", "", insn)  # objdump symbol annotation on targets
    insn = re.sub(r"\s*,\s*", ",", insn)
    insn = re.sub(r"\s+", " ", insn).strip()
    # branch targets: label name vs hex address — both become LBL
    if re.match(r"^(beq|bne|beqz|bnez|bgez|bgezal|bgtz|blez|bltz|bltzal)\b", insn):
        insn = re.sub(r",[^,]+$", ",LBL", insn)
    insn = re.sub(r"\.L\w+", "LBL", insn)
    # objdump prints the `li` alias; spimdisasm prints addiu with zero
    insn = re.sub(r"^li (\w+),(-?\d+)$", r"addiu \1,zero,\2", insn)
    return insn


def run_cc1(cc1, runner, flags, src, out_s):
    cmd_flags = flags.split()
    if runner == "docker":
        cmd = [
            DOCKER, "run", "--rm",
            "-v", f"{DECOMP}/tools/compilers/linux/gcc-2.6.3-psx:/cc:ro",
            "-v", f"{WORK}:/work",
            "ubuntu:25.10", str(cc1), *cmd_flags,
            "-o", f"/work/{out_s.name}", f"/work/{src.name}",
        ]
    else:
        cmd = [str(cc1), *cmd_flags, "-o", str(out_s), str(src)]
    return subprocess.run(cmd, capture_output=True, text=True)


def fix_flags_for_compiler(cc1, runner, flags):
    """Drop options the old cc1 rejects (e.g. -fgnu-linker on 2.6.3)."""
    probe_src = WORK / "probe.c"
    probe_src.write_text("int f(int x){return x+1;}\n")
    flags = flags.split()
    while True:
        r = run_cc1(cc1, runner, " ".join(flags), probe_src, WORK / "probe.s")
        if r.returncode == 0:
            return " ".join(flags)
        m = re.search(r"[Ii]nvalid option [`']([^`']+)'", r.stderr + r.stdout)
        if not m:
            print(f"  !! cannot run {cc1}: {r.stderr.strip()[:200]}")
            return None
        bad = m.group(1).lstrip("-")
        # cc1 reports e.g. `no-check-zero-division' for -mno-check-zero-division
        before = len(flags)
        flags = [f for f in flags
                 if f.lstrip("-") not in (bad, "m" + bad, bad.removeprefix("m"))]
        if len(flags) == before:
            print(f"  !! {cc1}: cannot drop rejected option '{bad}'")
            return None


def compile_and_score(cc1, runner, base_flags, opt, g, aspsx_ver, verbose):
    per_func = {}
    for cfile, func in CANDIDATES.items():
        src = HERE / "candidates" / cfile
        # cc1 2.x (the arm64 builds) crashes on comments; feed it preprocessed
        # input, exactly like the PE2 pipeline (cpp -> cc1 -> maspsx).
        pre = subprocess.run(
            ["clang", "-E", "-P", "-xc", str(src)], capture_output=True, text=True)
        if pre.returncode != 0:
            return None
        work_src = WORK / cfile.replace(".c", ".i")
        work_src.write_text(pre.stdout)
        out_s = WORK / (cfile.replace(".c", ".s"))
        r = run_cc1(cc1, runner, f"{opt} {g} {base_flags}", work_src, out_s)
        if r.returncode != 0:
            return None
        out_o = WORK / (cfile.replace(".c", ".o"))
        # NB: no --expand-div (see EXPAND_DIV note above); maspsx then emits
        # the trap-free div form the retail binary uses.
        r = subprocess.run(
            [sys.executable, str(MASPSX),
             f"--aspsx-version={aspsx_ver}", "--run-assembler",
             f"--gnu-as-path={GNU_AS}",
             "-EL", opt, "-march=r3000", "-mtune=r3000", "-no-pad-sections",
             g, "-o", str(out_o), str(out_s)],
            capture_output=True, text=True, stdin=subprocess.DEVNULL)
        if r.returncode != 0:
            if verbose:
                print("maspsx failed:", r.stderr[:300])
            return None
        r = subprocess.run([OBJDUMP, "-drz", str(out_o)], capture_output=True, text=True)
        got = []
        in_func = False
        for line in r.stdout.splitlines():
            if re.match(r"^[0-9a-f]+ <\w+>:", line):
                in_func = func in line or not got and "<" in line
                in_func = True  # single function per file
                continue
            mm = re.match(r"\s+[0-9a-f]+:\s+([0-9a-f]{8})\s+\t?(.*)", line)
            if mm and in_func:
                insn = mm.group(2)
                insn = insn.split("#")[0]  # strip comment
                got.append((mm.group(1).upper(), normalize(insn)))
        # objdump prints relocated hi/lo pairs as bare zeroes; mark them.
        hi_reg = None
        marked = []
        for word, insn in got:
            m = re.match(r"lui (\w+),0$", insn)
            if m:
                hi_reg = m.group(1)
                marked.append((word, f"lui {hi_reg},0xHI"))
                continue
            if hi_reg is not None:
                if insn == f"addiu {hi_reg},{hi_reg},0":
                    insn = f"addiu {hi_reg},{hi_reg},0xHI"
                else:
                    hi_reg = None
            marked.append((word, insn))
        got = marked
        orig = parse_original(func)
        # score: positional identity of normalized instructions + seq ratio.
        # Raw 32-bit words can never match on relocated instructions, so the
        # word list is only used for length.
        oi = [i for _, i in orig]
        gi = [i for _, i in got]
        exact = sum(1 for a, b in zip(oi, gi) if a == b and a != "")
        ratio = difflib.SequenceMatcher(None, oi, gi).ratio()
        per_func[func] = (len(orig), exact, ratio, oi, gi)
    return per_func


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--verbose", action="store_true")
    args = ap.parse_args()
    WORK.mkdir(exist_ok=True)

    results = []  # (compiler, opt, g, aspsx, tot_insns, tot_exact, avg_ratio, detail)
    for cname, (cc1, runner) in COMPILERS.items():
        if runner is None and not Path(cc1).exists():
            print(f"skip {cname}: {cc1} missing")
            continue
        fixed = fix_flags_for_compiler(cc1, runner, CC1_BASE)
        if fixed is None:
            continue
        if fixed != CC1_BASE:
            print(f"{cname}: flags reduced to: {fixed}")
        for opt in OPT_LEVELS:
            for g in G_LEVELS:
                if runner == "docker" and (opt, g) not in DOCKER_COMBOS:
                    continue
                # pick best ASPSX version per (compiler,opt,g); docker runs
                # only the reference version to keep container count sane
                versions = ["2.77"] if runner == "docker" else ASPSX_VERSIONS
                best = None
                for ver in versions:
                    res = compile_and_score(cc1, runner, fixed, opt, g, ver, args.verbose)
                    if res is None:
                        continue
                    tot = sum(n for n, _, _, _, _ in res.values())
                    exact = sum(e for _, e, _, _, _ in res.values())
                    avg = sum(r for _, _, r, _, _ in res.values()) / len(res)
                    if best is None or (exact, avg) > (best[1], best[2]):
                        best = (ver, exact, avg, tot, res)
                if best is None:
                    print(f"{cname} {opt} {g}: FAILED")
                    continue
                ver, exact, avg, tot, res = best
                results.append((cname, opt, g, ver, tot, exact, avg, res))
                print(f"{cname:14s} {opt} {g} aspsx={ver}: "
                      f"exact-words {exact}/{tot}  seq-sim {avg*100:.1f}%")

    print("\n=== ranked ===")
    for cname, opt, g, ver, tot, exact, avg, _ in sorted(
            results, key=lambda x: (-x[5], -x[6])):
        print(f"{exact:3d}/{tot:<3d} sim {avg*100:5.1f}%  {cname} {opt} {g} aspsx={ver}")

    if args.verbose and results:
        best = sorted(results, key=lambda x: (-x[5], -x[6]))[0]
        print(f"\n=== detail for best: {best[0]} {best[1]} {best[2]} aspsx={best[3]} ===")
        for func, (n, exact, ratio, orig, got) in best[7].items():
            if ratio < 1.0 or exact < n:
                print(f"\n--- {func} (exact {exact}/{n}, sim {ratio*100:.0f}%) ---")
                for line in difflib.unified_diff(orig, got, "original", "compiled",
                                                 lineterm=""):
                    print(line)


if __name__ == "__main__":
    main()
