/**********************************
 * @author      HIK_wangfei105
 * License:     hikvision
 * Time:	    2024.7.22 - 2024.7.23
 **********************************/

#ifndef HIK_THPOOL_
#define HIK_THPOOL_

#ifdef __cplusplus
extern "C" {
#endif

/* =================================== API ======================================= */


typedef struct thpool Thpool;


/**
 * @brief  Initialize threadpool
 *
 * Initializes a threadpool. This function will not return until all
 * threads have initialized successfully.
 *
 * @example
 *
 *    ..
 *    Thpool thpool;                      // First we declare a threadpool
 *    thpool = thpool_init(4, 8, 10);     // then we initialize it to 4-8 threads and capacity 10
 *    ..
 *
 * @param  min_num_threads   create the maximum number of threads in the thread pool
 * @param  max_num_threads   create the minimum number of threads in the thread pool
 * @param  queueCapacity     the capacity of the task queue, that is, the maximum number of tasks
 * @return threadpool    created threadpool on success,
 *                       NULL on error
 */
Thpool* thpool_init(int min_num_threads, int max_num_threads, int queueCapacity);   // 初始化线程池


/**
 * @brief Add work to the job queue
 *
 * Takes an action and its argument and adds it to the threadpool's job queue.
 * If you want to add to work a function with more than one arguments then
 * a way to implement this is by passing a pointer to a structure.
 *
 * NOTICE: You have to cast both the function and argument to not get warnings.
 *
 * @example
 *
 *    void print_num(int num){
 *       printf("%d\n", num);
 *    }
 *
 *    int main() {
 *       ..
 *       int a = 10;
 *       thpool_add_work(thpool, (void*)print_num, (void*)a);
 *       ..
 *    }
 *
 * @param  threadpool    threadpool to which the work will be added
 * @param  function_p    pointer to function to add as work
 * @param  arg_p         pointer to an argument
 * @return 0 on success, -1 otherwise.
 */
int thpool_add_work(Thpool* threadpool, void (*function_p)(void*), void* arg_p);    // 添加工作任务


/**
 * @brief Show currently working threads
 *
 * Working threads are the threads that are performing work (not idle).
 *
 * @example
 * int main() {
 *    Thpool thpool1 = thpool_init(2);
 *    Thpool thpool2 = thpool_init(2);
 *    ..
 *    printf("Working threads: %d\n", thpool_num_threads_working(thpool1));
 *    ..
 *    return 0;
 * }
 *
 * @param threadpool     the threadpool of interest
 * @return integer       number of threads working
 */
int thpool_num_threads_working(Thpool* threadpool);     // 当前正在工作的线程个数


/**
 * @brief Show currently working threads
 *
 * A live thread is a thread that is alive (not destroyed).
 * A working thread is definitely alive, but an alive thread is not necessarily working.
 *
 * @example
 * int main() {
 *    Thpool thpool = thpool_init(4);
 *    ..
 *    printf("Alive threads: %d\n", thpool_num_threads_alive(thpool));
 *    ..
 *    return 0;
 * }
 *
 * @param threadpool     the threadpool of interest
 * @return integer       number of threads alive
 */
int thpool_num_threads_alive(Thpool* threadpool);     // 当前活着的线程个数


/**
 * @brief Destroy the threadpool
 *
 * This will wait for the currently active threads to finish and then 'kill'
 * the whole threadpool to free up memory.
 *
 * @example
 * int main() {
 *    Thpool thpool1 = thpool_init(2);
 *    Thpool thpool2 = thpool_init(2);
 *    ..
 *    thpool_destroy(thpool1);
 *    ..
 *    return 0;
 * }
 *
 * @param threadpool     the threadpool to destroy
 * @return nothing
 */
void thpool_destroy(Thpool* threadpool);    // 销毁线程池


#ifdef __cplusplus
}
#endif

#endif
