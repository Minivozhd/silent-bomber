typedef struct {
    char pad0[3];
    char f3;
    char pad4[3];
    char f7;
    char pad8[12];
    int f14;
} T;
void func_8005F088(T *p)
{
    p->f3 = 5;
    p->f7 = 0x48;
    p->f14 = 0x55555555;
}
