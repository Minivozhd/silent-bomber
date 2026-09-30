#include "common.h"

// WIP: returns the package arena base (0x800DD4F0). gcc 2.8.1 -O2 always
// fills the return delay slot with the %lo addiu; the original has it
// unfilled (lui/addiu/jr/nop) — likely a handwritten asm stub. Kept as asm.
INCLUDE_ASM("asm/USA/main/nonmatchings/arena", func_8001243C);

INCLUDE_ASM("asm/USA/main/nonmatchings/arena", func_8001244C);

INCLUDE_ASM("asm/USA/main/nonmatchings/arena", func_80012478);

INCLUDE_ASM("asm/USA/main/nonmatchings/arena", func_80012550);

INCLUDE_ASM("asm/USA/main/nonmatchings/arena", func_800125A0);

INCLUDE_ASM("asm/USA/main/nonmatchings/arena", func_800125DC);

void func_80012668(void) {
}

INCLUDE_ASM("asm/USA/main/nonmatchings/arena", func_80012670);

INCLUDE_ASM("asm/USA/main/nonmatchings/arena", func_800126BC);

INCLUDE_ASM("asm/USA/main/nonmatchings/arena", func_800127A4);

void func_800127AC(void) {
}
