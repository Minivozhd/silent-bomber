typedef struct {
    char pad0[0x58];
    void (*fn)(void *);
    char pad5C[0x3994 - 0x5C];
    short f3994;
} T;
void func_8005BFFC(void *p);
void func_8005BAA0(T *p)
{
    p->f3994 = 0xB;
    p->fn = func_8005BFFC;
}
