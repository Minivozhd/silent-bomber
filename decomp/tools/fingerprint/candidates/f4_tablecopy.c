/* Target: func_8005E938 — copies a u16 from a table into *out.
 *
 *   lui   $v0, %hi(D_8009F8EC)
 *   addiu $v0, $v0, %lo(D_8009F8EC)
 *   sll   $a1, $a1, 1
 *   addu  $a1, $a1, $v0
 *   lhu   $v0, 0x0($a1)
 *   jr    $ra
 *   sh    $v0, 0x0($a0)
 */
extern unsigned short D_8009F8EC[];

void func_8005E938(short *out, int idx)
{
    *out = D_8009F8EC[idx];
}
