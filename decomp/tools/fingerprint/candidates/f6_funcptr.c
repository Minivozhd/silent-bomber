/* Target: func_8005BAA0 — stores a constant and a function pointer.
 *
 *   addiu $v0, $zero, 0xB
 *   sh    $v0, 0x3994($a0)
 *   lui   $v0, %hi(func_8005BFFC)
 *   addiu $v0, $v0, %lo(func_8005BFFC)
 *   jr    $ra
 *   sw    $v0, 0x58($a0)
 */
typedef struct {
    char pad0[0x58];   /* 0x0 */
    void (*fn)(void *); /* 0x58 */
    char pad5C[0x3994 - 0x5C];
    short f3994;        /* 0x3994 */
} T;

void func_8005BFFC(void *p);

void func_8005BAA0(T *p)
{
    p->f3994 = 0xB;
    p->fn = func_8005BFFC;
}
