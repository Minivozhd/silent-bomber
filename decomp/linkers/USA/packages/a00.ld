SECTIONS
{
    HIDDEN(__romPos = 0);
    a00_ROM_START = __romPos;
    a00_VRAM = ADDR(.a00);
    .a00 0x800DD4F0 : AT(a00_ROM_START) SUBALIGN(4)
    {
        FILL(0x00000000);
        a00_RODATA_START = .;
        build/USA/asm/USA/packages/a00/948.s.o(.rodata);
        a00_RODATA_END = .;
        a00_RODATA_SIZE = ABSOLUTE(a00_RODATA_END - a00_RODATA_START);
        a00_TEXT_START = .;
        build/USA/asm/USA/packages/a00/948.s.o(.text);
        a00_TEXT_END = .;
        a00_TEXT_SIZE = ABSOLUTE(a00_TEXT_END - a00_TEXT_START);
        a00_DATA_START = .;
        build/USA/asm/USA/packages/a00/data/0.data.s.o(.data);
        build/USA/asm/USA/packages/a00/948.s.o(.data);
        build/USA/asm/USA/packages/a00/data/52F0.data.s.o(.data);
        a00_DATA_END = .;
        a00_DATA_SIZE = ABSOLUTE(a00_DATA_END - a00_DATA_START);
    }
    a00_bss_VRAM = ADDR(.a00_bss);
    .a00_bss (NOLOAD) : SUBALIGN(4)
    {
        FILL(0x00000000);
        a00_BSS_START = .;
        build/USA/asm/USA/packages/a00/948.s.o(.bss);
        a00_BSS_END = .;
        a00_BSS_SIZE = ABSOLUTE(a00_BSS_END - a00_BSS_START);
    }
    __romPos += SIZEOF(.a00);
    __romPos = ALIGN(__romPos, 4);
    a00_ROM_END = __romPos;
    a00_VRAM_END = .;

    /DISCARD/ :
    {
        *(*);
    }
}
