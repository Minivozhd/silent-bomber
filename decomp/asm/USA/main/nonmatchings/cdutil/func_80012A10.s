nonmatching func_80012A10, 0x24

glabel func_80012A10
    /* 3210 80012A10 E8FFBD27 */  addiu      $sp, $sp, -0x18
    /* 3214 80012A14 01000424 */  addiu      $a0, $zero, 0x1
    /* 3218 80012A18 1000BFAF */  sw         $ra, 0x10($sp)
    /* 321C 80012A1C 1F31020C */  jal        func_8008C47C
    /* 3220 80012A20 21280000 */   addu      $a1, $zero, $zero
    /* 3224 80012A24 1000BF8F */  lw         $ra, 0x10($sp)
    /* 3228 80012A28 A80382AF */  sw         $v0, %gp_rel(D_800B5BE0)($gp)
    /* 322C 80012A2C 0800E003 */  jr         $ra
    /* 3230 80012A30 1800BD27 */   addiu     $sp, $sp, 0x18
endlabel func_80012A10
