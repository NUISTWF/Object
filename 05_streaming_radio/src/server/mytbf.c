#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#include "mytbf.h"

typedef void (*sighandler_t)(int);          // typedef int FUNC(int);  FUNC f; 就等同于 int f(int);

static struct mytbf_st *job[MYTBF_MAX];     // 存放指针的数组(指针数组)，也就是存放各个令牌桶结构体首地址的数组；job是该数组的名字，前面是数组内每组元素也就是每组指针的类型，数组最大存放MYTBF_MAX个元素
static pthread_mutex_t mut_job = PTHREAD_MUTEX_INITIALIZER;
static pthread_once_t init_once = PTHREAD_ONCE_INIT;

// static int inited = 0;
static pthread_t tid;

struct mytbf_st {                           // 令牌桶的结构体
    int cps;                                // 速率，每秒多少字符
    int burst;                              // 令牌的最大个数
    int token;                              // 令牌的数量
    int pos;                                // 第几个令牌桶，即该令牌桶在令牌桶数组内的位置
    pthread_mutex_t mut;
    pthread_cond_t cond;
};

static void* thr_alrm(void* p)
{   
    int i = 0;
    while (1) {
        pthread_mutex_lock(&mut_job);
        for(i = 0; i < MYTBF_MAX; i++) {        // 循环整个存放令牌桶的数组
            if (job[i] != NULL) {               // 遇到不为空的，说明有令牌桶结构体占了该位置了，则将该位置的令牌桶中的令牌数开始累加
                pthread_mutex_lock(&job[i]->mut);
                job[i]->token += job[i]->cps;   // token = token + cps 令牌数为最大传输字符数，每过一秒token令牌数都加上要传输的字符数，可以累加起来存到token中，之后要取令牌数可以一次性将所存的令牌数取出来使用
                if(job[i]->token > job[i]->burst)        // 当前令牌桶内的令牌数比上限令牌数还大，则当前令牌桶内的令牌数取为上限值
                    job[i]->token = job[i]->burst;
                pthread_cond_broadcast(&job[i]->cond);
                pthread_mutex_unlock(&job[i]->mut);
            }
        }
        pthread_mutex_unlock(&mut_job);
        sleep(1);
    }
}

static void module_unload()                 // 原来现场什么样就恢复成什么样；比如说进程在某一位置异常结束，要将当前数组内还没释放的令牌桶释放掉
{
    int i;

    pthread_cancel(tid);
    pthread_join(tid, NULL);

    for(i = 0; i < MYTBF_MAX; i++)          // 异常结束后再释放掉所有的申请过的空间，free空的没有影响，全free一遍即可
        free(job[i]);
}

static void module_load()                   // 模块加载
{
    int err = pthread_create(&tid, NULL, thr_alrm, NULL);
    if (err) {
        fprintf(stderr, "pthread_create()%s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

static int get_free_pos_unlock()            // 判断数组中是否有可用位置，如果有则返回数组下标，否则为-1
{
    int i;

    for(i = 0; i < MYTBF_MAX; i++) {        // 循环一圈该数组，看数组内有没有NULL，有空位置就可以存数据
        if(job[i] == NULL)
            return i;
    }

    return -1;
}

mytbf_t *mytbf_init(int cps, int burst)     // 初始化令牌桶
{
    struct mytbf_st *me;
    int pos;

    pthread_once(&init_once, module_load);

    // if(!inited) {                           // 全局的标志性变量，只执行一次，下次再创建一个令牌桶的时候就跳过该步骤，因为alarm会覆盖
    //     module_load();
    //     inited = 1;
    // }

    me = malloc(sizeof(*me));
    if(me == NULL)
        return NULL;

    me->token = 0;
    me->cps = cps;
    me->burst = burst;
    pthread_mutex_init(&me->mut, NULL);
    pthread_cond_init(&me->cond, NULL);
    
    pthread_mutex_lock(&mut_job);
    pos = get_free_pos_unlock();
    if(pos < 0) {                           // 小于0说明没有空位置给新的令牌桶了，则出错不能创建；大于0则是知道哪一个位置为空位置
        pthread_mutex_unlock(&mut_job);
        free(me);
        return NULL;
    }

    me->pos = pos;                          // 这个令牌桶的存放位置也放到令牌桶结构体中，说白了就是令牌桶结构体中有他存放的具体位置

    job[me->pos] = me;                      // 将该令牌桶结构体存到数组内的空位置上去

    pthread_mutex_unlock(&mut_job);

    return me;
}

static int min(int a, int b)
{
    if(a < b)
        return a;
    
    return b;
}

int mytbf_fetchtoken(mytbf_t *ptr, int size)// 用户调用此函数获取令牌数量，返回值为可用令牌数；若不够size个，则返回已有的最大令牌数
{
    struct mytbf_st *me = ptr;              // 空指针类型的ptr转化为令牌桶结构体的类型
    int n;

    if(size <= 0)
        return -EINVAL;                     // EINVAL表示参数非法，负号是因为用户使用的时候可以加个出错提示，返回值小于0的时候出错

    pthread_mutex_lock(&me->mut);
    while(me->token <= 0)                   // 查看有没有token，是不是大于0，有没有令牌在令牌桶里，如果没有则阻塞等待；此时等待中，alarm过了1秒发出终止信号，打断阻塞且执行处理函数增加了令牌数，继续向下执行
        pthread_cond_wait(&me->cond, &me->mut);

    n = min(me->token, size);               // size想要取到的令牌数和已有的令牌数比较，取小者
    me->token -= n;                         // 取走了n个令牌，则已有的令牌数token要减去取走的令牌数，记录令牌桶内还有多少令牌个数

    pthread_mutex_unlock(&me->mut);

    return n;
}

int mytbf_returntoken(mytbf_t *ptr, int size)    // 用户调用此函数将多余的令牌返还
{
    struct mytbf_st *me = ptr;

    if(size <= 0)                           // 还给我最少大于0个令牌数吧
        return -EINVAL;

    pthread_mutex_lock(&me->mut);
    me->token += size;                      // 令牌桶内的令牌数合并起来
    if(me->token > me->burst)               // 还回来后，如果比上限要大的话，则令牌数就等于上限令牌个数
        me->token = me->burst;
    pthread_cond_broadcast(&me->cond);   
    pthread_mutex_unlock(&me->mut);

    return size;                            // 返回归还的令牌个数
}

int mytbf_destory(mytbf_t *ptr)
{
    struct mytbf_st *me = ptr;
    pthread_mutex_lock(&mut_job);
    job[me->pos] = NULL;                    // 删除该令牌桶的时候也要将数组内的令牌桶所占位置清空
    pthread_mutex_unlock(&mut_job);

    pthread_mutex_destroy(&me->mut);
    pthread_cond_destroy(&me->cond);
    free(ptr);                              // 释放该令牌桶所申请的内存

    return 0;
}