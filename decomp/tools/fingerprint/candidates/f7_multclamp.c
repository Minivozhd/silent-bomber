/* Target: func_8001AFB8 — u16 mult, shift, clamp.
 *
 *   lhu    $v1, %gp_rel(D_800B5C9C)($gp)
 *   lhu    $v0, 0x0($a0)
 *   nop
 *   mult   $v1, $v0
 *   mflo   $a1
 *   sra    $v0, $a1, 8
 *   sh     $v0, 0x0($a0)
 *   andi   $v0, $v0, 0xFFFF
 *   sltiu  $v0, $v0, 0x80
 *   bnez   $v0, .L
 *   addiu  $v0, $zero, 0x7F
 *   sh     $v0, 0x0($a0)
 * .L:
 *   jr     $ra
 *   nop
 */
unsigned short D_800B5C9C;

void func_8001AFB8(unsigned short *p)
{
    *p = (D_800B5C9C * *p) >> 8;
    if (*p >= 0x80)
        *p = 0x7F;
}
