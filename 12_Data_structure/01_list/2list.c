/* 双向有头链表 */

#include <stdio.h>
#include <stdlib.h>

typedef struct ListNode{
    int data;
    struct ListNode* prev;
    struct ListNode* next;
} Node;

Node* initList()
{
	Node* list = malloc(sizeof(*list));
    list->data = 0;
    list->prev = NULL;
	list->next = NULL;
	return list;
}

// 头插要注意只有头节点没有后继节点的时候，也就是只有一个头节点的时候，list->next->prev会错误
void headInsert(Node* list, int data)
{
    Node* node = malloc(sizeof(Node));
    node->data = data;
    node->next = list->next;
    if (list->next)
        list->next->prev = node;
    node->prev = list;
    list->next = node;
    list->data++;
}

void tailInsert(Node* list, int data)
{
    Node* cur = list->next;
    Node* node = malloc(sizeof(Node));
    while (cur->next) {
        cur = cur->next;
    }
    node->data = data;
    node->next = NULL;
    node->prev = cur;
    cur->next = node;
    list->data++;
}

void deleteNode(Node* list, int data)
{
    Node* cur = list->next;
    Node* pre = list;
    while (cur != NULL) {
        if (cur->data == data) {
            pre->next = cur->next;
            cur->next->prev = pre;
            free(cur);
            cur = NULL;
            break;
        }
        pre = cur;
        cur = cur->next;
    }
    list->data--;
}

void printList(Node* list)
{
    Node* cur = list->next;
    while (cur) {
        printf("%d ", cur->data);
        cur = cur->next;
    }
    printf("\nnumber:%d\n", list->data);
}

void deleteList(Node* list)
{
    Node* cur = list;
    Node* tmp = NULL;
    while (cur) {
        tmp = cur->next;
        free(cur);
        cur = tmp;
    }
}

int main()
{
    Node* list = initList();
    headInsert(list, 2);
    tailInsert(list, 3);
    headInsert(list, 1);
    printList(list);
    deleteNode(list, 2);
    printList(list);
    deleteList(list);
    return 0;
}