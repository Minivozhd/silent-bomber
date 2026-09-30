nonmatching func_800127B4, 0x12C

glabel func_800127B4
    /* 2FB4 800127B4 88FFBD27 */  addiu      $sp, $sp, -0x78
    /* 2FB8 800127B8 0A80043C */  lui        $a0, %hi(aDataBin)
    /* 2FBC 800127BC C4A18424 */  addiu      $a0, $a0, %lo(aDataBin)
    /* 2FC0 800127C0 0B80023C */  lui        $v0, %hi(D_800B5B4C)
    /* 2FC4 800127C4 4C5B428C */  lw         $v0, %lo(D_800B5B4C)($v0)
    /* 2FC8 800127C8 1000A527 */  addiu      $a1, $sp, 0x10
    /* 2FCC 800127CC 7000B0AF */  sw         $s0, 0x70($sp)
    /* 2FD0 800127D0 7400BFAF */  sw         $ra, 0x74($sp)
    /* 2FD4 800127D4 08004234 */  ori        $v0, $v0, 0x8
    /* 2FD8 800127D8 0B80013C */  lui        $at, %hi(D_800B5B4C)
    /* 2FDC 800127DC 4C5B22AC */  sw         $v0, %lo(D_800B5B4C)($at)
    /* 2FE0 800127E0 9C49000C */  jal        func_80012670
    /* 2FE4 800127E4 5000B027 */   addiu     $s0, $sp, 0x50
    /* 2FE8 800127E8 21200002 */  addu       $a0, $s0, $zero
  .L800127EC:
    /* 2FEC 800127EC A421020C */  jal        func_80088690
    /* 2FF0 800127F0 1000A527 */   addiu     $a1, $sp, 0x10
    /* 2FF4 800127F4 05004014 */  bnez       $v0, .L8001280C
    /* 2FF8 800127F8 1000A427 */   addiu     $a0, $sp, 0x10
    /* 2FFC 800127FC 9A49000C */  jal        func_80012668
    /* 3000 80012800 21280000 */   addu      $a1, $zero, $zero
    /* 3004 80012804 FB490008 */  j          .L800127EC
    /* 3008 80012808 21200002 */   addu      $a0, $s0, $zero
  .L8001280C:
    /* 300C 8001280C 8421020C */  jal        func_80088610
    /* 3010 80012810 21200002 */   addu      $a0, $s0, $zero
    /* 3014 80012814 AC0382AF */  sw         $v0, %gp_rel(DATA_LBA)($gp)
    /* 3018 80012818 0B80043C */  lui        $a0, %hi(aStrBin)
    /* 301C 8001281C 58588424 */  addiu      $a0, $a0, %lo(aStrBin)
    /* 3020 80012820 9C49000C */  jal        func_80012670
    /* 3024 80012824 1000A527 */   addiu     $a1, $sp, 0x10
    /* 3028 80012828 21200002 */  addu       $a0, $s0, $zero
  .L8001282C:
    /* 302C 8001282C A421020C */  jal        func_80088690
    /* 3030 80012830 1000A527 */   addiu     $a1, $sp, 0x10
    /* 3034 80012834 05004014 */  bnez       $v0, .L8001284C
    /* 3038 80012838 1000A427 */   addiu     $a0, $sp, 0x10
    /* 303C 8001283C 9A49000C */  jal        func_80012668
    /* 3040 80012840 21280000 */   addu      $a1, $zero, $zero
    /* 3044 80012844 0B4A0008 */  j          .L8001282C
    /* 3048 80012848 21200002 */   addu      $a0, $s0, $zero
  .L8001284C:
    /* 304C 8001284C 8421020C */  jal        func_80088610
    /* 3050 80012850 21200002 */   addu      $a0, $s0, $zero
    /* 3054 80012854 B00382AF */  sw         $v0, %gp_rel(STR_LBA)($gp)
    /* 3058 80012858 0B80043C */  lui        $a0, %hi(aXaBin)
    /* 305C 8001285C 60588424 */  addiu      $a0, $a0, %lo(aXaBin)
    /* 3060 80012860 9C49000C */  jal        func_80012670
    /* 3064 80012864 1000A527 */   addiu     $a1, $sp, 0x10
    /* 3068 80012868 21200002 */  addu       $a0, $s0, $zero
  .L8001286C:
    /* 306C 8001286C A421020C */  jal        func_80088690
    /* 3070 80012870 1000A527 */   addiu     $a1, $sp, 0x10
    /* 3074 80012874 05004014 */  bnez       $v0, .L8001288C
    /* 3078 80012878 1000A427 */   addiu     $a0, $sp, 0x10
    /* 307C 8001287C 9A49000C */  jal        func_80012668
    /* 3080 80012880 21280000 */   addu      $a1, $zero, $zero
    /* 3084 80012884 1B4A0008 */  j          .L8001286C
    /* 3088 80012888 21200002 */   addu      $a0, $s0, $zero
  .L8001288C:
    /* 308C 8001288C 8421020C */  jal        func_80088610
    /* 3090 80012890 21200002 */   addu      $a0, $s0, $zero
    /* 3094 80012894 B40382AF */  sw         $v0, %gp_rel(XA_LBA)($gp)
    /* 3098 80012898 80000224 */  addiu      $v0, $zero, 0x80
    /* 309C 8001289C 6800A2A3 */  sb         $v0, 0x68($sp)
    /* 30A0 800128A0 0E000424 */  addiu      $a0, $zero, 0xE
  .L800128A4:
    /* 30A4 800128A4 6800A527 */  addiu      $a1, $sp, 0x68
    /* 30A8 800128A8 F42D020C */  jal        func_8008B7D0
    /* 30AC 800128AC 21300000 */   addu      $a2, $zero, $zero
    /* 30B0 800128B0 FCFF4010 */  beqz       $v0, .L800128A4
    /* 30B4 800128B4 0E000424 */   addiu     $a0, $zero, 0xE
    /* 30B8 800128B8 F7FF0324 */  addiu      $v1, $zero, -0x9
    /* 30BC 800128BC 0B80023C */  lui        $v0, %hi(D_800B5B4C)
    /* 30C0 800128C0 4C5B428C */  lw         $v0, %lo(D_800B5B4C)($v0)
    /* 30C4 800128C4 7400BF8F */  lw         $ra, 0x74($sp)
    /* 30C8 800128C8 7000B08F */  lw         $s0, 0x70($sp)
    /* 30CC 800128CC 24104300 */  and        $v0, $v0, $v1
    /* 30D0 800128D0 0B80013C */  lui        $at, %hi(D_800B5B4C)
    /* 30D4 800128D4 4C5B22AC */  sw         $v0, %lo(D_800B5B4C)($at)
    /* 30D8 800128D8 0800E003 */  jr         $ra
    /* 30DC 800128DC 7800BD27 */   addiu     $sp, $sp, 0x78
endlabel func_800127B4
