#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>

#include <proto.h>
#include "medialib.h"
#include "server_conf.h"
#include "thr_list.h"
#include "thr_channel.h"

/**
 * -P       指定端口（命令行指定端口，指定服务器往哪个端口发数据）
 * -M       指定多播组
 * -F       前台运行
 * -H       显示帮助
 * -I       指定网络设备
 * -D       指定媒体库位置（媒体库的路径可以变化，不一定就一定放在某个路径上）
 */

// 如果命令行没有任何参数，就使用下面默认给定的参数
struct server_conf_st server_conf = {
    .ifName = DEFAULT_NET_CARD,         // 指定默认网卡
    .media_dir = DEFAULT_MEDIA_DIR,     // 默认媒体库位置
    .mgroup = DEFAULT_MGROUP,           // 多播组ip
    .rcvport = DEFAULT_RCVPORT,         // 要发送的客户端端口号
    .runMode = RUN_DAEMON               // 后台运行
};

int sd;
struct sockaddr_in sndaddr;
struct mlib_listentry_st* list = NULL;

/**
 * @brief 打印帮助文档
 * 
 * @param argv 命令行调用的第一个参数，一般是 argv[0]
 */
static void print_help(char *argv)
{
    printf("Usage: %s [-M mgroup] [-P port] [-D /var/media]\n", argv);
    printf("-P       指定端口\n");
    printf("-M       指定多播组\n");
    printf("-F       前台运行\n");
    printf("-I       指定网络设备\n");
    printf("-D       指定媒体库位置\n");
    printf("-H       显示帮助\n");
}

/**
 * @brief 信号处理函数，用来做守护进程被干掉后的释放操作
 * 
 * @param signo 是 signal number 的缩写，用于表示信号的编号
 * @return 无
 * @note 给守护进程发终止信号后，放入来不及释放的内容，相当于钩子函数或者析构函数
 */
static void daemon_exit(int signo)
{
    // 里面做一些释放的操作
    // thr_channel_destroy();
    thr_channel_destroyall();
    mlib_freeChnList(list);
    syslog(LOG_INFO, "signal-%d caught, exit now.", signo);
    closelog();

    exit(EXIT_SUCCESS);
}

/**
 * @brief 创建一个守护进程
 * 
 * @param 无
 * @return 返回小于0是daemonize函数内部中途出错，-1是fork出错，-2是open出错，返回0表示成功
 * @note 无
 */
static int daemonize()
{
    pid_t pid = fork();
    if (pid < 0) {
        //perror("fork() in daemonize()");      // 此时还可以输出到终端，还没重定向
        syslog(LOG_ERR, "fork(): %s", strerror(errno));
        return -1;
    } else if (pid > 0) {   // 父进程执行到这里就结束了，守护进程不需要父进程等待收尸，父进程结束，1号init进程接管
        //printf("parent pid = %d\n", getpid());
        exit(EXIT_SUCCESS);
    }

    int fd = open("/dev/null", O_RDWR); // 按顺序赋值是文件描述符，此时fd=3
    if (fd < 0) {	
        //perror("open() in daemonize()");
        syslog(LOG_WARNING, "open(): %s", strerror(errno));
        return -2;
    }

    dup2(fd, 0);            // 重定向 0 1 2 文件描述符，全指向该fd文件；关闭0且将fd的副本复制到0号上
    dup2(fd, 1);
    dup2(fd, 2);
    if (fd > 2) {           // 上面打开文件fd应该默认是3号，此时关闭即可（此时0，1，2全部指向/dev/null）
        close(fd);
    }

    setsid();   // 当前子进程做成守护进程，pid_t setsid(void); 创建一个会话，并返回一个进程组的id，此时这个子进程就是新会话id，新进程组的组长id，即该子进程为守护进程了，继续向下运行程序
 
    chdir("/");             // 改变当前进程的工作目录为根目录（如果我一个进程踩着一个文件夹在执行，此时文件夹卸载就会不成功，进程还在这个文件夹运行，守护进程退不掉，所以就没法卸载了；需要修改守护进程的路径）
    umask(0);               // 确定在程序中不会产生文件了，还可以关掉umask的值（只会影响当前进程，而不会影响其他进程或系统的文件创建掩码）
    
    return 0;
}

/**
 * @brief socket初始化，包含 socket(); setsockopt();
 * 
 * @param 无
 * @return 返回socket的文件描述符
 * @note 无
 */
