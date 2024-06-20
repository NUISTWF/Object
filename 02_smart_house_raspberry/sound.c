#include "gdevice.h"
#include "control.h"
#include "op.h"

// 语音播报线程，将识别的数据发给语音模块让其播报
static void *sound_set(void *arg)           // arg传输的是接收线程那边的数据
{   
    int serial_fd;
    unsigned char buffer[6] = {0xAA, 0x55, 0x00, 0x00, 0X55, 0xAA};
    ctrl_info_t *ctrl_info = NULL;
    gdevice* cur_dev = NULL;                // 当前的设备节点

    ctrl_info = (ctrl_info_t *)arg; 
    memcpy(buffer, ctrl_info->handle_buffer, sizeof(buffer));
//  cur_dev = find_gdevice_by_key(ctrl_info->dev_phead, buffer[2]);    这边根据名字来找，因为根据key的话，数据中第三个数据只能是0x45，但是我人脸识别之后，发回去的数据不能是0x45，是0x47
    cur_dev = find_device_by_name(ctrl_info->dev_phead, "sound");       // 此时cur_dev就是当前的语音节点
    
    serial_fd = serialOpen(cur_dev->dev_name, 115200);
    if (serial_fd < 0)
        return NULL;
    
    if (NULL != buffer)
        write(serial_fd, buffer, sizeof(buffer)/sizeof(buffer[0]));     // 将收到的数据发给语音助手进行播报

    pthread_exit(NULL);
}


void add_sound_to_ctrl_list(gdevice** list)
{
    gdevice* node = (gdevice*)malloc(sizeof(gdevice));

    strcpy(node->device_name, "sound");
    strcpy(node->dev_name, "/dev/serial0");
    node->key = 0x45;                       // 烟雾报警模块触发的标识数值0x45
    node->thread = sound_set;

    node->next_dev = *list;
    *list = node;
}
