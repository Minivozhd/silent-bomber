#include "common.h"

extern s32 DATA_LBA; // gp-relative (vram 0x800B5BE4), resolved at boot by func_800127B4

void func_8008B630(s32 lba, u8* buf);  // CD read wrapper (PsyQ libcd)
void func_8008B7D0(s32 cmd, u8* buf, s32 arg); // CD sync wrapper (PsyQ libcd)

INCLUDE_ASM("asm/USA/main/nonmatchings/cdload", func_800127B4);

// MATCHED (byte-identical): reads the CD sector covering file offset `off`
// of DATA.BIN into the scratchpad (0x1F800000) and waits for the two
// completion events.
void func_800128E0(u32 off) {
    func_8008B630(DATA_LBA + ((off + 0x7FF) >> 11), (u8*)0x1F800000);
    func_8008B7D0(2, (u8*)0x1F800000, 0);
    func_8008B7D0(0x15, (u8*)0x1F800000, 0);
}

INCLUDE_ASM("asm/USA/main/nonmatchings/cdload", func_80012930);
