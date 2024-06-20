#ifndef CLIENT_H__
#define CLIENT_H__

#define DEFAULT_PLAYERCMD       "/usr/bin/mpg123 - > /dev/null" // 不想命令行运行该命令时看到运行的其他内容，重定向到null里面（该命令行敲入mpg123 - 表示只会接受标准输入来的数据流）
//#define DEFAULT_PLAYERCMD       "/snap/bin/vlc > /dev/null"  // vlc也可以播放音频

struct client_conf_st {
    char *rcvport;      // 跨网络的数据传输不能用指针（当前客户端是本地执行，不会上传到网络）
    char *mgroup;       // 多播组组号
    char *player_cmd;   // 所使用的播放器路径
};

// client.c中的全局变量如果还有机会在其他.c文件内使用，应该在任意头文件内extern包含一下声明，此时别的.c文件也能看到该变量，就不会报错，会在整个项目文件夹下查找该变量
extern struct client_conf_st client_conf;    // 声明一下，其他.c文件包含client.h头文件就可以使用

#endif