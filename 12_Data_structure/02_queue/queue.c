/* 队列是尾插法链表 */

#include <stdio.h>
#include <stdlib.h>

typedef struct QueueNode{
    int data;
    struct QueueNode* next;
} Node;

Node* initQueue()
{
	Node* queue = malloc(sizeof(Node));
    queue->data = 0;
	queue->next = NULL;
	return queue;
}

void queueInsert(Node* queue, int data)
{
    Node* cur = queue;
    Node* node = malloc(sizeof(Node));
    /*
    while (cur->next != NULL) {
        cur = cur->next;
    }
    */
    for (int i = 0; i < queue->data; ++i) {
        cur = cur->next;
    }
    cur->next = node;
    node->next = NULL;
    node->data = data;
    queue->data++;
}

void dequeueNode(Node* queue)
{
    Node* cur = queue->next;
    int data = cur->data;
    printf("dequeueNode data: %d\n", data);
    queue->next = cur->next;
    free(cur);
    cur = NULL;
    queue->data--;
}

void dequeue(Node* queue)
{
    Node* cur = queue->next;
    Node* tmp = NULL;
    printf("dequeue data: ");
    while (cur != NULL) {
        tmp = cur->next;
        printf("%d ", cur->data);
        free(cur);
        cur = tmp;
        queue->data--;
    }
    queue->next = NULL;
}

void printQueue(Node* queue)
{
    Node* cur = queue->next;
    while (cur) {
        printf("%d ", cur->data);
        cur = cur->next;
    }
    printf("number:%d\n", queue->data);
}

int main()
{
    Node* queue = initQueue();
    queueInsert(queue, 1);
    queueInsert(queue, 2);
    queueInsert(queue, 3);
    queueInsert(queue, 4);
    queueInsert(queue, 5);
    printQueue(queue);
    dequeueNode(queue);
    printQueue(queue);
    dequeue(queue);
    printf("\n");
    printQueue(queue);
    free(queue);
    return 0;
}