SECTIONS
{
    HIDDEN(__romPos = 0);
    p01_ROM_START = __romPos;
    p01_VRAM = ADDR(.p01);
    .p01 0x800DD4F0 : AT(p01_ROM_START) SUBALIGN(4)
    {
        FILL(0x00000000);
        p01_RODATA_START = .;
        build/USA/asm/USA/packages/p01/6C34.s.o(.rodata);
        p01_RODATA_END = .;
        p01_RODATA_SIZE = ABSOLUTE(p01_RODATA_END - p01_RODATA_START);
        p01_TEXT_START = .;
        build/USA/asm/USA/packages/p01/6C34.s.o(.text);
        p01_TEXT_END = .;
        p01_TEXT_SIZE = ABSOLUTE(p01_TEXT_END - p01_TEXT_START);
        p01_DATA_START = .;
        build/USA/asm/USA/packages/p01/data/0.data.s.o(.data);
        build/USA/asm/USA/packages/p01/6C34.s.o(.data);
        build/USA/asm/USA/packages/p01/data/1ACC0.data.s.o(.data);
        p01_DATA_END = .;
        p01_DATA_SIZE = ABSOLUTE(p01_DATA_END - p01_DATA_START);
    }
    p01_bss_VRAM = ADDR(.p01_bss);
    .p01_bss (NOLOAD) : SUBALIGN(4)
    {
        FILL(0x00000000);
        p01_BSS_START = .;
        build/USA/asm/USA/packages/p01/6C34.s.o(.bss);
        p01_BSS_END = .;
        p01_BSS_SIZE = ABSOLUTE(p01_BSS_END - p01_BSS_START);
    }
    __romPos += SIZEOF(.p01);
    __romPos = ALIGN(__romPos, 4);
    p01_ROM_END = __romPos;
    p01_VRAM_END = .;

    /DISCARD/ :
    {
        *(*);
    }
}
