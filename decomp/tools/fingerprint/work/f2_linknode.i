typedef struct Node {
    struct Node *prev;
    short pad4;
    short flags;
    struct Node *next;
    struct Node *head;
} Node;
void func_80034C90(Node *a0, Node *a1, Node *a2, Node *a3)
{
    a1->pad4 = 1;
    a1->prev = a0->head;
    a1->head = a2;
    a1->next = a3;
    a0->head = a1;
}
