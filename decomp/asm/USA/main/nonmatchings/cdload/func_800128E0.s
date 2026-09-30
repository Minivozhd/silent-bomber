nonmatching func_800128E0, 0x50

glabel func_800128E0
    /* 30E0 800128E0 E8FFBD27 */  addiu      $sp, $sp, -0x18
    /* 30E4 800128E4 801F053C */  lui        $a1, (0x1F800000 >> 16)
    /* 30E8 800128E8 FF078424 */  addiu      $a0, $a0, 0x7FF
    /* 30EC 800128EC AC03828F */  lw         $v0, %gp_rel(DATA_LBA)($gp)
    /* 30F0 800128F0 C2220400 */  srl        $a0, $a0, 11
    /* 30F4 800128F4 1000BFAF */  sw         $ra, 0x10($sp)
    /* 30F8 800128F8 8C2D020C */  jal        func_8008B630
    /* 30FC 800128FC 21204400 */   addu      $a0, $v0, $a0
    /* 3100 80012900 02000424 */  addiu      $a0, $zero, 0x2
    /* 3104 80012904 801F053C */  lui        $a1, (0x1F800000 >> 16)
    /* 3108 80012908 F42D020C */  jal        func_8008B7D0
    /* 310C 8001290C 21300000 */   addu      $a2, $zero, $zero
    /* 3110 80012910 15000424 */  addiu      $a0, $zero, 0x15
    /* 3114 80012914 801F053C */  lui        $a1, (0x1F800000 >> 16)
    /* 3118 80012918 F42D020C */  jal        func_8008B7D0
    /* 311C 8001291C 21300000 */   addu      $a2, $zero, $zero
    /* 3120 80012920 1000BF8F */  lw         $ra, 0x10($sp)
    /* 3124 80012924 00000000 */  nop
    /* 3128 80012928 0800E003 */  jr         $ra
    /* 312C 8001292C 1800BD27 */   addiu     $sp, $sp, 0x18
endlabel func_800128E0
