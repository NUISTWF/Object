#include "gdevice.h"
#include "control.h"
#include "op.h"

// 该设备模块的线程执行函数
static void *led_thread(void *arg)          // 只传入当前节点的话就没有buffer数据来判断开关，要是只传输buffer的话就没办法赋值设备名字；最主要的还是方便需要调用语音模块的时候，在设备链表中可以直接找到语音节点，从而调用语音播报线程
{
    int fd;
    int data = 0;
    unsigned char buffer[6] = {0xAA, 0x55, 0x00, 0x00, 0X55, 0xAA};
    ctrl_info_t *ctrl_info = NULL;
    gdevice* cur_dev = NULL;                // 当前的设备节点

    ctrl_info = (ctrl_info_t *)arg; 
    memcpy(buffer, ctrl_info->handle_buffer, sizeof(buffer));
    cur_dev = find_gdevice_by_key(ctrl_info->dev_phead, buffer[2]);     // 取出当前节点目的是下面open的时候不需要我人为的写入打开路径

    if (buffer[3] == 0x00) {                // 00是开灯
        data = 1;
    } else if (buffer[3] == 0x01) {         // 01是关灯
        data = 0;
    }

    fd = open(cur_dev->dev_name, O_RDWR);   // 通过传参来实现
    write(fd, &data, 1);
    // 这样的话我这一大块代码也不需要修改，基本就是模板
    close(fd);
    pthread_exit(NULL);
}


// 添加客厅灯的设备链表，要是需要其他gpio设备，一样的驱动写好，主程序main中加一个设备链表，完成一个类似该函数的操作即可
void add_living_to_ctrl_list(gdevice** list)
{
    gdevice* node = (gdevice*)malloc(sizeof(gdevice));

    strcpy(node->device_name, "living");
    strcpy(node->dev_name, "/dev/gpio4");
    node->key = 0x41;               // 0x42卧室灯 0x43风扇 0x44人脸识别 0x45烟雾报警模块
    node->thread = led_thread;      // 线程函数

    node->next_dev = *list;
    *list = node;
}
