SECTIONS
{
    HIDDEN(__romPos = 0);
    demop_ROM_START = __romPos;
    demop_VRAM = ADDR(.demop);
    .demop 0x800DD4F0 : AT(demop_ROM_START) SUBALIGN(4)
    {
        FILL(0x00000000);
        demop_RODATA_START = .;
        build/USA/asm/USA/packages/demop/28.s.o(.rodata);
        demop_RODATA_END = .;
        demop_RODATA_SIZE = ABSOLUTE(demop_RODATA_END - demop_RODATA_START);
        demop_TEXT_START = .;
        build/USA/asm/USA/packages/demop/28.s.o(.text);
        demop_TEXT_END = .;
        demop_TEXT_SIZE = ABSOLUTE(demop_TEXT_END - demop_TEXT_START);
        demop_DATA_START = .;
        build/USA/asm/USA/packages/demop/data/0.data.s.o(.data);
        build/USA/asm/USA/packages/demop/28.s.o(.data);
        build/USA/asm/USA/packages/demop/data/1DE0.data.s.o(.data);
        demop_DATA_END = .;
        demop_DATA_SIZE = ABSOLUTE(demop_DATA_END - demop_DATA_START);
    }
    demop_bss_VRAM = ADDR(.demop_bss);
    .demop_bss (NOLOAD) : SUBALIGN(4)
    {
        FILL(0x00000000);
        demop_BSS_START = .;
        build/USA/asm/USA/packages/demop/28.s.o(.bss);
        demop_BSS_END = .;
        demop_BSS_SIZE = ABSOLUTE(demop_BSS_END - demop_BSS_START);
    }
    __romPos += SIZEOF(.demop);
    __romPos = ALIGN(__romPos, 4);
    demop_ROM_END = __romPos;
    demop_VRAM_END = .;

    /DISCARD/ :
    {
        *(*);
    }
}
