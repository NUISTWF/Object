#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <net/if.h>
#include <errno.h>
#include <arpa/inet.h>

#include <proto.h>  // #include "../include/proto.h"
#include "client.h"

/**
 * -P   --port      指定接收端口
 * -M   --mgroup    指定多播组（-M是指短格式输入命令，--mgroup是指长格式输入命令【相当于ls -a和ls --all】）
 * -p   --player    指定播放器
 * -H   --help      显示帮助
 */

// 如果命令行没有任何参数，就使用下面默认给定的参数
struct client_conf_st client_conf = {
    .rcvport = DEFAULT_RCVPORT,         // 默认客户端的端口
    .mgroup = DEFAULT_MGROUP,           // 默认加入的多播组组号
    .player_cmd = DEFAULT_PLAYERCMD     // 默认使用的播放器路径
};

/**
 * @brief 打印帮助文档
 * 
 * @param argv 命令行调用的第一个参数，一般是 argv[0]
 */
static void print_help(char *argv)
{
    printf("Usage: %s [-M mgroup] [-P port] [-p player]\n", argv);
    printf("-P   --port     指定接收端口\n");
    printf("-M   --mgroup   指定多播组\n");
    printf("-p   --player   指定播放器\n");
    printf("-H   --help     显示帮助\n");
}

/**
 * @brief 接受inet_pton函数的返回值，判断是否发生错误且如果发生错误是什么错误
 * 
 * @param epton inet_pton函数的返回值，一般是 epton = inet_pton();
 */
static void error_pton(int epton)
{
    if (epton <= 0) {
        if (epton == 0)
            fprintf(stderr, "Not in presentation format");
        else
            perror("inet_pton");
        exit(EXIT_FAILURE); // 相当于exit(1);  #define EXIT_FAILURE 1  /  #define EXIT_SUCCESS 0
    }
}

/**
 * @brief 封装一个write函数，坚持写够多少字节数
 * 
 * @param fd 需要写入的文件描述符
 * @param buffer 写入的数据存储地址
 * @param len 要写入的长度
 * @return 真正写入的字节数
 * @note 往fd中写入buf中的数据，坚持写够len个字节才结束
 */
static ssize_t writen(int fd, const char* buf, size_t len)
{
    int pos = 0;
    int ret = 0;
    // 万一读到10个字节只写入3个字节; buf+pos 下次循环从第三个位置开始写入后面七个数据
    while (len > 0) {                   // 大于0就一直重复的写
        ret = write(fd, buf + pos, len);// ssize_t write(int fd, const void *buf, size_t count);
        if (ret < 0) {
            if (errno == EINTR)         // 防止假错
                continue;
            perror("write()");
            return -1;       // 写出错
        }                    // write 写的时候要判断，我要写进去 len 个字符，有没有都写进去，对比 write 的返回值 ret
        pos += ret;          // 定义一个pos位置，写入了3个字节后从buf中第4个字节开始再次循环写入
        len -= ret;          // 减去已经写入的，还剩多少没写进去，保存在n_read中重新再写
    }
    
    return pos;
}

/**
 * @brief 客户端主函数，用于接受流媒体的信息流
 * 
 * @param argc 命令行参数个数
 * @param argv 命令行上的字符串
 * @return 无
 * @note 默认值，配置文件，环境变量，命令行参数（越往后越高，自动忽略前面的）
 */
