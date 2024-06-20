#include "gdevice.h"
#include "control.h"
#include "op.h"


// tiktok模块的线程执行函数
static void *tiktok_thread(void *arg)
{
    unsigned char buffer[6] = {0xAA, 0x55, 0x00, 0x00, 0X55, 0xAA};
    ctrl_info_t *ctrl_info = NULL;
    gdevice* cur_dev = NULL;

    ctrl_info = (ctrl_info_t *)arg; 
    memcpy(buffer, ctrl_info->handle_buffer, sizeof(buffer));           // 将共享内存中收到的数据拿过来存放在buffer中
    cur_dev = find_device_by_name(ctrl_info->dev_phead, "tiktok");      // 根据tiktok的名字查找到对应的设备节点（不然就是语音助手里面设置一个数值对应抖音的key，通过key查找也行）

    switch (buffer[2]) {
        case 0x50: {
            printf("Already playing next video!\n");    
            system("adb shell input swipe 540 1300 540 500 100");       // 安卓机屏幕往上滑
            break;
        }
        case 0x51: {
            printf("Go back to previous video!\n");
            system("adb shell input swipe 540 500 540 1300 100");       // 屏幕下滑
            break;
        } 
        case 0x52: {
            printf("Already liked this video!\n");
            system("adb shell \"seq 3 | while read i;do input tap 350 1050 & input tap 350 1050 & sleep 0.02;done;\""); // 每个双引号前面做个转义，说明不是跟前面的连一起，这是字符双引号
            // 双击屏幕 点赞; 0.01就是0.01秒后再次执行，也就是点击位置350 1050两次，之间的间隔0.01秒，0.01太快了变成点一次暂停了；需要点赞就改为0.02
            break;
        }  
        case 0x53: {
            printf("Exit Douyin and turn off your phone!\n");
            system("adb shell input keyevent 26");                      // 锁屏
            break;
        }
    }

    pthread_exit(NULL);
}


void add_tiktok_to_ctrl_list(gdevice** list)
{
    gdevice* node = (gdevice*)malloc(sizeof(gdevice));

    strcpy(node->device_name, "tiktok");
    node->key = 0x50;
    node->thread = tiktok_thread;       // 抖音实现的线程函数（需要一个线程函数即可，判断语音指令去刷抖音；还需要一个设备名字以备查找）

    node->next_dev = *list;
    *list = node;
}
