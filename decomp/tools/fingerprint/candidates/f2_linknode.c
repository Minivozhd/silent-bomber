/* Target: func_80034C90 — links a node, mixes sh/lw/sw stores.
 *
 *   addiu $v0, $zero, 1
 *   sh    $v0, 0x4($a1)
 *   lw    $v0, 0xC($a0)
 *   sw    $a2, 0xC($a1)
 *   sw    $a3, 0x8($a1)
 *   sw    $v0, 0x0($a1)
 *   jr    $ra
 *   sw    $a1, 0xC($a0)
 */
typedef struct Node {
    struct Node *prev; /* 0x0 */
    short pad4;        /* 0x4 */
    short flags;       /* 0x6 */
    struct Node *next; /* 0x8 */
    struct Node *head; /* 0xC */
} Node;

void func_80034C90(Node *a0, Node *a1, Node *a2, Node *a3)
{
    a1->pad4 = 1;
    a1->prev = a0->head;
    a1->head = a2;
    a1->next = a3;
    a0->head = a1;
}