int main(int argc, char* argv[])
{
    int pipefd[2];      // pipefd[0] 读端；pipefd[1] 写端
    int index = 0;
    int c;
    int udp_sd;
    int epton;
    int len = 0;
    struct sockaddr_in laddr, raddr, data_raddr;  // remote 远端
    char ipstr[40];
    struct ip_mreqn mreq;   // 设置本地要发生的多播组信息
    struct option argarr[] = {{"port",   1, NULL, 'P'}, // 1代表有参数，0代表没有参数
                              {"mgroup", 1, NULL, 'M'},
                              {"player", 1, NULL, 'p'},
                              {"help",   0, NULL, 'H'},
                              {NULL,     0, NULL,  0 }};

    // getopt_long()
    while ((c = getopt_long(argc, argv, "P:M:p:H", argarr, &index)) != -1) {  // 分析命令行（后面有冒号就代表有参数）
        switch(c) {
            case 'P':
                client_conf.rcvport = optarg;   // 如果P后面有参数，则optarg自动指向该参数（默认值覆盖为我命令行输入的值）
                break;
            case 'M':
                client_conf.mgroup = optarg;
                break;
            case 'p':
                client_conf.player_cmd = optarg;
                break;
            case 'H':
                print_help(argv[0]);    // argv[0]是./client 
                exit(EXIT_SUCCESS);
                break;
            default:
                print_help(argv[0]);
                abort();    // 终止程序的执行，并生成一个异常终止信号，即SIGABRT信号
                exit(EXIT_SUCCESS);
                break;
        }
    }

    // socket()
    udp_sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_sd < 0) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    // setsockopt()
    epton = inet_pton(AF_INET, client_conf.mgroup, &mreq.imr_multiaddr);   // 多播组的 IP 地址，多播组的ip需要将点分十进制转换为二进制
    error_pton(epton);  // 判断是否出错
    epton = inet_pton(AF_INET, "0.0.0.0", &mreq.imr_address);   // 本地接口的 IP 地址，万能的4个0代表默认的本地ip
    error_pton(epton);
    mreq.imr_ifindex = if_nametoindex(DEFAULT_NET_CARD);        // 本地接口的网络索引号，将网络接口名转换为对应的索引号
    // 上述将mreq结构体全部赋值成功，就去设置setsockopt
    if (setsockopt(udp_sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {   // 设置多播组，IP_MULTICAST_IF 全称为 "IP Multicast Interface"
        perror("setsockopt(IP_ADD_MEMBERSHIP)");
        close(udp_sd);
        exit(EXIT_FAILURE);
    }

    // 启用或禁用多播循环（意味着本地主机上的多播发送者也能接收到发送的数据包，本地都能接收到信息；当禁用时，本地发送者就收不到了，只有发送对象才能收到）
    int loop = 1; // 启用多播循环
    if (setsockopt(udp_sd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)) == -1) {
        perror("setsockopt(IP_MULTICAST_LOOP)");
        close(udp_sd);
        exit(EXIT_FAILURE);
    }

    // bind()
    memset(&laddr, 0, sizeof(laddr));
    laddr.sin_family = AF_INET;
    laddr.sin_port = htons(atoi(client_conf.rcvport));
    inet_pton(AF_INET, "0.0.0.0", &laddr.sin_addr);
    if (bind(udp_sd, (void *)&laddr, sizeof(laddr)) < 0) {
        perror("bind()");
        exit(EXIT_FAILURE);
    }

    // 创建管道，用于父子进程通信，传输视频流，交给子进程去exec播放
    if (pipe(pipefd) == -1) {
        perror("pipe()");
        exit(EXIT_FAILURE);
    }

    // 创建子进程
    fflush(NULL);
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork()");
        exit(EXIT_FAILURE);
    }

    // child子进程exec调用解码器，从读端 pipefd[0] 不断读取信息流内容，输出到播放器上进行播放
    if (pid == 0) {
        close(udp_sd);      // 不需要用sd，直接关闭
        close(pipefd[1]);   // 不需要写入，关闭写端
        // 重定向文件描述符；关闭标准输入0号描述符，将管道的读端作为0号文件描述符；将标准输入重定向到管道读端的文件描述符上（因为解码器只能获得标准输入的内容）
        // cat baby\ i\'m\ sorry.mp3  | mpg123 - // cat一个mp3文件，通过管道以标准输入的方式使mpg123播放（将cat命令的标准输出作为了另外一个命令的标准输入）
        dup2(pipefd[0], 0);     // 当前管道的读端作为子进程的标准输入，这样父进程往管道不断写数据就相当于往标准输入写数据，这样mpg123通过标准输入获得数据进行播放
        if (pipefd[0] > 0)      // 文件描述符 0 已经指向了管道的读端，不需要pipefd[0]，即可关闭
            close(pipefd[0]);

        execl("/usr/bin/sh", "sh", "-c", client_conf.player_cmd, NULL); // 这样调用sh来间接执行player_cmd的命令，这个命令执行mpg123播放器并且从标准输入即读端获取数据
        // execl("/usr/bin/mpg123", "mpg123", "-", NULL);   // （不然还需要将字符串拆开）这种方式不便于修改，上一行只需要在define处进行修改即可，也可以用户指定（随便变动，上行代码都不需要改）
        perror("execl()");
        exit(EXIT_FAILURE);
    }

    // parent从网络上不断收包通过管道发给子进程（父子进程通过管道进行通信）
    if (pid > 0) {
        close(pipefd[0]);

        // 收节目单（收到消息后区分哪些是节目单，一次性发过来100个节目信息和节目单信息，总共101个包）
        struct msg_list_st* msg_list = NULL;   // 存放节目单
        msg_list = (struct msg_list_st *)malloc(MSG_LIST_MAX); // 给定一个节目单的最大容量
        if (NULL == msg_list) {
            perror("malloc(msg_list_st)");
            exit(EXIT_FAILURE);
        }

        socklen_t raddr_len = sizeof(raddr);
        while (1) {         // 防止没收到包，再次执行一下
            len = recvfrom(udp_sd, msg_list, MSG_LIST_MAX, 0, (void *)&raddr, &raddr_len);    // 接受消息存储到msg_list中，同时接受对端的地址信息；先执行客户端，就会卡在这里等待消息
            if (len < sizeof(struct msg_list_st)) {     // 最少会接收到这个结构体大小的字节，即1个字节，占位符不占大小，如果比这个还小肯定出错了
                fprintf(stderr, "message is too small.\n");
                continue;
            }
            if (msg_list->chnid != LISTCHNID) {     // 如果不是0则不是节目单，重新收包（因为服务器同时给组播内每个人都发信息流，包含了节目单信息msg_list_st和正常的节目流信息msg_channel_st）
                fprintf(stderr, "channel id is not match.\n");  // 这里的chnid不需要网络转，因为是单字节，网络上传输字节序不变；而下面的len是16位2个字节，所以需要网络转主机
                continue;
            }
            fprintf(stdout, "list len: %d.\n", len);
            break;      // 如果跳过了上述内容（没收到的包或者收到的包不是节目单），就代表收到节目单的包了，跳出循环往下走
        }
        inet_ntop(AF_INET, &raddr.sin_addr, ipstr, sizeof(ipstr));  // 将raddr中的sin_addr的二进制ip地址信息转为点分十进制保存到ipstr中
        printf("---message from ip:%s, port:%d---\n", ipstr, ntohs(raddr.sin_port));
        
        // 打印节目单
        struct msg_listentry_st* pos = NULL;   // 定义一个指针，用来指向每一个节目单的数组起始点
        for (pos = &(msg_list->entry[0]); (char*)pos < ((char*)(msg_list) + len); pos = (struct msg_listentry_st*)((char*)pos + ntohs(pos->len))) { // pos指针的偏移量是当前结构体内的长度，当成char单字节来移动，如果不加强转就是每次移动sizeof(struct msg_listentry_st)个大小；要转化len，因为这是在网络上传输的数据
            // 整个包的长度是len个字节，msg_list是指针指向该数据包的起始位置，保证两边类型匹配，将左边也强转为char*，这样只有当pos指向数据包最后一个地方时就结束
            printf("channel %d : (%s)\n", pos->chnid, pos->desc);   // 1 music:baby I'm sorry             
        }
        free(msg_list); // 释放掉节目单信息的空间，已经打印出来节目单清单了
        
        // 选择频道
        printf("\nPlease select a channel:");
        int chooseChnId;
        int ret = 0;
        while (1) {                            // 防止输入错误，万一我输入其他类型的值
            ret = scanf("%d", &chooseChnId);   // 返回值是成功读取并匹配的输入项的个数，因为输入不匹配或者输入结束而无法继续读取，则返回值可能小于格式化字符串中指定的输入项的个数
            if (ret != 1) {
                printf("Please enter a valid number\n");
                continue;
                //exit(EXIT_FAILURE);
            } else {
                printf("You have selected channel %d, and then play.\n", chooseChnId);
                break;
            }
        }

        // 收所选择的频道包，将数据流转发给子进程，即往管道写端 pipefd[1] 去写数据即可，子进程的播放器会自动拿到管道流内的数据进行解析播放
        struct msg_channel_st* msg_channel = NULL;
        msg_channel = (struct msg_channel_st*)malloc(MSG_CHANNEL_MAX);
        if (NULL == msg_channel) {
            perror("malloc(msg_channel_st)");
            exit(EXIT_FAILURE);
        }

        while (1) {         // 这边是收具体频道的数据
            socklen_t data_raddr_len = sizeof(data_raddr);
            int data_len = recvfrom(udp_sd, msg_channel, MSG_CHANNEL_MAX, 0, (void *)&data_raddr, &data_raddr_len);
            if (data_raddr.sin_addr.s_addr != raddr.sin_addr.s_addr || data_raddr.sin_port != raddr.sin_port) { // 验证两次收到（节目单和数据流）的是不是同一个地址发出的
                fprintf(stderr, "Ignore: address not match.\n");
                continue;
            }
            if (data_len < sizeof(struct msg_channel_st)) {
                fprintf(stderr, "Ignore: message too small.\n");
                continue;
            }
            if (msg_channel->chnid == chooseChnId) {    // 验证是否是我所选择的频道；会收到很多包，拿到我想要的
                fprintf(stdout, "choose %d channel stream len: %ld.\n", chooseChnId, data_len - sizeof(uint8_t));
                if (writen(pipefd[1], msg_channel->data, data_len - sizeof(uint8_t)) < 0)   // 封装了个函数，坚持写够多少个字节，搞个循环一直将收到的内容全部写入，防止写入写少了
                    exit(EXIT_FAILURE);
            }
        }

        free(msg_channel);

         // 当前关闭收音机的时候就默认离开多播组了，程序结束时，使用 DROP 操作离开多播组
        if (setsockopt(udp_sd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq)) == -1) {
            perror("setsockopt(IP_DROP_MEMBERSHIP)");
            close(udp_sd);
            exit(EXIT_FAILURE);
        }

        close(pipefd[1]);
        close(udp_sd);
    }

    exit(EXIT_SUCCESS);
}