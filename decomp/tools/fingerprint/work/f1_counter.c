/* Target: func_80010364 — increments a gp-relative u16 counter.
 *
 *   lhu   $v0, %gp_rel(D_800B5B20)($gp)
 *   nop
 *   addiu $v0, $v0, 1
 *   sh    $v0, %gp_rel(D_800B5B20)($gp)
 *   jr    $ra
 *   nop
 */
/* Defined (not extern) so -G8 places it in .sbss and access goes through $gp,
 * matching the original's %gp_rel form. */
unsigned short D_800B5B20;

void func_80010364(void)
{
    D_800B5B20++;
}
