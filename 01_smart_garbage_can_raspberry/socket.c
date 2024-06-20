#include "socket.h"

int socket_init(const char *port)
{
    int sd;
    struct sockaddr_in server_addr;

    sd= socket(AF_INET, SOCK_STREAM, 0);    // IPPROTO_TCP, IPPRORO_SCTP
    if (sd < 0) {
        perror("socket()");
        return -1;
    }

    int val = 1;
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0) {        // SO_REUSEADDR -- reuse 重复使用 ，设置该选项，可以告诉操作系统允许地址和端口的重用，即套接字刚关闭时，它进入 TIME_WAIT 状态，以确保任何延迟的数据包都不会干扰下一个使用相同本地地址和端口的套接字
        perror("setsockopt");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(port));                                     // 先字符串转为整型数再转为网络字节序
    inet_pton(AF_INET, "0.0.0.0", &server_addr.sin_addr);
    if (bind(sd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {     // 给socket绑定一个本地的地址
        perror("bind()");
        return -1;
    }

    if (listen(sd, 1)) {   // （本来100，现在只允许1个客户端连接）100 本来是指本连接池的大小（我能存放多少个半连接状态的节点），现在是支持客户端的连接数量
        perror("listen()");
        return -1;
    }

    return sd;
}

void client_KeepAlive(int newfd)// 心跳包，防止客户端死机，服务端还在死等
{
    int keepalive = 1;      	// 开启TCP KeepAlive功能
    int keepidle = 5;     		// tcp_keepalive_time 5s内没收到数据开始发送心跳包
    int keepcnt = 3;       		// tcp_keepalive_probes 发送3次新心跳包，没有回复则认为客户端断开
    int keepintvl = 3;     		// tcp_keepalive_intvl 每3s发送一次心跳包

    setsockopt(newfd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive, sizeof(keepalive));
    setsockopt(newfd, SOL_TCP, TCP_KEEPIDLE, (void *) &keepidle, sizeof(keepidle));
    setsockopt(newfd, SOL_TCP, TCP_KEEPCNT, (void *)&keepcnt, sizeof(keepcnt));
    setsockopt(newfd, SOL_TCP, TCP_KEEPINTVL, (void *)&keepintvl, sizeof(keepintvl));
}