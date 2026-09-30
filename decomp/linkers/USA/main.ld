SECTIONS
{
    HIDDEN(__romPos = 0);
    _gp = 0x800B5838;
    header_ROM_START = __romPos;
    header_VRAM = ADDR(.header);
    .header : AT(header_ROM_START) SUBALIGN(2)
    {
        FILL(0x00000000);
        header_DATA_START = .;
        build/USA/asm/USA/main/header.s.o(.data);
        header_DATA_END = .;
        header_DATA_SIZE = ABSOLUTE(header_DATA_END - header_DATA_START);
    }
    __romPos += SIZEOF(.header);
    header_ROM_END = __romPos;
    header_VRAM_END = .;

    main_ROM_START = __romPos;
    main_VRAM = ADDR(.main);
    .main 0x80010000 : AT(main_ROM_START) SUBALIGN(2)
    {
        FILL(0x00000000);
        main_RODATA_START = .;
        build/USA/asm/USA/main/boot.s.o(.rodata);
        build/USA/asm/USA/main/bootsys.s.o(.rodata);
        build/USA/src/main/arena.c.o(.rodata);
        build/USA/src/main/cdload.c.o(.rodata);
        build/USA/src/main/cdutil.c.o(.rodata);
        build/USA/src/main/sbrle.c.o(.rodata);
        build/USA/asm/USA/main/pkgload.s.o(.rodata);
        build/USA/asm/USA/main/4F78.s.o(.rodata);
        build/USA/asm/USA/main/20074.s.o(.rodata);
        build/USA/asm/USA/main/psyq.s.o(.rodata);
        . = ALIGN(., 16);
        main_RODATA_END = .;
        main_RODATA_SIZE = ABSOLUTE(main_RODATA_END - main_RODATA_START);
        main_TEXT_START = .;
        build/USA/asm/USA/main/boot.s.o(.text);
        build/USA/asm/USA/main/bootsys.s.o(.text);
        build/USA/src/main/arena.c.o(.text);
        build/USA/src/main/cdload.c.o(.text);
        build/USA/src/main/cdutil.c.o(.text);
        build/USA/src/main/sbrle.c.o(.text);
        build/USA/asm/USA/main/pkgload.s.o(.text);
        build/USA/asm/USA/main/4F78.s.o(.text);
        build/USA/asm/USA/main/20074.s.o(.text);
        build/USA/asm/USA/main/psyq.s.o(.text);
        . = ALIGN(., 16);
        main_TEXT_END = .;
        main_TEXT_SIZE = ABSOLUTE(main_TEXT_END - main_TEXT_START);
        main_DATA_START = .;
        build/USA/asm/USA/main/boot.s.o(.data);
        build/USA/asm/USA/main/bootsys.s.o(.data);
        build/USA/src/main/arena.c.o(.data);
        build/USA/src/main/cdload.c.o(.data);
        build/USA/src/main/cdutil.c.o(.data);
        build/USA/src/main/sbrle.c.o(.data);
        build/USA/asm/USA/main/pkgload.s.o(.data);
        build/USA/asm/USA/main/4F78.s.o(.data);
        build/USA/asm/USA/main/20074.s.o(.data);
        build/USA/asm/USA/main/psyq.s.o(.data);
        build/USA/asm/USA/main/data/8A8F0.data.s.o(.data);
        . = ALIGN(., 16);
        main_DATA_END = .;
        main_DATA_SIZE = ABSOLUTE(main_DATA_END - main_DATA_START);
        main_BSS_START = .;
        build/USA/asm/USA/main/boot.s.o(.bss);
        build/USA/asm/USA/main/bootsys.s.o(.bss);
        build/USA/src/main/arena.c.o(.bss);
        build/USA/src/main/cdload.c.o(.bss);
        build/USA/src/main/cdutil.c.o(.bss);
        build/USA/src/main/sbrle.c.o(.bss);
        build/USA/asm/USA/main/pkgload.s.o(.bss);
        build/USA/asm/USA/main/4F78.s.o(.bss);
        build/USA/asm/USA/main/20074.s.o(.bss);
        build/USA/asm/USA/main/psyq.s.o(.bss);
        build/USA/asm/USA/main/data/bss.bss.s.o(.bss);
        . = ALIGN(., 16);
        main_BSS_END = .;
        main_BSS_SIZE = ABSOLUTE(main_BSS_END - main_BSS_START);
    }
    __romPos += SIZEOF(.main);
    __romPos = ALIGN(__romPos, 16);
    . = ALIGN(., 16);
    main_ROM_END = __romPos;
    main_VRAM_END = .;

    /DISCARD/ :
    {
        *(*);
    }
}
