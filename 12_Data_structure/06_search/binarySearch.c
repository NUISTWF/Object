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
    list->num = 0;
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

// 递归方式查找
int binarySearchRecursion(List* list, int data, int start, int end)
{
    //int start = 0;
    //int end = (list->num) - 1;
    if (start > end)    // 退出条件，找到最后都没有找到
        return -1;

    int mid = (start + end) / 2;
    if (data > list->data[mid]) {
        start = mid + 1;
        binarySearchRecursion(list, data, start, end);
    } else if (data < list->data[mid]) {
        end = mid - 1;
        binarySearchRecursion(list, data, start, end);
    } else {
        return mid;
    }
}

// 循环实现查找
int binarySearch(List* list, int data)
{
    int start = 0;
    int end = (list->num) - 1;
    // 循环查找
    while (start <= end) {
        int mid = (start + end) >> 1; // >> 1 == / 2
        if (data > list->data[mid]) {
            start = mid + 1;
        } else if (data < list->data[mid]) {
            end = mid - 1;
        } else {
            return mid;
        }
    }
    return -1;
}

 int main()
 {
    int arr[] = {1,2,3,4,5,6,7,8,9};

    List* list = initList(sizeof(arr)/sizeof(arr[0]));
    for (int i = 0; i < sizeof(arr)/sizeof(arr[0]); ++i)
        listAdd(list, arr[i]);
    printList(list);
    printf("recursion search data index = %d\n", binarySearchRecursion(list, 8, 0, (list->num) - 1));
    printf("search data index = %d\n", binarySearch(list, 8));

    return 0;
 }