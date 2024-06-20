#ifndef MEDIALIB_H__
#define MEDIALIB_H__

#include <stdint.h>

struct mlib_listentry_st {
    uint8_t chnid;      // 频道号
    char* desc;         // 该频道的描述（当前的char*指针是在当前本机下执行的，不会传输到网络上；在有待发送的内容中不使用本地指针即可）
};

// 像这种分文件编程，都是一个人负责一个模块
// 下面提供方法给server调用，以便于获取媒体库信息

int mlib_getChnList(struct mlib_listentry_st** , int * );  // 需要一个起始位置，给二级指针回填；int 是该数组多长
int mlib_freeChnList(struct mlib_listentry_st* );          // 释放内存
ssize_t mlib_readChn(uint8_t , void* , size_t );           // 给频道线程获取mp3的文件内容的

#endif