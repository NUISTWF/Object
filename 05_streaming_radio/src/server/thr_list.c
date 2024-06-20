#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#include <proto.h>
#include "thr_list.h"
#include "server_conf.h"
#include "medialib.h"

static pthread_t tid_list;  // 线程id

static int nr_list_ent;                         // 频道个数
static struct mlib_listentry_st* list_ent;      // 节目清单的一个指针（频道id和的desc内容描述），list_ent[0]代表第一个节目信息

/**
 * @brief 发送节目清单的线程函数
 * 
 * @param p 创建线程时传递的参数，这里是空的没使用
 * @return 无
 * @note 将媒体结构体的数据转为要发送的频道节目清单，再给频道节目清单 struct msg_list_st 结构体的每一项进行数据填充，最后发给组播ip
 */
static void* thr_list(void* p)
{
    int totalsize;
    struct msg_list_st *entlistp;       // 总的节目单的结构体，用来存放节目单的信息（下面这个结构体包含在这个结构体内）
    struct msg_listentry_st *entryp;    // 每个频道信息内容的结构体，用来指向每一个频道的信息的指针

    totalsize = sizeof(uint8_t);        // msg_list_st一开始这个结构体内的0的id大小
    // 搞一个循环将每一个节目的内容大小全部加起来，为下面malloc做准备
    for (int i = 0; i < nr_list_ent; i++) {
        syslog(LOG_INFO, "[thr_list] chnid = %d, desc = %s", list_ent[i].chnid, list_ent[i].desc);
        totalsize += sizeof(struct msg_listentry_st) + strlen(list_ent[i].desc); // 每个节目单的描述内容大小
    }
    // 此时 totalsize 就是 struct msg_list_st 这个节目清单结构体的整体大小

    entlistp = (struct msg_list_st *)malloc(totalsize);
    if (NULL == entlistp) {
        syslog(LOG_ERR, "[thr_list] malloc error:%s", strerror(errno));
        exit(1);
    }

    entlistp->chnid = LISTCHNID;    // 即0号id
    entryp = entlistp->entry;       // 这样entryp就指向entlistp->entry这个内存第一个频道地址了，下面对entryp进行操作即可

    int size;
    for (int i = 0; i < nr_list_ent; i++) {
        size = sizeof(struct msg_listentry_st) + strlen(list_ent[i].desc);
        // 下面利用entryp指针，给每一项 entlistp 的频道信息进行填充
        entryp->chnid = list_ent[i].chnid;  // chnid是1个字节不需要转网络字节序
        entryp->len = htons(size);          // len是2个字节需要转换下网络字节序
        strcpy(entryp->desc, list_ent[i].desc);     // 字符串每个都是单字节传输，所以也不需要转换网络字节序
        entryp = (void *)(((char *)entryp) + size);
    }

    int ret;
    while (1) {
        // 往组播ip去发 entlistp 节目单信息，发 totalsize 整个节目单的大小，sndaddr是目标地址的信息
        ret = sendto(sd, entlistp, totalsize, 0, (void *)&sndaddr, sizeof(sndaddr));
        if (ret < 0) {
            syslog(LOG_WARNING, "[thr_list] send data fail:%s", strerror(errno));
        } else {
            syslog(LOG_DEBUG, "[thr_list] send data success!");
        }
        sleep(1);       // 每一秒发一次节目单信息
    }
}

/**
 * @brief 创建节目单的线程，用于将节目单的信息内容发到组播地址ip上
 * 
 * @param listp 节目单的信息内容
 * @param nr_ent 命令行上的字符串
 * @return 成功返回0，失败返回-1
 * @note 无
 */
int thr_list_create(struct mlib_listentry_st *listp, int nr_ent)
{
    int err;
    list_ent = listp;           // 清单内容 list_ent[0] 就是第1个频道的清单内容
    nr_list_ent = nr_ent;       // 频道个数

    err = pthread_create(&tid_list, NULL, thr_list, NULL);
    if (err) {
        syslog(LOG_ERR, "[thr_list] pthread_create fail, the reason is :%s.", strerror(err));
        return -1;
    }
    return 0;
}

/**
 * @brief 线程销毁函数
 * 
 * @param void 无
 * @return 0代表成功
 * @note 无
 */
int thr_list_destroy(void)
{
    pthread_cancel(tid_list);
    pthread_join(tid_list, NULL);
    return 0;
}