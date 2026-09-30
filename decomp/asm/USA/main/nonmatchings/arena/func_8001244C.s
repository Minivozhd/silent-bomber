nonmatching func_8001244C, 0x2C

glabel func_8001244C
    /* 2C4C 8001244C 0C000224 */  addiu      $v0, $zero, 0xC
    /* 2C50 80012450 7C0384AF */  sw         $a0, %gp_rel(D_800B5BB4)($gp)
    /* 2C54 80012454 040080AC */  sw         $zero, 0x4($a0)
    /* 2C58 80012458 000080AC */  sw         $zero, 0x0($a0)
    /* 2C5C 8001245C 080082AC */  sw         $v0, 0x8($a0)
    /* 2C60 80012460 21208200 */  addu       $a0, $a0, $v0
    /* 2C64 80012464 780385AF */  sw         $a1, %gp_rel(D_800B5BB0)($gp)
    /* 2C68 80012468 840384AF */  sw         $a0, %gp_rel(D_800B5BBC)($gp)
    /* 2C6C 8001246C 8C0382AF */  sw         $v0, %gp_rel(D_800B5BC4)($gp)
    /* 2C70 80012470 0800E003 */  jr         $ra
    /* 2C74 80012474 00000000 */   nop
endlabel func_8001244C
