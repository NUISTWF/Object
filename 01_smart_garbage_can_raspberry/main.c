#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
// 后续不用wiringPi的串口函数，自己写驱动
#include <wiringPi.h>
#include <wiringSerial.h>

#include "garbage.h"            // 包含指令和c调用python的函数
#include "myoled.h"
#include "socket.h"


#define PORT    "9999"


static int serial_fd;
static int servo_fd;


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;      // 互斥锁 静态初始化
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;         // 条件变量 静态初始化


// 将识别的数据发给语音模块让其播报
void *sound_thread_func(void *arg)
{
    unsigned char *buffer = (unsigned char *)arg;                // 此时buf应该为这种类型 buffer[6] = {0xAA, 0X55, 0x41, 0x00, 0x55, 0xAA};
    
    write(serial_fd, buffer, sizeof(buffer)/sizeof(buffer[0]));  // 判断好参数，设置完数据发给语音模块，它会根据数据进行语音播报

    return NULL;
}


// 根据垃圾识别出来的数据来进行开对应的垃圾桶盖
void *trash_thread_func(void *arg)
{
    unsigned char *buffer = (unsigned char *)arg;
    int angle = 0;

    // 我就一个舵机意思一下，下面可以添加代码 （我的舵机是自己写的驱动程序，直接打开该设备即可操作）       
    if (buffer[2] == 0x41) {            // 如果为干垃圾就打开对应的垃圾桶；打开垃圾桶，即驱动舵机 2 -- > 45°
        angle = 2;                      // input angle: 1-0 2-45 3-90 4-135 5-180
        write(servo_fd, &angle, 4);     
        delay(5000);                    // 延时5s关盖
        angle = 1;                      // 1 --> 0°
        write(servo_fd, &angle, 4);     // 关盖
    }
    /* else if (buffer[2] == 0x42) {
        angle = 2;
        write(servo_fd2, &angle, 4);    // 驱动第二个打开的舵机口！
        delay(5000);
        angle = 1;
        write(servo_fd2, &angle, 4);
    } */

    return NULL;
}


void *oled_thread_func(void *arg)
{
    unsigned char *buffer = (unsigned char *)arg;

    oled_show(buffer);

    return NULL;
}


// 语音唤醒线程
void *voice_thread_func(void *arg)
{   
    int len = 0;
    unsigned char buffer[6] = {0xAA, 0X55, 0x00, 0x00, 0x55, 0xAA};

    while (1) {                             // 循环等待唤醒指令
        len = read(serial_fd, buffer, sizeof(buffer)/sizeof(buffer[0]));    // 读串口数据，当我喊小陌同学的时候，语音模块会发一串数据到树莓派上（当该线程读取到该数据说明要开始进行垃圾识别了）
        if (buffer[2] != 0x46) {                                            // 防止接收到的初始唤醒数据出错
//          printf("buffer[2] = 0x%x, read xiaomo error %s:%d\n", buffer[2], __func__, __LINE__);
            continue;                       // 只要不是0x46就说明没收到唤醒指令，重新开始读入数据
        }

        if ((len > 0) && (buffer[2] == 0x46) && (buffer[3] == 0x00)) {      // 此时条件都处理即代表唤醒成功，需要识别垃圾类型
            pthread_mutex_lock(&mutex);     // 获取到数据后需要上锁给阿里云发信号通知它开始获取图片识别垃圾
            pthread_cond_signal(&cond);     // 通知其他等待条件的线程
            pthread_mutex_unlock(&mutex);
        }

        buffer[2] = 0x00;               // 完成一次唤醒之后还置0
    }
    
    pthread_exit(NULL);
}


