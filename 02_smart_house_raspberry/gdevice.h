#ifndef __GDEVICE_H
#define __GDEVICE_H

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
#include <sys/shm.h>
#include <wiringPi.h>
#include <wiringSerial.h>
#include "face.h"
#include "myoled.h"


typedef struct gdevice {
    char device_name[32]; 			// 设备名称，如 "living"
    char dev_name[32];           	// 需要打开设备的名字，如 "/dev/gpio4"
    unsigned char key; 				// key值，用于匹配控制指令的值，如 0x41
//  int check_face_status; 			// 是否进行人脸检测状态
//  int voice_set_status; 			// 是否设置语音播报
    void *(*thread)(void *arg);     // 线程函数
    int gpio_pin;                   // 控制的gpio引脚
    int gpio_mode;                  // 输入输出模式
    int gpio_status;                // 高低电平状态
    int level_trig;                 // 电平触发

    struct gdevice* next_dev;
} gdevice;


void add_living_to_ctrl_list(gdevice** list);
void add_servo_to_ctrl_list(gdevice** list);
void add_sound_to_ctrl_list(gdevice** list);
void add_face_to_ctrl_list(gdevice** list);
void add_bed_to_ctrl_list(gdevice** list);
void add_fan_to_ctrl_list(gdevice** list);
void add_tiktok_to_ctrl_list(gdevice** list);


#endif