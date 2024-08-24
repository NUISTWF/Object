/* ********************************
 * Author:       HIK_wangfei105
 * License:	     hikvision
 * Time:	     2024.7.22 - 2024.7.23
 * Description:  Library providing a threading pool where you can add
 *               work. For usage, check the thpool.h file or README.md
 *
 *//** @file thpool.h *//*
 *
 ********************************/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include "thpool.h"


#define DEFAULT_ADD_THREAD_NUM 		2


/* task */
typedef struct task {
	void (*function)(void* arg);        /* function pointer          */
	void* arg;                          /* function's argument       */
} Task;


/* threadPool */
typedef struct thpool {
	/* task queue */
	Task* taskQueue;			// 任务队列（每一项存储着一个任务task，直接指针开辟空间来存放）
	int queueCapacity;			// 队列容量
	int queueSize;				// task个数
	int queueFront;             /* pointer to front of queue  （队头删除排队任务） */
	int queueRear;           	/* pointer to rear  of queue  （队尾插入新加任务） */

	/* thread pool */
	pthread_t managerID;		/* manger thread id （管理者线程tid） */
	pthread_t* threadIDs;		/* work thread id arrs （一块空间来存储线程池内的工作线程tid） */
	int minNum;					/* min threads （最小线程数） */
	int maxNum;					/* max threads */
	int workNum;				/* busy thread nums (work thread) （正在工作的线程数） */
	int aliveNum;				/* threads alive number（存活的线程个数） */
	int exitNum;				/* kill thread nums （要杀死的线程个数） */

	/* 对整个任务队列进行同步，不能多个线程竞争一个任务 */
	pthread_mutex_t mutexPool;  /* 锁整个线程池 */
	pthread_mutex_t mutexWork;  /* 工作线程的个数，上锁（锁 workNum 变量的） */
	pthread_cond_t  condFull;   /* 工作是不是满了，如果为满则需要阻塞生产者 */
	pthread_cond_t  condEmpty;  /* 工作队列是不是空的，如果为空则需要阻塞消费者（即阻塞工作线程） */

	int shutdown;				/* 是否销毁线程池（1销毁，0保留） */
} Thpool;


// 工作线程退出时，需要清空对应的存放线程id的数组值
static void thread_exit(void* arg)
{
	Thpool* threadpool = (Thpool*)arg;
	// 将存放线程id的数组对应的位置清空以便于存放下一个新创建的线程
	for (int i = 0; i < threadpool->maxNum; i++) {
		if (threadpool->threadIDs[i] == pthread_self()) {
			printf("threadExit() called, %ld exiting...\n", threadpool->threadIDs[i]);
			threadpool->threadIDs[i] = 0;
			break;
		}
	}
	pthread_exit(NULL);
}


// 每个线程函数都去执行任务队列中的每个任务里面的待执行函数
static void* thpool_worker(void* arg)
{
	Thpool* threadpool = (Thpool*)arg;

	// 每个线程都需要不停的去读任务队列（每个线程函数都会对线程池进行操作，所有需要互斥锁）
	while (1) {
		pthread_mutex_lock(&threadpool->mutexPool);
		// 判断当前的任务队列是否为空（为空就阻塞等待任务）
		while (threadpool->queueSize == 0 && !threadpool->shutdown) {	// while 是由于condEmpty也会通知其他所有阻塞的等待线程，当某一个线程抢到锁后执行结束，其他线程依然需要阻塞在这
			pthread_cond_wait(&threadpool->condEmpty, &threadpool->mutexPool);
			
			// 判断是不是需要销毁线程，如果是则让当前的工作线程自杀
			if (threadpool->exitNum > 0) {
				(threadpool->exitNum)--;
				if (threadpool->aliveNum > threadpool->minNum) {	// 只有当存活数大于最小数才注销
					(threadpool->aliveNum)--;
					pthread_mutex_unlock(&threadpool->mutexPool);
					thread_exit(threadpool);	// 注销当前线程（自杀）
				}
			} // 如果执行到这里，还会继续判断while是否成立，不成立则该工作线程继续阻塞等待
		}
		if (threadpool->shutdown) {
			pthread_mutex_unlock(&threadpool->mutexPool);
			thread_exit(threadpool);
		}

		/* get task */
		Task task = threadpool->taskQueue[threadpool->queueFront];
		threadpool->queueFront = (threadpool->queueFront + 1) % (threadpool->queueCapacity);	// 队头后移
		(threadpool->queueSize)--;
		pthread_cond_signal(&threadpool->condFull);		// 唤醒阻塞的生产者（即主线程往任务队列放任务的）
		pthread_mutex_unlock(&threadpool->mutexPool);

		/* work threads num */
		pthread_mutex_lock(&threadpool->mutexWork);
		(threadpool->workNum)++;	// 开始执行任务，工作线程数加一
		pthread_mutex_unlock(&threadpool->mutexWork);

		/* thread start work */
		task.function(task.arg);	// 调用函数指针执行第一个任务待执行的函数

		free(task.arg);				// 当每次传入工作队列的都是堆内存，需要释放
		task.arg = NULL;

		pthread_mutex_lock(&threadpool->mutexWork);
		(threadpool->workNum)--;	// 执行完一个任务，工作线程数减一
		pthread_mutex_unlock(&threadpool->mutexWork);
	}

	return NULL;
}