// 阿里云垃圾识别线程
void *ali_thread_func(void *arg)
{   
    unsigned char buffer[6] = {0xAA, 0x55, 0x00, 0x00, 0X55, 0xAA};
    char *category = NULL;
    pthread_t trash_tid, sound_tid, oled_tid;

    while (1) {                                     // 循环等待垃圾识别
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&cond, &mutex);           // 休眠等待信号，有信号过来就加锁，然后往下执行（等待语音唤醒后发信号过来才开始执行）
        // 此时如果有需要共享的资源就在这里实现
        pthread_mutex_unlock(&mutex);               // 再次解锁
        
        buffer[2] = 0x00;
        system(WGET_CMD);                           // 获取照片
        category = garbage_category(category);      // 判断照片是什么垃圾
        printf("%s\n", category);                   // 打印界面上

        // 将垃圾类型进行分类保存在buf中，发回语音助手，让语音助手播报
        if (strstr(category, "干垃圾")) {
            buffer[2] = 0x41;                       // 如果是干垃圾，就给他置为0x41
        } else if (strstr(category, "湿垃圾")) {
            buffer[2] = 0x42;
        } else if (strstr(category, "可回收垃圾")) {
            buffer[2] = 0x43;
        } else if (strstr(category, "有害垃圾")) {
            buffer[2] = 0x44;
        } else {
            buffer[2] = 0x45;
        }

        /* 
         * 当识别出来垃圾后要做三件事：
         * 1、将数据发给语音播报线程让其播报
         * 2、垃圾桶同时也需要开盖
         * 3、oled也要一起显示
         */
        pthread_create(&sound_tid, NULL, sound_thread_func, (void *)buffer);
        pthread_create(&trash_tid, NULL, trash_thread_func, (void *)buffer);
        pthread_create(&oled_tid, NULL, oled_thread_func, (void *)buffer);
        // 设为分离态，这些线程执行结束就自动释放，不需要join等待
        pthread_detach(sound_tid);
        pthread_detach(trash_tid);
        pthread_detach(oled_tid);
    }

    pthread_exit(NULL);
}


void *socket_thread_func(void *arg)
{
    int sd, newsd;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len;
    char readBuf[32];
    int n_read = 0;
    char ipstr[32];


    sd = socket_init(PORT);                             // socket, bind, listen, 9999是端口号，ip其实不用给，我bind的时候给0.0.0.0就是默认自动获取当前ip

    client_addr_len = sizeof(client_addr);
    while (1) {                                         // 循环接受客户端的连接
        newsd = accept(sd, (struct sockaddr *)&client_addr, &client_addr_len);      // 接受连接的同时存储客户端的地址信息
        if (newsd < 0) {
            perror("accept");
            exit(1);
        }
        inet_ntop(AF_INET, &client_addr.sin_addr, ipstr, sizeof(ipstr));            // 解析客户端的ip
        printf("Client ip:%s and port:%d\n", ipstr, ntohs(client_addr.sin_port));   // 解析网络字节序并打印客户端的ip和端口号
        
        // 心跳包，防止客户端死机，服务端还在死等（用处不大，下面n_read小于0已经可以判断出来客户端没数据了）
        client_KeepAlive(newsd);

        // 此处一次连接一个客户端，不需要fork子进程来工作           
        while (1) {	                                    // 子进程在循环读取客户端发来的数据不退出，直到此个连接的客户端退出才结束循环
            memset(readBuf, 0, sizeof(readBuf));
            n_read = recv(newsd, readBuf, sizeof(readBuf), 0);
            if (n_read > 0) {
                printf("message num:%d and text:%s\n", n_read, readBuf);
                if (strstr(readBuf, "open")) {
                    pthread_mutex_lock(&mutex);         // 获取到数据后需要上锁给阿里云发信号通知它开始获取图片识别垃圾
                    pthread_cond_signal(&cond);         // 通知阿里云在等待条件的线程
                    pthread_mutex_unlock(&mutex);
                } else {
                    printf("please input open\n");
                }
            } else if (n_read <= 0) {		            // 断开连接也就是读取数据为0（断开就会读到0）
                printf("Client ip:%s is quit\n", ipstr);
                break;			                        // 此次客户端断开连接就退出循环，对应该客户端的子进程执行完毕
            }
        }
        close(newsd);                                   // 该客户端执行结束释放对应的客户端描述符
    }

    pthread_exit(NULL);
}


int main(int argc, char *argv[])
{
    pthread_t voice_tid, ali_tid, socket_tid;

    garbage_init();             // 阿里云平台的初始化
    wiringPiSetup();            // 目前串口打开函数是用的wiringPi，需要初始化
    myoled_init();              // oled的初始化

    serial_fd = serialOpen("/dev/serial0", 115200);                 // 打开需要用的设备
    servo_fd = open("/dev/servo", O_RDWR);

    pthread_create(&voice_tid, NULL, voice_thread_func, NULL);      // 开三个线程分别为：语音唤醒，阿里云垃圾识别，网络控制
    pthread_create(&ali_tid, NULL, ali_thread_func, NULL);
    pthread_create(&socket_tid, NULL, socket_thread_func, NULL);

    printf("Please start waking up [ni hao xiao mo] or [nc 192.168.18.11 9999]!\n");

    pthread_join(voice_tid, NULL);
    pthread_join(ali_tid, NULL);
    pthread_join(socket_tid, NULL);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

    close(servo_fd);
    close(serial_fd);

    exit(0);
}