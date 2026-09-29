/* Target: func_8005F088 — constant synthesis of 0x55555555 (lui/ori)
 * plus byte stores; int store sunk into the jr delay slot.
 *
 *   lui   $v1, 0x5555
 *   ori   $v1, $v1, 0x5555
 *   addiu $v0, $zero, 5
 *   sb    $v0, 0x3($a0)
 *   addiu $v0, $zero, 0x48
 *   sb    $v0, 0x7($a0)
 *   jr    $ra
 *   sw    $v1, 0x14($a0)
 */
typedef struct {
    char pad0[3];  /* 0x0 */
    char f3;       /* 0x3 */
    char pad4[3];  /* 0x4 */
    char f7;       /* 0x7 */
    char pad8[12]; /* 0x8 */
    int f14;       /* 0x14 */
} T;

void func_8005F088(T *p)
{
    p->f3 = 5;
    p->f7 = 0x48;
    p->f14 = 0x55555555;
}