// 管理者线程（去检测当前任务队列中任务的数量和工作线程的个数，动态的创建或销毁线程）
static void* thpool_manager(void* arg)
{
	Thpool* threadpool = (Thpool*)arg;

	while (!threadpool->shutdown) {
		sleep(3);	// 每隔3秒检测一次

		pthread_mutex_lock(&threadpool->mutexWork);
		int workNum = threadpool->workNum;				// 取出当前正在工作的线程数量
		pthread_mutex_unlock(&threadpool->mutexWork);

		pthread_mutex_lock(&threadpool->mutexPool);
		int queueSize = threadpool->queueSize;			// 取出线程池中的任务数量
		int aliveNum = threadpool->aliveNum;			// 取出线程池中存活的线程数量
		pthread_mutex_unlock(&threadpool->mutexPool);

		/* add thread */
		// 如果当前任务个数大于存活个数且存活线程数小于最大线程数，则创建线程
		if (queueSize > aliveNum && aliveNum < threadpool->maxNum) {
			pthread_mutex_lock(&threadpool->mutexPool);
            int counter = 0;
            for (int i = 0; i < threadpool->maxNum && counter < DEFAULT_ADD_THREAD_NUM
                && threadpool->aliveNum < threadpool->maxNum; i++) {
                if (threadpool->threadIDs[i] == 0) {
					printf("create new thread!\n");
                    pthread_create(&threadpool->threadIDs[i], NULL, thpool_worker, (void*)threadpool);
                    counter++;
                    (threadpool->aliveNum)++;
                }
            }
            pthread_mutex_unlock(&threadpool->mutexPool);
		}

		/* destroy thread */
		// 如果当前工作线程数的双倍小于存活线程数且存活线程数大于最小线程数，则销毁多余的线程
		if (workNum * 2 < aliveNum && aliveNum > threadpool->minNum) {	// 销毁 DEFAULT_ADD_THREAD_NUM 个线程
			pthread_mutex_lock(&threadpool->mutexPool);
			threadpool->exitNum = DEFAULT_ADD_THREAD_NUM;	// 设置要销毁的线程个数
			pthread_mutex_unlock(&threadpool->mutexPool);
			// 当前管理线程不能够去销毁工作线程（让工作的线程自杀）
			// 当前存活的线程数减去工作线程的两倍还有剩余，没干活的线程都会阻塞住（广播唤醒他们去自杀）
			for (int i = 0; i < DEFAULT_ADD_THREAD_NUM; i++) {
				pthread_cond_signal(&threadpool->condEmpty);	// 唤醒阻塞的工作线程（也就是现在没任务的工作线程）
			}
		}
	}
	
	pthread_exit(NULL);
}