static int socket_init()
{   
    // socket()
    int server_sd;
    server_sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_sd < 0) {
        syslog(LOG_ERR, "scoket(): %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // setsockopt()
    struct ip_mreqn mreq;                               // 设置本地要发生的多播组信息
    inet_pton(AF_INET, server_conf.mgroup, &mreq.imr_multiaddr);    // 多播组的 IP 地址，多播组的ip需要将点分十进制转换为二进制
    inet_pton(AF_INET, "0.0.0.0", &mreq.imr_address);   // 本地接口的 IP 地址，万能的4个0代表默认的本地ip
    mreq.imr_ifindex = if_nametoindex(server_conf.ifName);          // 本地接口的网络索引号，将网络接口名转换为对应的索引号

    if (setsockopt(server_sd, IPPROTO_IP, IP_MULTICAST_IF, &mreq, sizeof(mreq)) < 0) {   // 设置多播组，IP_MULTICAST_IF 全称为 "IP Multicast Interface"。
        syslog(LOG_ERR, "setsockopt(IP_MULTICAST_IF): %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // bind(); 可省略，先发数据的一方在客户端可以解析其ip和端口（所以先发数据的一方可省bind）

    // sndaddr 往哪里发消息（往多播组ip和对应该多播组ip的端口上发数据）
    sndaddr.sin_family = AF_INET;
    sndaddr.sin_port = htons(atoi(server_conf.rcvport));
    inet_pton(AF_INET, server_conf.mgroup, &sndaddr.sin_addr);

    syslog(LOG_INFO, "[server][socket_init] socket init ok!\n");

    return server_sd;
}

/**
 * @brief 服务器端主函数
 * 
 * @param argc 命令行参数个数
 * @param argv 命令行上的字符串
 * @return 无
 * @note 无
 */
int main(int argc, char* argv[])
{
    int opt;
    struct sigaction sa;

    // 注册信号（当前的守护进程肯定是由一个信号意外结束；因为守护进程被杀死，执行不到那些释放操作，所以触发这些信号会去调用函数来释放）
    sa.sa_handler = daemon_exit;    // 设置处理函数
    sigemptyset(&sa.sa_mask);       // 清空 sa(sigaction) 结构，清空需要阻塞的信号集合
    sigaddset(&sa.sa_mask, SIGINT); // 将SIGINT加入信号触发集合中（这样触发下面加入sigaction中的一个信号后，又来一个集合内的信号就会阻塞等待触发）
    sigaddset(&sa.sa_mask, SIGQUIT);
    sigaddset(&sa.sa_mask, SIGTERM);

    sigaction(SIGINT, &sa, NULL);   // 注册 SIGINT 信号的处理函数；为什么不用signal，因为signal会重入，就是一个信号没执行完又来另外一个信号，这样前面一个信号就被覆盖掉不执行了；我需要这是三个信号都可以准确触发，所以使用sigaction函数
    sigaction(SIGQUIT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    // 建立系统日志
    openlog("streaming_radio", LOG_PID|LOG_PERROR, LOG_DAEMON);   // 和系统日志建立连接，用于初始化系统日志记录；打开系统日志，用于在程序中将消息发送到系统日志（由于守护进程没法使用终端输出，使用使用系统日志来观察）

    // 命令行参数分析
    while ((opt = getopt(argc, argv, "P:M:FHD:I:")) != -1) {
        switch (opt) {
            case 'P':
                server_conf.rcvport = optarg;    // 获取选项参数的值，保存到 input_file 中
                break;
            case 'M':
                server_conf.mgroup = optarg;
                break;
            case 'D':
                server_conf.media_dir = optarg;
                break;
            case 'I':
                server_conf.ifName = optarg;
                break;
            case 'F':
                server_conf.runMode = RUN_FOREGROUND;
                break;
            case 'H':
                print_help(argv[0]);    // argv[0]是./client 
                exit(EXIT_SUCCESS);
                break;
            case '?':                   // 处理未识别的选项
                fprintf(stderr, "Unknown option: %c\n", optopt);
                exit(EXIT_FAILURE);
            default:
                abort();
                exit(EXIT_FAILURE);
        }
    }

    // 守护进程的实现；服务器端完成一个守护进程的效果，一直后台跑，相当于广播一直开着
    if (server_conf.runMode == RUN_DAEMON) {
        if (daemonize() != 0)    // 在这之后的代码程序全是守护进程在执行（没法关联标准输出）
            exit(EXIT_FAILURE);
        syslog(LOG_INFO, "daemonize() successed!");      // 信息日志
    } else if (server_conf.runMode == RUN_FOREGROUND) {
        /* do nothing 后续可以比如在前台运行时候做扩展 */
    } else {
        syslog(LOG_ERR, "EINVAL server_conf.runMode!");
        exit(EXIT_FAILURE);
    }

    // socket初始化
    sd = socket_init();

    // 获取媒体库的频道信息（目的是创建节目单线程，每秒往外发节目单；顺带获取有多少个频道数，创建多少个频道线程）
    // 定义全局变量，方便释放；struct mlib_listentry_st* list = NULL;      // 频道清单信息 （根据mlib_getChnList内容得知，list[0]就代表第一个节目单的内容）
    int list_size;                              // 回填节目单大小，得到当前频道个数
    int err;
    err = mlib_getChnList(&list, &list_size);       // 获取频道描述信息和频道个数
    if (err) {
        syslog(LOG_ERR, "mlib_getChnList()");
        exit(EXIT_FAILURE);
    }

    // 创建节目单线程，发送节目单信息（用线程去创建并且每秒往多播组发送节目单信息）
    err = thr_list_create(list, list_size); // 传入要发送的节目单和频道个数
    if (err) {
        syslog(LOG_ERR, "thr_list_create()");
        exit(EXIT_FAILURE);
    }

    // 将节目单写入文件（list[0]就代表第一个节目单的内容，包含了chnid和desc）
    FILE *file = fopen("list.txt", "w");
    if (file == NULL) {
        perror("fopen");
        return 1;
    }
    for (int i = 0; i < list_size; ++i) {
        fprintf(file, "channel %d (%s)\n", list[i].chnid, list[i].desc);
    }
    fclose(file);

    // 创建频道线程，发送文件信息流（可以一个线程每秒往外发100个频道信息包；也可以每个频道使用一个线程去发，即100线程对100频道）
    int i;
    for (i = 0; i < list_size; ++i) {
        err = thr_channel_create(list + i);
        if (err) {
            fprintf(stderr, "thr_channel_create() fail: %s\n", strerror(err));
            exit(EXIT_FAILURE);
        }
    }

    // 启动日志，LOG_INFO就相当于标准输出
    syslog(LOG_INFO, "\n===============[server][main] create channel thread success!===============\n");
    
    while (1)
        pause();        // 以防忙等；用于挂起当前进程的系统调用，直到接收到一个信号（不然一直死等占用cpu调度，进程会进入睡眠状态，即不会被调度到CPU上执行，直到信号触发）
    
    closelog();         // 关闭日志

    exit(EXIT_SUCCESS);
}