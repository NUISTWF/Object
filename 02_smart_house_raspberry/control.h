#ifndef __CONTROL__H
#define __CONTROL__H

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <time.h>
#include <pthread.h>
#include <mqueue.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <wiringPi.h>
#include <wiringSerial.h>
#include "face.h"
#include "myoled.h"

#define PORT    "9999"
//printf("receive: 0x%X, 0x%X, 0x%X, 0x%X, 0x%X, 0x%X\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5]);


typedef struct control {
    char ctrl_name[32]; 			// 监听模块名称
	char dev_name[32];           	// 需要打开设备的名字
    int (*init)(void); 				// 初始化函数
	void (*final)(void);			// 结束释放函数
	void *(*get)(void *arg);		// 监听线程函数，如语音监听
	void *(*set)(void *arg); 		// 设置线程函数，如语音播报
    
	struct control* next_ctrl;
} control;


// 共享内存空间的数据形式
struct shared_use_st
{
	pthread_mutex_t mutex;  		// 互斥锁（这样可以直接在其他线程中调用到互斥锁）
	pthread_cond_t cond;			// 条件变量
    unsigned char shm_buffer[6];
};


// 传输给线程共享内存的标识和控制链表
typedef struct {
	int shmid;						 // 共享内存标识符（线程传参，是为了让每一个线程都能知道是哪个共享空间）
	struct control *ctrl_phead;		 // 控制链表的各个节点（也是为了让线程处理函数拿到控制链表）
	struct gdevice* dev_phead;		 // 设备链表的各个节点（也是为了让线程处理函数拿到设备链表）
	unsigned char handle_buffer[6];  // 是为了在receive线程中可以将拿到的数据传给处理的线程函数		
} ctrl_info_t;


void add_voice_to_ctrl_list(control** list);
void add_socket_to_ctrl_list(control** list);
void add_smoke_to_ctrl_list(control** list);
void add_receive_to_ctrl_list(control** list);
void add_ds18b20_to_ctrl_list(control** list);
void add_sr04_to_ctrl_list(control** list);
void add_hc01_to_ctrl_list(control** list);


#endif