#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <glob.h>
#include <syslog.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>

#include "medialib.h"
#include "server_conf.h"
#include "mytbf.h"
#include <proto.h>

#define PATHSIZE    1024
#define LINEBUFSIZE 1024
#define MP3_BITRATE 1024 * 256

// 解析当前目录结构，有什么内容，里面的desc描述内容
struct channel_context_st {
    uint8_t chnid;
    char *desc;
    glob_t mp3glob;     // 存储每个ch文件夹下的mp3文件，比如 mp3glob.gl_pathv[] 和 gl_pathc 分别代表文件名字和文件个数
    int pos;            // 指每个ch文件夹下第几个mp3文件，pos来指向这些位置（pos都是0，除非多个mp3文件）
    int fd;             // 每个mp3的文件描述符
    off_t offset;       // 偏移量
    mytbf_t* tbf;       // 流量控制（Token Bucket Filter）漏桶和令牌桶两种实现
};

// 下面这个数组用来存放每个频道ch文件夹下的信息内容
static struct channel_context_st channel[CHNNR + 1];   // 定义一个数组，每个值都是上述结构体类型的

/**
 * @brief 获取媒体库的每一个频道的内容信息（说白了就是得到每一个ch目录下的内容）
 * 
 * @param path 传递过来每一个媒体库中的频道文件
 * @return 返回每一个频道文件的信息结构体
 * @note 无
 */
static struct channel_context_st *path2entry(const char *path)
{
    char pathstr[PATHSIZE];
    char linebuf[LINEBUFSIZE];
    FILE *fp;
    struct channel_context_st *me;
    static uint8_t curr_id = 1;

    strncpy(pathstr, path, strlen(path) + 1);
    strncat(pathstr, "/desc.txt", strlen("/desc.txt") + 1);   // pathstr == "ch1/desc.txt"

    fp = fopen(pathstr, "r");
    if (fp == NULL) {
        syslog(LOG_INFO, "[medialib][channel_context_st] %s is not a channel dir(Cannot find desc.text)", path);
        return NULL;
    }

    // 获取到的desc描述内容存放到linebuf中
    if (fgets(linebuf, LINEBUFSIZE, fp) == NULL) {
        syslog(LOG_INFO, "[medialib][channel_context_st] %s is not a channel dir(Cannot get the desc.text)", path);
        fclose(fp);
        return NULL;
    }

    fclose(fp);

    // 申请 struct channel_context_st 类型的空间来存放内容数据
    me = malloc(sizeof(*me));
    if (me == NULL) {
        syslog(LOG_ERR, "[medialib][channel_context_st] malloc fail :%s\n", strerror(errno));
        return NULL;
    }

    // 下面都是对me这个大结构体进行赋值，然后将每个频道ch文件夹的信息内容返回给 mlib_getChnList 函数中
    me->tbf = mytbf_init(MP3_BITRATE/8, MP3_BITRATE/8*10);  // 令牌桶进行流控
    if (me->tbf == NULL) {
        syslog(LOG_ERR, "[medialib][channel_context_st] mytbf_init fail :%s\n", strerror(errno));
        free(me);
        return NULL;
    }

    me->desc = strdup(linebuf);     // 直接开辟新内存来存储linebuf的描述内容，这个me->desc是需要释放free的

    strncpy(pathstr, path, strlen(path) + 1);           // pathstr == "ch1"
    strncat(pathstr, "/*.mp3", strlen("/*.mp3") + 1);   // pathstr == "ch1/*.mp3"
    // 获取当前文件下的每一个mp3文件，将文件个数以及名字存储在mp3glob中
    if (glob(pathstr, 0, NULL, &(me->mp3glob)) != 0) {
        syslog(LOG_ERR, "[medialib][channel_context_st] %s is not a channel dir(cannot find mp3 files)", path);
        free(me);
        return NULL;
    }

    // 这两个变量是为拓展铺垫的，当前ch文件夹内就一个mp3文件，读取文件的偏移量也是从0开始，也就是从头开始，也可以从中间开始收听
    me->pos = 0;
    me->offset = 0;

    me->fd = open(me->mp3glob.gl_pathv[me->pos], O_RDONLY); // 这个fd对应mp3的文件内容
    if (me->fd < 0) {
        syslog(LOG_WARNING, "[medialib][channel_context_st] %s open failed", me->mp3glob.gl_pathv[me->pos]);
        free(me);
        return NULL;
    }

    me->chnid = curr_id;
    curr_id++;

    return me;
}

/**
 * @brief 获取媒体库的节目清单
 * 
 * @param result 回填一个节目单的起始地址（二级指针可以改写主函数传给他的一级指针内容）
 * @param resnum 回填一个节目单的个数
 * @return 0代表成功，1代表失败
 * @note 这个虽然对外只是获取媒体库的节目清单，但是里面也将每个媒体库频道的信息流全部保存下来，且发给用户的只有频道号和频道描述内容
 */
