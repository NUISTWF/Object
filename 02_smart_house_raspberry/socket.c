#include "control.h"

static int sd = -1;

static int socket_init()
{
    struct sockaddr_in server_addr;
    const char *port = PORT;

    sd = socket(AF_INET, SOCK_STREAM, 0);    // IPPROTO_TCP, IPPRORO_SCTP
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

    if (listen(sd, 100)) {   // （本来100，现在只允许1个客户端连接）100 本来是指本连接池的大小（我能存放多少个半连接状态的节点），现在是支持客户端的连接数量
        perror("listen()");
        return -1;
    }

    return sd;
}


// 关闭文件描述符
static void socket_final(void)
{
    if (sd > 0)
        close(sd);
    sd = -1;
}


// 网络获取线程
static void *socket_get(void *arg) 
{
    int newsd;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len;
    unsigned char buffer[6] = {0x00, 0x00, 0x00, 0x00, 0X00, 0x00};
    int n_read = 0;
    pid_t pid;
    int shmid;
    char *shm = NULL;
    ctrl_info_t *ctrl_info = NULL;
    char ipstr[32];
    struct shared_use_st *shared;   // 共享内存的数据段

    ctrl_info = (ctrl_info_t *)arg;         // 传值，此时ctrl_info就包含了链表中的该控制项和共享内存标识符
    shmid = ctrl_info->shmid;               // 取出共享内存标识符

    sd = socket_init();                     // socket, bind, listen, 9999是端口号，ip其实不用给，我bind的时候给0.0.0.0就是默认自动获取当前ip 
    if (-1 == sd) {
        printf("socket_init error!\n");
        pthread_exit(NULL);
    }

    shm = shmat(shmid, NULL, 0);            // 将共享内存连接到当前线程的地址空间，此时shm就是共用的地址空间
    // 设置共享内存
    shared = (struct shared_use_st*)shm;    // 此时shared就是共享内存的地址空间（将共享内存强制转为存储6个数据的自定义结构体类型）

    client_addr_len = sizeof(client_addr);
    while (1) {                             // 循环接受客户端的连接
        newsd = accept(sd, (struct sockaddr *)&client_addr, &client_addr_len);      // 接受连接的同时存储客户端的地址信息
        if (newsd < 0) {
            perror("accept");
            exit(1);
        }
        inet_ntop(AF_INET, &client_addr.sin_addr, ipstr, sizeof(ipstr));            // 解析客户端的ip
        printf("Client ip:%s and port:%d\n", ipstr, ntohs(client_addr.sin_port));   // 解析网络字节序并打印客户端的ip和端口号

        pid = fork();                       // 创建子进程
        if (pid < 0) {
            perror("fork");
            exit(1);
        }
        if (pid == 0) { 	                // child，让子进程干活，父进程继续accept等待连接
            close(sd);                      // 子进程只要用于通信的newsd不需要sd就把子进程中的sd关闭     
            while (1) {	                    // 子进程在循环读取客户端发来的数据不退出，直到此个连接的客户端退出才结束循环
                memset(buffer, 0, sizeof(buffer));
                n_read = recv(newsd, buffer, sizeof(buffer), 0);
                pthread_mutex_lock(&shared->mutex);
                if (n_read > 0) { 
                    if ((buffer[0] == 0xAA) && (buffer[1] == 0x55) && (buffer[4] == 0x55) && (buffer[5] == 0xAA)) {      
                        printf("Received data from client: 0x%X, 0x%X, 0x%X, 0x%X, 0x%X, 0x%X\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5]);
                        memcpy(shared->shm_buffer, buffer, sizeof(buffer));// 往共享内存的地址上写数据（传输数据）
                    } else {                                               // 这个else是为了输入字符串服务的
                        printf("Received data from client:%s\n", buffer);
                        memcpy(shared->shm_buffer, buffer, sizeof(buffer));// 往共享内存的地址上写数据（传输数据）
                    }
                    pthread_cond_signal(&shared->cond);     // 网络上发消息了，就通知接收线程可以拿到数据了
                } else if (n_read <= 0) {		            // 断开连接也就是读取数据为0（断开就会读到0）
                    printf("Client ip:%s is quit\n", ipstr);
                    pthread_mutex_unlock(&shared->mutex);   // !!! 困扰我半天，没有这句话，第二次客户端链接的时候，就死在上锁那里不允许，我怀疑是没解锁，所以此处断开的时候需要解锁
                    break;                                  // 此次客户端断开连接就退出循环，对应该客户端的子进程执行完毕
                }
                pthread_mutex_unlock(&shared->mutex);
            }
            close(newsd);               // 关闭新产生的文件描述符，此次通信结束
            exit(0);                    // 子进程干完活就结束
        }
        if (pid > 0) {                  // 父进程执行的
            close(newsd);               // 父进程中对于不用的文件描述符也要关闭，释放资源，及时刷新缓冲区以至于客户端数据的接受，虽然子进程中释放了，父进程也需要释放！！！新套接字的描述符newsd，用于后续与客户端通信，原始的套接字 sd 仍然用于监听其他连接请求
        }
    }

    shmdt(shm);                     // 将共享内存从当前线程的地址空间分离
    close(sd);
    pthread_exit(NULL);
}


// 初始化网络模块, 并且将其加入
void add_socket_to_ctrl_list(control** list)
{
    control* node = (control*)malloc(sizeof(control));

    strcpy(node->ctrl_name, "socket");
    memset(node->dev_name, '\0', sizeof(node->dev_name));
    node->init = socket_init;
    node->final = socket_final;
    node->get = socket_get;
    node->set = NULL;                                   // 没有用到的就置为NULL

    node->next_ctrl = *list;
    *list = node;
}
