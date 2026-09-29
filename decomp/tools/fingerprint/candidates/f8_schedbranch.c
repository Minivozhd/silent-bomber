/* Target: func_80035330 — magic-free mult, branch delay-slot store.
 *
 *   lw    $v0, 0x8($a2)
 *   lw    $v1, 0xC($a2)
 *   sra   $v0, $v0, 4
 *   mult  $v1, $v0
 *   lw    $v0, 0x24($a1)
 *   mflo  $a0
 *   sra   $v1, $a0, 8
 *   subu  $v0, $v0, $v1
 *   bgez  $v0, .L1
 *   sw    $v0, 0x24($a1)     <- branch delay slot
 *   sw    $zero, 0x24($a1)
 * .L1:
 *   lw    $v0, 0x24($a1)
 *   nop
 *   beqz  $v0, .L2
 *   addiu $v0, $zero, 0xC
 *   sh    $v0, 0x30($a1)
 * .L2:
 *   lw    $v0, 0x24($a1)
 *   jr    $ra
 *   nop
 */
typedef struct {
    char pad[0x24];  /* 0x0 */
    int f24;         /* 0x24 */
    char pad2[0x30 - 0x28];
    short f30;       /* 0x30 */
} A;

typedef struct {
    char pad[8];     /* 0x0 */
    int f8;          /* 0x8 */
    int fC;          /* 0xC */
} B;

int func_80035330(int a0, A *a1, B *a2)
{
    a1->f24 -= (a2->fC * (a2->f8 >> 4)) >> 8;
    if (a1->f24 < 0)
        a1->f24 = 0;
    if (a1->f24 != 0)
        a1->f30 = 0xC;
    return a1->f24;
}
