/* 栈就是头插法链表 */

#include <stdio.h>
#include <stdlib.h>

typedef struct StackNode{
    int data;
    struct StackNode* next;
} Node;

Node* initStack()
{
	Node* stack = malloc(sizeof(*stack));
    stack->data = 0;
	stack->next = NULL;
	return stack;
}

void push(Node* stack, int data)
{   
    Node* node = malloc(sizeof(Node));
    node->data = data;
    node->next = stack->next;
    stack->next = node;
    stack->data++; 
}

int popNode(Node* stack)
{   
    if (stack->data == 0 || stack->next == NULL)
        return -1;
    Node* cur = stack->next;
    int data = cur->data;
    printf("popNode data: %d\n", data);
    stack->next = cur->next;
    free(cur);
    cur = NULL;
    stack->data--;
}

int pop(Node* stack)
{
    if (stack->data == 0 || stack->next == NULL)
        return -1;
    Node* cur = stack->next;
    Node* tmp = NULL;
    printf("pop data: ");
    while (cur != NULL) {
        tmp = cur->next;
        printf("%d ", cur->data);
        free(cur);
        cur = tmp;
        stack->data--;
    }
    stack->next = NULL;
}

void printStack(Node* stack)
{
    Node* cur = stack;
    while (cur->next) {
        cur = cur->next;
        printf("%d\n", cur->data);
    }
    printf("number:%d\n", stack->data);
}

int main()
{
    Node* stack = initStack();
    push(stack, 1);
    push(stack, 2);
    push(stack, 3);
    push(stack, 4);
    push(stack, 5);
    printStack(stack);
    popNode(stack);
    printStack(stack);
    pop(stack);
    printf("\n");
    printStack(stack);
    free(stack);
    return 0;
}