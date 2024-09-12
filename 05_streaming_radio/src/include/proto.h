#ifndef PROTO_H__
#define PROTO_H__

#include <stdint.h>

#define DEFAULT_MGROUP   "224.2.2.3"         // 多播组号
#define DEFAULT_RCVPORT  "2025"              // 端口
#define DEFAULT_NET_CARD "ens33"             // 网卡信息

#define CHNNR            100                 // 频道个数channel number
#define LISTCHNID        0                   // 0号频道发节目单（节目单所在频道）

#define MSG_CHANNEL_MAX  (65536-20-8)        // channel数据包的最大值
#define MAX_DATA         (MSG_CHANNEL_MAX-sizeof(uint8_t))

#define MSG_LIST_MAX     (65536-20-8)
#define MAX_ENTRY        (MSG_LIST_MAX - sizeof(uint8_t))

struct msg_channel_st {     // 其他频道号发的数据流
    uint8_t chnid;          // must between [MINCHNID, MAXCHNID] 表示频道号
    uint8_t data[];         // 实际上是一个占位符，用于灵活数组；这样在实际开辟内存时候根据所需大小来开辟内存
}__attribute__((packed));

struct msg_listentry_st {
    uint8_t chnid;          // 节目频道号                             1 
    uint16_t len;           // len表示此时该结构体是多大，占多少字节，这样才能知道后续哪里是id哪里是内容
    uint8_t desc[];         // 节目描述可长可短，如下节目单的样式       music:baby I'm sorry
}__attribute__((packed));

struct msg_list_st {        // 0号频道发节目单
    uint8_t chnid;          // must be LISTCHNID     0号频道
    struct msg_listentry_st entry[];    // 节目单的样式：1 music:baby I'm sorry / 2 sport:football
}__attribute__((packed));

#endif