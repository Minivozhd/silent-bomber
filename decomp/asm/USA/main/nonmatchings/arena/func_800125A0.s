nonmatching func_800125A0, 0x3C

glabel func_800125A0
    /* 2DA0 800125A0 F4FF8424 */  addiu      $a0, $a0, -0xC
    /* 2DA4 800125A4 0800858C */  lw         $a1, 0x8($a0)
    /* 2DA8 800125A8 00000000 */  nop
    /* 2DAC 800125AC FCFFA228 */  slti       $v0, $a1, -0x4
    /* 2DB0 800125B0 03004010 */  beqz       $v0, .L800125C0
    /* 2DB4 800125B4 FCFF0324 */   addiu     $v1, $zero, -0x4
    /* 2DB8 800125B8 0800E003 */  jr         $ra
    /* 2DBC 800125BC 21100000 */   addu      $v0, $zero, $zero
  .L800125C0:
    /* 2DC0 800125C0 080083AC */  sw         $v1, 0x8($a0)
    /* 2DC4 800125C4 8C03838F */  lw         $v1, %gp_rel(D_800B5BC4)($gp)
    /* 2DC8 800125C8 0400A424 */  addiu      $a0, $a1, 0x4
    /* 2DCC 800125CC 23186400 */  subu       $v1, $v1, $a0
    /* 2DD0 800125D0 8C0383AF */  sw         $v1, %gp_rel(D_800B5BC4)($gp)
    /* 2DD4 800125D4 0800E003 */  jr         $ra
    /* 2DD8 800125D8 01000224 */   addiu     $v0, $zero, 0x1
endlabel func_800125A0
