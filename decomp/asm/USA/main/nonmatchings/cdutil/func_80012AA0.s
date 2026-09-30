nonmatching func_80012AA0, 0x4C

glabel func_80012AA0
    /* 32A0 80012AA0 E0FFBD27 */  addiu      $sp, $sp, -0x20
    /* 32A4 80012AA4 1000B0AF */  sw         $s0, 0x10($sp)
    /* 32A8 80012AA8 21808000 */  addu       $s0, $a0, $zero
    /* 32AC 80012AAC 1800BFAF */  sw         $ra, 0x18($sp)
    /* 32B0 80012AB0 B24A0008 */  j          .L80012AC8
    /* 32B4 80012AB4 1400B1AF */   sw        $s1, 0x14($sp)
  .L80012AB8:
    /* 32B8 80012AB8 8D4A000C */  jal        func_80012A34
    /* 32BC 80012ABC 21200002 */   addu      $a0, $s0, $zero
    /* 32C0 80012AC0 80101100 */  sll        $v0, $s1, 2
    /* 32C4 80012AC4 21800202 */  addu       $s0, $s0, $v0
  .L80012AC8:
    /* 32C8 80012AC8 0000118E */  lw         $s1, 0x0($s0)
    /* 32CC 80012ACC 00000000 */  nop
    /* 32D0 80012AD0 F9FF2016 */  bnez       $s1, .L80012AB8
    /* 32D4 80012AD4 04001026 */   addiu     $s0, $s0, 0x4
    /* 32D8 80012AD8 1800BF8F */  lw         $ra, 0x18($sp)
    /* 32DC 80012ADC 1400B18F */  lw         $s1, 0x14($sp)
    /* 32E0 80012AE0 1000B08F */  lw         $s0, 0x10($sp)
    /* 32E4 80012AE4 0800E003 */  jr         $ra
    /* 32E8 80012AE8 2000BD27 */   addiu     $sp, $sp, 0x20
endlabel func_80012AA0
