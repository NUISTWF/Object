/* 单向有头链表 */

#include <stdio.h>
#include <stdlib.h>

typedef struct ListNode{
    int data;
    struct ListNode* next;
} Node;

// 初始化头节点
Node* initList()
{
	Node* list = malloc(sizeof(*list));
    list->data = 0;        // 头节点里面保存链表元素个数
	list->next = NULL;
	return list;
}

// 头插法；如果是无头节点的链表插入数据就是需要二级指针，需要返回起始位置
void headInsert(Node* list, int data)
{   
    Node* node = malloc(sizeof(Node));
    node->data = data;
    node->next = list->next;    // 头插法（新节点的下一个指向头节点指向的下一个）
    list->next = node;          // 然后头节点再往前移动，指向新节点
    list->data++; 
}

// 尾插法
void tailinsert(Node* list, int data)
{
    Node* cur = list->next;
    Node* node = malloc(sizeof(Node));
    node->data = data;
    node->next = NULL;
    while (cur->next != NULL) {
        cur = cur->next;
    }
    cur->next = node;
    list->data++;
}

// 删除具有这个数据的节点（传进来的都是头节点）
int deleteNode(Node* list, int data)
{
    Node* cur = list->next;     // 指向第一个节点元素
    Node* pre = list;
    while (cur != NULL) {       // 判断当前节点只要不为空就进去比对一下数据
        if (cur->data == data) {
            pre->next = cur->next;
            free(cur);
            cur = NULL;
            break;
        }
        pre = cur;
        cur = cur->next;
    }
    list->data--;
}

// 改（和上面这个删除一样，找到一个节点替换掉数据即可）

// 查（和对上面一样循环遍历找到对应的index可以打印数据）

// 遍历打印
void printList(Node* list)
{
    Node* cur = list->next;
    while (cur != NULL) {
        printf("%d ", cur->data);
        cur = cur->next;
    }
    printf("\nnumber:%d\n", list->data);
}

// 删除链表
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
    tailinsert(list, 3);
    headInsert(list, 1);
    printList(list);
    deleteNode(list, 2);
    printList(list);
    deleteList(list);
    return 0;
}