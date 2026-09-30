#include "common.h"

extern s32 D_800B5BE0; // gp-relative CD-layer state

s32 func_8008C47C(s32 arg0, s32 arg1);
void func_80012A34(u32* block);

// MATCHED (byte-identical): stores func_8008C47C(1, 0) into the CD-layer
// state word.
void func_80012A10(void) {
    D_800B5BE0 = func_8008C47C(1, 0);
}

INCLUDE_ASM("asm/USA/main/nonmatchings/cdutil", func_80012A34);

// MATCHED (byte-identical): walks an SB-RLE stream's [skip][block]...
// records and applies func_80012A34 to each block header (no decompression).
void func_80012AA0(u32* stream) {
    u32* p = stream;
    u32 skip;

    while ((skip = *p++) != 0) {
        func_80012A34(p);
        p += skip;
    }
}
