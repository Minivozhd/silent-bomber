nonmatching func_80012670, 0x4C

glabel func_80012670
    /* 2E70 80012670 5C000224 */  addiu      $v0, $zero, 0x5C
    /* 2E74 80012674 0000A2A0 */  sb         $v0, 0x0($a1)
    /* 2E78 80012678 0100A524 */  addiu      $a1, $a1, 0x1
  .L8001267C:
    /* 2E7C 8001267C 00008290 */  lbu        $v0, 0x0($a0)
    /* 2E80 80012680 01008424 */  addiu      $a0, $a0, 0x1
    /* 2E84 80012684 0000A2A0 */  sb         $v0, 0x0($a1)
    /* 2E88 80012688 FF004230 */  andi       $v0, $v0, 0xFF
    /* 2E8C 8001268C FBFF4014 */  bnez       $v0, .L8001267C
    /* 2E90 80012690 0100A524 */   addiu     $a1, $a1, 0x1
    /* 2E94 80012694 FFFFA524 */  addiu      $a1, $a1, -0x1
    /* 2E98 80012698 3B000224 */  addiu      $v0, $zero, 0x3B
    /* 2E9C 8001269C 0000A2A0 */  sb         $v0, 0x0($a1)
    /* 2EA0 800126A0 0100A524 */  addiu      $a1, $a1, 0x1
    /* 2EA4 800126A4 31000224 */  addiu      $v0, $zero, 0x31
    /* 2EA8 800126A8 0000A2A0 */  sb         $v0, 0x0($a1)
    /* 2EAC 800126AC 0100A524 */  addiu      $a1, $a1, 0x1
    /* 2EB0 800126B0 0100A224 */  addiu      $v0, $a1, 0x1
    /* 2EB4 800126B4 0800E003 */  jr         $ra
    /* 2EB8 800126B8 0000A0A0 */   sb        $zero, 0x0($a1)
endlabel func_80012670
