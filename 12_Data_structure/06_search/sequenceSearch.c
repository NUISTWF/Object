 #include <stdio.h>
 #include <stdlib.h>

typedef struct sequenceList {
    int* data;
    int length;
    int num;
} List;

List* initList(int length)
{
    List* list = malloc(sizeof(List));
    list->data = malloc(sizeof(int) * length);
    list->length = length;
    list->num = 1;
    return list;
}

void listAdd(List* list, int data)
{
    list->data[(list->num)++] = data;
}

void printList(List* list)
{
    for (int i = 0; i < list->num; ++i)
        printf("%d ", list->data[i]);
    printf("\n");
}

int search(List* list, int data)
{
    /* 这是常规查找，需要加一次判断
    for (int i = 0; i < list->num; ++i) {
        if (list->data[i] == data)
            return i;
    }*/
    // 哨兵查找
    int i = 0;
    list->data[0] = data;
    for (i = (list->num) - 1; list->data[i] != data; --i);   // 一直找，直到两个data相等为止
    return i;
}

 int main()
 {
    List* list = initList(5);
    listAdd(list, 8);
    listAdd(list, 3);
    listAdd(list, 4);
    listAdd(list, 6);
    printList(list);
    printf("search data index = %d\n", search(list, 4));

    return 0;
 }