int mlib_getChnList(struct mlib_listentry_st** result, int *resnum)
{
    char path[PATHSIZE];
    glob_t glob_result;
    int ret = 0;
    int i = 0;
    int num = 0;
    struct mlib_listentry_st* ptr = NULL;
    struct channel_context_st* res = NULL;

    for (i = 0; i < CHNNR + 1; ++i) {
        channel[i].chnid = -1;
    }

    snprintf(path, PATHSIZE, "%s/*", server_conf.media_dir);   // 这个 path == "../../media/*"

    // 匹配 ../../media/* 目录下所有文件，就知道媒体库下面有多少个频道了
    ret = glob(path, 0, NULL, &glob_result);
    if (ret != 0) {
        if (ret == GLOB_NOMATCH) {
            //printf("No matching files found.\n");
            syslog(LOG_INFO, "No matching files found.");
        } else {
            //perror("glob");
            syslog(LOG_ERR, "glob error!");
            return 1;
        }
    }

    // 申请内存，该指针指向的每一项都是mlib_listentry_st结构体类型，也就是每一项不是常规的int整数，而是这种结构体的频道信息
    ptr = (struct mlib_listentry_st*)malloc(sizeof(struct mlib_listentry_st) * glob_result.gl_pathc);
    if (ptr == NULL) {
        syslog(LOG_ERR, "malloc(struct mlib_listentry_st*).");
    }

    for (i = 0; i < glob_result.gl_pathc; ++i) {
        // glob_result.gl_pathv[0] -> "../../media/ch1"  下面path2entry就是去解析每一个频道目录的内容并且返回
        res = path2entry(glob_result.gl_pathv[i]);  // 将路径变成每一条记录（现在能拿到每一个频道的信息内容）
        if (res != NULL) {
            syslog(LOG_DEBUG, "[medialib][mlib_getchnlist] path2entry success [chnid: %d desc: %s]", res->chnid, res->desc);
            memcpy(channel + res->chnid, res, sizeof(*res));    // 每个频道文件夹ch的信息都保存在channel数组中了
            ptr[num].chnid = res->chnid;
            ptr[num].desc = strdup(res->desc);  // 相当于给desc申请内存并且赋值
            num++;                              // 每次解析出来一个目录就自增一下，代表节目单的数量
        }
    }

    *result = realloc(ptr, sizeof(struct mlib_listentry_st) * num); // 给ptr扩容，申请为新的内存大小；成功时，返回指向新分配的内存块的指针（就相当于把ptr的指针赋值给它了）
    if (NULL == *result) {
        syslog(LOG_ERR, "[medialib][mlib_getchnlist]realloc failed.");
    }

    *resnum = num;

    return 0;
}

/**
 * @brief 释放媒体库申请的内存，释放各种在mlib_getChnList中用到的资源等等
 * 
 * @param 1 无
 * @return 
 * @note 无
 */
int mlib_freeChnList(struct mlib_listentry_st* )
{
    return 0;
}

/**
 * @brief 打开同一个频道（ch文件夹）内的下一个mp3开始播放
 * 
 * @param chnid 想要收到的频道号
 * @return 0是正常播放下一个；-1是播放结束，该频道没有内容了
 * @note 无
 */
static int open_next(uint8_t chnid)
{
    for (int i = 0; i < channel[chnid].mp3glob.gl_pathc; i++) {         // 循环ch文件夹内的mp3文件个数
        channel[chnid].pos++;
        if (channel[chnid].pos == channel[chnid].mp3glob.gl_pathc) {    // mp3glob.gl_pathc 是指ch文件夹频道下的mp3文件个数
            channel[chnid].pos = 0;                                     // 用来循环播放该频道的mp3文件
            break;
        }

        close(channel[chnid].fd);
        channel[chnid].fd = open(channel[chnid].mp3glob.gl_pathv[channel[chnid].pos], O_RDONLY);  // 打开下一个mp3的文件，获取文件描述符

        if (channel[chnid].fd < 0) {
            syslog(LOG_WARNING, "[medialib][open_next] open(%s) fail:%s", channel[chnid].mp3glob.gl_pathv[channel[chnid].pos], strerror(errno));
        } else {
            channel[chnid].offset = 0;
            return 0;
        }
    }

    syslog(LOG_ERR, "[medialib][open_next] None of mp3s in channel %d id available.", chnid);
    return -1;
}

/**
 * @brief 读取频道的信息流
 * 
 * @param chnid 想要读取的频道号
 * @param buf 存储信息流的buf
 * @param size 每次读取的大小
 * @return 返回读取到mp3的文件内容长度
 * @note 这个读取mp3内容的函数已经做了流控，不会一下子读多导致音乐一闪而过
 */
ssize_t mlib_readChn(uint8_t chnid, void *buf, size_t size)
{
    int tbfsize;
    tbfsize = mytbf_fetchtoken(channel[chnid].tbf, size);

    int len;
    while (1) {
        // 从fd想要收到的信息流中获取mp3的内容，存储到buf中，读取tbfsize个字节，从文件开始的offset偏移量来获取，指定读取数据的位置
        len = pread(channel[chnid].fd, buf, tbfsize, channel[chnid].offset);    // 32768 = 1024 * 256 / 8
        if (len < 0) {
            syslog(LOG_WARNING, "[medialib][mlib_readchn] media file %s pread() fail:%s", channel[chnid].mp3glob.gl_pathv[channel[chnid].pos], strerror(errno));
            if (open_next(chnid) < 0) {     // 没收到当前mp3的内容就播放下一个mp3文件
                syslog(LOG_ERR, "channel %d: There is no successed open", chnid);
            }
        } else if (len == 0) {
            syslog(LOG_DEBUG, "[medialib][mlib_readchn] media file %s is over.", channel[chnid].mp3glob.gl_pathv[channel[chnid].pos]);
            channel[chnid].offset = 0;
            open_next(chnid);   //打开下一首mp3
        } else {    // len > 0 这里就是第一首mp3读取成功
            channel[chnid].offset += len;   // 偏移量开始往后移，表示播放到什么时候了
            break;
        }
    }

    if (tbfsize - len > 0) {        // 归还令牌个数
        mytbf_returntoken(channel[chnid].tbf, tbfsize - len);
    }

    return len;
}