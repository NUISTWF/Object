#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "thpool.h"

void func(void* arg)
{
    int num = *(int*)arg;
    printf("hello, arg = %d, cur thread tid = %ld\n", num, pthread_self());
    sleep(2);
    printf("tid = %ld , wait... 2 second end!\n", pthread_self());
}

int main()
{
    Thpool* threadpool = thpool_init(2, 4, 10);

    for (int i = 0; i < 30; i++) {
        int* num = (int*)malloc(sizeof(int));
        *num = i;
        printf("add task[%d]\n", i + 1);
        thpool_add_work(threadpool, func, (void *)num);
    } 

    printf("add task end!\n");
    printf("--------------------------------------------\n");
    printf("Working threads: %d\n", thpool_num_threads_working(threadpool));
    printf("Alive threads: %d\n", thpool_num_threads_alive(threadpool));
    printf("--------------------------------------------\n");
    sleep(4);
    printf("--------------------------------------------\n");
    printf("Working threads: %d\n", thpool_num_threads_working(threadpool));
    printf("Alive threads: %d\n", thpool_num_threads_alive(threadpool));
    printf("--------------------------------------------\n");
    sleep(20);

    thpool_destroy(threadpool);

    return 0;
}