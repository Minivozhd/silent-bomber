/* Target: func_80055E78 — zeroes five fields in this exact C order
 * (gcc emits them in order and sinks the last store into the delay slot).
 *
 *   sw    $zero, 0x0($a0)
 *   sh    $zero, 0xA($a0)
 *   sh    $zero, 0x8($a0)
 *   sh    $zero, 0x6($a0)
 *   jr    $ra
 *   sh    $zero, 0x4($a0)
 */
typedef struct {
    int f0;    /* 0x0 */
    short f4;  /* 0x4 */
    short f6;  /* 0x6 */
    short f8;  /* 0x8 */
    short fA;  /* 0xA */
} T;

void func_80055E78(T *p)
{
    p->f0 = 0;
    p->fA = 0;
    p->f8 = 0;
    p->f6 = 0;
    p->f4 = 0;
}
