/* 双向有头循环链表 */

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
    list->prev = list;
	list->next = list;
	return list;
}

void headInsert(Node* list, int data)
{
    Node* node = malloc(sizeof(Node));
    node->data = data;
    node->next = list->next;
    if (list->next != list)
        list->next->prev = node;
    else
        list->prev = node;
    node->prev = list;
    list->next = node;
    list->data++;
}

void tailInsert(Node* list, int data)
{
    Node* cur = list->next;
    Node* node = malloc(sizeof(Node));
    node->data = data;
    while (cur->next != list) {
        cur = cur->next;
    }
    cur->next = node;
    node->next = list;
    node->prev = cur;
    list->prev = node;
    list->data++;
}

void deleteNode(Node* list, int data)
{
    Node* cur = list->next;
    Node* pre = list;
    while (cur->next != list) {
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
    while (cur != list) {
        printf("%d ", cur->data);
        cur = cur->next;
    }
    printf("\nnumber:%d\n", list->data);
}

void deleteList(Node* list)
{
    Node* cur = list->next;
    Node* tmp = NULL;
    while (cur != list) {
        tmp = cur->next;
        free(cur);
        cur = tmp;
    }
    free(list);
}

int main()
{
    Node* list = initList();
	headInsert(list, 2);
	tailInsert(list, 3);
	headInsert(list, 1);
	tailInsert(list, 4);
	tailInsert(list, 5);
	printList(list);
	deleteNode(list, 3);
	printList(list);
	deleteList(list);
    return 0;
}