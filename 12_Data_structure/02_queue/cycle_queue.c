/* 循环队列 */

#include <stdio.h>
#include <stdlib.h>

#define MAXSIZE 5

typedef struct QueueNode{
    int data[MAXSIZE];
    int front;
    int rear;
} Node;

Node* initQueue()
{
	Node* queue = malloc(sizeof(Node));
    queue->front = 0;
    queue->rear = 0;
	return queue;
}

int isFull(Node* queue)
{
    return ((queue->rear + 1) % MAXSIZE == queue->front) ? 1 : 0;
}

int isEmpty(Node* queue)
{
    return (queue->rear == queue->front) ? 1 : 0;
}

int enQueue(Node* queue, int data)
{
    if (isFull(queue))
        return -1;
    queue->data[queue->rear] = data;
    queue->rear = (queue->rear + 1) % MAXSIZE;
}

int deQueue(Node* queue)
{
    if (isEmpty(queue))
        return -1;
    int data = queue->data[queue->front];
    queue->front = (queue->front + 1) % MAXSIZE;
    return data;
}

void printfQueue(Node* queue)
{
    int elements = ((queue->rear - queue->front) + MAXSIZE) % MAXSIZE;
    int index = queue->front;
    for (int i = 0; i < elements; ++i) {
        printf("%d ", queue->data[index]);
        index = (index + 1) % MAXSIZE;
    }
    printf("\n");
}

int main()
{
    Node* queue = initQueue();
    enQueue(queue, 1);
    enQueue(queue, 2);
    enQueue(queue, 3);
    enQueue(queue, 4);
    printfQueue(queue);
    deQueue(queue);
    enQueue(queue, 5);
    printfQueue(queue);
    return 0;
}
