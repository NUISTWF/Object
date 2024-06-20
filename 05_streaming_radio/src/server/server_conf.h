#ifndef SERVER_CONF_H__
#define SERVER_CONF_H__

#define DEFAULT_MEDIA_DIR       "../../media"

enum {
    RUN_DAEMON = 1,     // 守护进程后台运行
    RUN_FOREGROUND      // 前台运行
};

struct server_conf_st {
    char* rcvport;      // 跨网络的数据传输不能用指针（当前客户端是本地执行，不会上传到网络）
    char* mgroup;       // 多播组组号
    char* media_dir;    // 默认媒体库路径
    char runMode;       // 运行模式（前台运行还是守护进程形式的后台运行 ）
    char* ifName;       // 指定网卡信息 interface name 接口名称
};

extern struct server_conf_st server_conf;   // 全局变量最好声明到头文件中，以防其他文件需要该变量
extern int sd;                              // 网络套接字；文件描述符
extern struct sockaddr_in sndaddr;          // 对端地址ip端口信息

#endif