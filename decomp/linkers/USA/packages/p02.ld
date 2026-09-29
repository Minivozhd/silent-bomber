SECTIONS
{
    HIDDEN(__romPos = 0);
    p02_ROM_START = __romPos;
    p02_VRAM = ADDR(.p02);
    .p02 0x800DD4F0 : AT(p02_ROM_START) SUBALIGN(4)
    {
        FILL(0x00000000);
        p02_RODATA_START = .;
        build/USA/asm/USA/packages/p02/32B0.s.o(.rodata);
        p02_RODATA_END = .;
        p02_RODATA_SIZE = ABSOLUTE(p02_RODATA_END - p02_RODATA_START);
        p02_TEXT_START = .;
        build/USA/asm/USA/packages/p02/32B0.s.o(.text);
        p02_TEXT_END = .;
        p02_TEXT_SIZE = ABSOLUTE(p02_TEXT_END - p02_TEXT_START);
        p02_DATA_START = .;
        build/USA/asm/USA/packages/p02/data/0.data.s.o(.data);
        build/USA/asm/USA/packages/p02/32B0.s.o(.data);
        build/USA/asm/USA/packages/p02/data/139E0.data.s.o(.data);
        p02_DATA_END = .;
        p02_DATA_SIZE = ABSOLUTE(p02_DATA_END - p02_DATA_START);
    }
    p02_bss_VRAM = ADDR(.p02_bss);
    .p02_bss (NOLOAD) : SUBALIGN(4)
    {
        FILL(0x00000000);
        p02_BSS_START = .;
        build/USA/asm/USA/packages/p02/32B0.s.o(.bss);
        p02_BSS_END = .;
        p02_BSS_SIZE = ABSOLUTE(p02_BSS_END - p02_BSS_START);
    }
    __romPos += SIZEOF(.p02);
    __romPos = ALIGN(__romPos, 4);
    p02_ROM_END = __romPos;
    p02_VRAM_END = .;

    /DISCARD/ :
    {
        *(*);
    }
}
