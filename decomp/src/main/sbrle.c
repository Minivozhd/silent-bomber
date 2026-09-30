#include "common.h"

// WIP (~92% similar): SB-RLE stream driver, draft below in #if 0.
// Remaining: gcc 2.8.1's scheduler sinks the `end`/`blocks` initializations
// past the initial branch (delay slot fill differs from the original).
INCLUDE_ASM("asm/USA/main/nonmatchings/sbrle", func_80012AEC);

// WIP (~91% similar): SB-RLE block decoder, draft below in #if 0.
// Structure fully matches; remaining diffs are register allocation of the
// mode/esc header values (sched1 reorders the two lbu's, then local-alloc
// assigns esc->v1/mode->v0 instead of mode->v1/esc->a2) plus one temp.
INCLUDE_ASM("asm/USA/main/nonmatchings/sbrle", func_80012BB0);

#if 0
void func_80012A34(u8* dst);
void func_80012BB0(u8* block, u8* dst);
void func_80016538(s32 arg);

// SB-RLE stream driver (research/data_bin.md §2.1).
// Walks [skip][block][skip][block]... records; decompresses each block at
// the running output pointer. Every 16 blocks (or when the output pointer
// leaves [dst, dstEnd)) it rewinds the pointer and calls func_80016538(1)
// (loader throttle / vsync wait).
void func_80012AEC(u32* stream, u8* dst, u8* dstEnd) {
    u32* p = stream;
    u32 skip;
    u32 dsize;
    u32 blocks;
    u8* base;
    u8* end;
    u8* out;

    base = dst;
    end = dstEnd;
    out = base;
    blocks = 0;
    skip = *p;
    if (skip == 0) {
        return;
    }
    p++;
    do {
        if (blocks++ < 16) {
            if (out < end) {
                goto decode;
            }
        }
        blocks = 0;
        out = base;
        func_80016538(1);
decode:
        func_80012BB0((u8*)p, out);
        func_80012A34(out);
        dsize = p[1];
        p += skip;
        skip = *p++;
        out += dsize;
    } while (skip != 0);
}

// SB-RLE block decoder (research/data_bin.md §2.2/2.3).
// block: [csize:4][dsize:4][esc:1][mode:1][payload...]
// mode == 0: nibble RLE (MSB-first), else byte RLE. `esc` introduces a
// (count, value) run; a zero count emits nothing.
void func_80012BB0(u8* block, u8* dst) {
    s32 inPhase = 0;
    s32 outPhase = inPhase;
    u32 csize = *(u32*)block;
    u32 mode = block[9];
    u8* end = block + csize;
    u32 esc = block[8];
    u32 nib;
    u32 cnt;
    u32 val;
    u32 valHi;
    u32 b;

    block += 0xA;
    if (mode == 0) {
        u32 escm = esc & 0xFF;
        if (block >= end) {
            return;
        }
        for (;;) {
            if (inPhase == 0) {
                nib = *block >> 4;
            } else {
                nib = *block;
                nib = nib & 0xF;
                block++;
                if (nib == escm && block >= end) {
                    return;
                }
            }
            inPhase = !inPhase;
            if (nib != escm) {
                if (outPhase == 0) {
                    *dst = nib << 4;
                } else {
                    *dst |= nib & 0xF;
                    dst++;
                }
                outPhase = !outPhase;
            } else {
                if (inPhase == 0) {
                    b = *block;
                    block++;
                    cnt = b >> 4;
                    val = b & 0xF;
                } else {
                    b = *block;
                    block++;
                    cnt = b & 0xF;
                    val = *block >> 4;
                }
                if ((cnt & 0xFF) != 0) {
                    valHi = val << 4;
                    val = val & 0xF;
                    do {
                        if (outPhase == 0) {
                            *dst = valHi;
                        } else {
                            *dst |= val;
                            dst++;
                        }
                        cnt--;
                        outPhase ^= 1;
                    } while ((cnt & 0xFF) != 0);
                }
            }
            if (block >= end) {
                break;
            }
        }
        return;
    }

    if (block >= end) {
        return;
    }
    {
        u32 escm = esc & 0xFF;
        do {
            nib = *block++;
            if (nib != escm) {
                *dst = nib;
                dst++;
            } else {
                cnt = *block++;
                val = *block++;
                if (cnt != 0) {
                    do {
                        *dst = val;
                        cnt--;
                        dst++;
                    } while ((cnt & 0xFF) != 0);
                }
            }
        } while (block < end);
    }
}
#endif