Thpool* thpool_init(int min_num_threads, int max_num_threads, int queueCapacity)
{
	Thpool* threadpool = NULL;

	do {	// do while (0) 实现 goto 语句来释放资源
		threadpool = (Thpool*)malloc(sizeof(Thpool));
		if (NULL == threadpool) {
			printf("malloc threadpool failed\n");
			break;
		}
		memset(threadpool, 0, sizeof(Thpool));

		threadpool->threadIDs = (pthread_t*)malloc(sizeof(pthread_t) * max_num_threads);
		if (NULL == threadpool->threadIDs) {
			printf("malloc threadIDs failed\n");
			break;
		}
		memset(threadpool->threadIDs, 0, sizeof(pthread_t) * max_num_threads);
		threadpool->minNum = min_num_threads;
		threadpool->maxNum = max_num_threads;
		threadpool->workNum = 0;
		threadpool->aliveNum = min_num_threads;		// 和最小线程数相等，开始按照最小线程数创建，当任务多了再添加线程数，但不能超过 max threads
		threadpool->exitNum = 0;	// 需要退出的线程数需要根据实际来判断

		threadpool->taskQueue = (Task*)malloc(sizeof(Task) * queueCapacity);
		if (NULL == threadpool->taskQueue) {
			printf("malloc taskQueue failed\n");
			break;
		}
		memset(threadpool->taskQueue, 0, sizeof(Task) * queueCapacity);
		threadpool->queueCapacity = queueCapacity;
		threadpool->queueSize = 0;
		threadpool->queueFront = 0;		// 此时没任务，头和尾指向同一个位置
		threadpool->queueRear = 0;

		pthread_mutex_init(&threadpool->mutexPool, NULL);
		pthread_mutex_init(&threadpool->mutexWork, NULL);
 		pthread_cond_init(&threadpool->condFull, NULL);
		pthread_cond_init(&threadpool->condEmpty, NULL);

		threadpool->shutdown = 0;

		pthread_create(&threadpool->managerID, NULL, thpool_manager, (void*)threadpool);  		// 创建管理者线程
		for (int i = 0; i < min_num_threads; i++) {
			pthread_create(&threadpool->threadIDs[i], NULL, thpool_worker, (void*)threadpool);	// 创建工作线程
		}

		printf("thpool_init success!\n");

		return threadpool;
	} while (0);
	
	if (threadpool && threadpool->threadIDs) free(threadpool->threadIDs);
	if (threadpool && threadpool->taskQueue) free(threadpool->taskQueue);
	if (threadpool) free(threadpool);

	return NULL;
}


// 往线程池的任务队列放任务
int thpool_add_work(Thpool* threadpool, void (*function_p)(void*), void* arg_p)
{
	if (threadpool == NULL || function_p == NULL)
		return -1;

	/* 锁住线程池，添加完任务再解锁（因为拿任务和加任务对同一块区域进行操作） */
	pthread_mutex_lock(&threadpool->mutexPool);	// 因为添加任务和获取任务会同时使用共享资源
	while (threadpool->queueSize == threadpool->queueCapacity && !threadpool->shutdown) {	// 任务满了且线程池没有被销毁
		pthread_cond_wait(&threadpool->condFull, &threadpool->mutexPool);	// 队列满了，阻塞生产者（即主线程往任务队列放任务的）
	}
	if (threadpool->shutdown) {
		pthread_mutex_unlock(&threadpool->mutexPool);
		return -1;
	}

	threadpool->taskQueue[threadpool->queueRear].function = function_p;	// 将任务函数指针放到任务队列中
	threadpool->taskQueue[threadpool->queueRear].arg = arg_p;				// 将任务参数放到任务队列中
	(threadpool->queueSize)++;	// 队列中任务数加一
	printf("queue task size :%d\n", threadpool->queueSize);
	threadpool->queueRear = (threadpool->queueRear + 1) % threadpool->queueCapacity;   // 队列尾指针后移
	pthread_cond_signal(&threadpool->condEmpty);	// 队列不为空，唤醒消费者（即工作线程从任务队列拿任务的）

	pthread_mutex_unlock(&threadpool->mutexPool);

	return 0;
}


// 获取当前正在工作的线程个数
int thpool_num_threads_working(Thpool* threadpool)
{
	if (NULL == threadpool)
		return -1;

	pthread_mutex_lock(&threadpool->mutexPool);
	int workNum = threadpool->workNum;
	pthread_mutex_unlock(&threadpool->mutexPool);

	return workNum;
}


// 获取当前活着的线程个数
int thpool_num_threads_alive(Thpool* threadpool)
{
	if (NULL == threadpool)
		return -1;

	pthread_mutex_lock(&threadpool->mutexPool);
	int aliveNum = threadpool->aliveNum;
	pthread_mutex_unlock(&threadpool->mutexPool);

	return aliveNum;
}


// 线程池的销毁
void thpool_destroy(Thpool* threadpool)
{
	if (NULL == threadpool)
		return;

	threadpool->shutdown = 1;		// 销毁标志，这样工作线程和管理线程每次循环时检测到就会释放资源退出

	pthread_cond_broadcast(&threadpool->condEmpty);	// 唤醒所有消费者线程（while条件不成立就会全部向下执行，遇到shutdown为1就退出）

	for (int i = 0; i < threadpool->maxNum; i++) {
		pthread_join(threadpool->threadIDs[i], NULL);
	}
	pthread_join(threadpool->managerID, NULL);

	if (threadpool->threadIDs) free(threadpool->threadIDs);
	if (threadpool->taskQueue) free(threadpool->taskQueue);

	pthread_mutex_destroy(&threadpool->mutexPool);
	pthread_mutex_destroy(&threadpool->mutexWork);
	pthread_cond_destroy(&threadpool->condFull);
	pthread_cond_destroy(&threadpool->condEmpty);

	if (threadpool) free(threadpool);
	threadpool = NULL;

	return;
}

