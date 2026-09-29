/* Target: func_8005D558 — variable div + magic-multiply constant div.
 *
 *   lui   $a2, 0xA0
 *   div   $zero, $a2, $a1
 *   mflo  $a2
 *   lui   $v0, 0x6666
 *   ori   $v0, $v0, 0x6667
 *   sll   $v1, $a1, 12
 *   mult  $v1, $v0
 *   addiu $a1, $a1, 0x300
 *   sra   $v1, $v1, 31
 *   sh    $a1, 0x22A($a0)
 *   mfhi  $v0
 *   sra   $v0, $v0, 10
 *   subu  $v0, $v0, $v1
 *   sh    $v0, 0x226($a0)
 *   jr    $ra
 *   sh    $a2, 0x228($a0)
 */
typedef struct {
    char pad[0x226]; /* 0x0 */
    short f226;      /* 0x226 */
    short f228;      /* 0x228 */
    short f22A;      /* 0x22A */
} T;

void func_8005D558(T *p, int a1)
{
    p->f22A = a1 + 0x300;
    p->f226 = (a1 << 12) / 2560;
    p->f228 = 0xA00000 / a1;
}
