#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <syslog.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>

#include <proto.h>
#include "thr_channel.h"
#include "server_conf.h"
#include "medialib.h"

struct thr_channel_ent_st {     // 一个频道号对应一个线程tid
    uint8_t chnid;
    pthread_t tid;
};

struct thr_channel_ent_st thr_channel[CHNNR];   // 创建一个存放频道号码和线程tid的数组
static int tid_nextpos = 0;     // 全局函数每次进来都会自增不会从0开始

/**
 * @brief 频道线程函数，拿到对应的频道号，然后去获取该频道的mp3数据发送到组播上
 * 
 * @param ptr 接受一个频道信息清单list
 * @return 无
 * @note 此时的ptr就是单独一个节目频道的信息内容，因为在主函数中是构造的一个线程对应一个频道
 */
static void *thr_channel_snder(void *ptr)
{
    struct msg_channel_st *sbufp;           // 要发送的数据流结构体
    struct mlib_listentry_st *ent = ptr;    // 主函数传过来的频道信息清单结构体

    sbufp = malloc(MSG_CHANNEL_MAX);        // 给频道信息申请空间
    if (NULL == sbufp) {
        syslog(LOG_ERR, "[thr_channel][thr_channel_snder] malloc fail:%s\n", strerror(errno));
        exit(1);
    }

    sbufp->chnid = ent->chnid;  // 该信息的频道号等于传过来的频道号

    int len;
    while (1) {     // 这个线程一直读取这个频道ch文件夹里面的内容，然后往组播ip上发数据
        // 去获取频道ch文件夹下的所有信息内容（这边不需要流控了，因为在mlib_readChn函数内就做流控了，不至于音乐一瞬间播放结束）
        len = mlib_readChn(ent->chnid, sbufp->data, MAX_DATA);
        // 将sbufp要发送的数据发到指定的sndaddr地址上去
        int err = sendto(sd, sbufp, len + sizeof(uint8_t), 0, (void *)&sndaddr, sizeof(sndaddr));
        if (err < 0) {
            syslog(LOG_ERR, "[thr_channel][thr_channel_snder] send data of channel %d fail:%s", ent->chnid, strerror(errno));
            exit(1);
        }
        syslog(LOG_INFO, "[thr_channel][thr_channel_snder] send data of channel %d success and data size %d\n", ent->chnid, len);
        sched_yield(); // 主动出让调度器，没有也没关系
    }

    pthread_exit(NULL);
}

/**
 * @brief 创建一个频道线程
 * 
 * @param ptr 接受一个频道信息清单list，这个ptr指向一个ch频道文件的内容
 * @return 成功返回0，失败返回-1
 * @note 无
 */
int thr_channel_create(struct mlib_listentry_st *ptr)
{
    int err = pthread_create(&thr_channel[tid_nextpos].tid, NULL, thr_channel_snder, ptr);
    if (err) {
        syslog(LOG_WARNING, "[thr_channel][thr_channel_create] pthread_create fail: %s, ", strerror(err));
        return -1;
    }
    thr_channel[tid_nextpos].chnid = ptr->chnid;
    tid_nextpos++;

    return 0;
}

/**
 * @brief 销毁一个频道线程函数
 * 
 * @param ptr 接受一个频道信息清单list
 * @return 成功返回0，失败返回-1
 * @note 无
 */
int thr_channel_destroy(struct mlib_listentry_st *ptr)
{
    for (int i = 0; i < CHNNR; i++) {
        if (thr_channel[i].chnid == ptr->chnid) {   // 匹配我指定的频道
            if (pthread_cancel(thr_channel[i].tid) < 0) {
                syslog(LOG_ERR, "[thr_channel][thr_channel_destory] pthread_cancel: the thread of channel %d", ptr->chnid);
                return -ESRCH;
            }
        }
        pthread_join(thr_channel[i].tid, NULL);
        thr_channel[i].chnid = -1;
        return 0;
    }
}

/**
 * @brief 销毁所有的频道线程函数
 * 
 * @param void 无
 * @return 成功返回0，失败返回-1
 * @note 无
 */
int thr_channel_destroyall(void)
{
    for (int i = 0; i < CHNNR; i++) {
        if (thr_channel[i].chnid > 0) {         // 全部释放
            if (pthread_cancel(thr_channel[i].tid) < 0) {
                syslog(LOG_ERR, "[thr_channel][thr_channel_destroyall] pthread cancel fail, the thread of channel id is %d", thr_channel[i].chnid);
                return -ESRCH;
            }
            pthread_join(thr_channel[i].tid, NULL);
            thr_channel[i].chnid = -1;
        }
    }
    return 0;
}