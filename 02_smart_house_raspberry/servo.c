#include "gdevice.h"
#include "control.h"
#include "op.h"

// 该设备模块的线程执行函数（这一块跟客厅灯一模一样，都是对io口操作）
static void *servo_thread(void *arg)
{
    int fd;
    int data = 0;
    unsigned char buffer[6] = {0xAA, 0x55, 0x00, 0x00, 0X55, 0xAA};
    unsigned char sound_success_buffer[6] = {0xAA, 0x55, 0x47, 0x00, 0X55, 0xAA};    // 识别成功播报语音数据
    ctrl_info_t *ctrl_info = NULL;
    gdevice* cur_dev = NULL;                // 当前的设备节点
    ctrl_info_t *ctrl_info_sound = NULL;    // 为传输给语音线程而定义的参数
    pthread_t sound_tid;
    gdevice* sound_dev = NULL;              // 存放语音节点的指针

    ctrl_info = (ctrl_info_t *)arg; 
    memcpy(buffer, ctrl_info->handle_buffer, sizeof(buffer));
    cur_dev = find_gdevice_by_key(ctrl_info->dev_phead, buffer[2]);

    // ctrl_info_sound的初始化（为创建一个语音线程服务的）
    ctrl_info_sound = (ctrl_info_t *)malloc(sizeof(ctrl_info_t));   // 因为我要将我的buffer数据写入这个结构体中，所以需要申请内存，不然直接写入就是未定义行为，程序出现段错误，且数据覆盖
    ctrl_info_sound->shmid = ctrl_info->shmid;                      // 进行初始化赋值
    ctrl_info_sound->ctrl_phead = ctrl_info->ctrl_phead;
    ctrl_info_sound->dev_phead = ctrl_info->dev_phead;
    memcpy(ctrl_info_sound->handle_buffer, sound_success_buffer, sizeof(sound_success_buffer));
    sound_dev = find_device_by_name(ctrl_info_sound->dev_phead, "sound");

    // input angle: 1-0 2-45 3-90 4-135 5-180（正常开门就是3，关门就是1，也就是0x03代表开门和0x01代表关门）
    switch (buffer[3]) {
        case 0x00:
            data = 0;
            break;
        case 0x01:
            data = 1;
            break;
        case 0x02:
            data = 2;
            break;
        case 0x03:
            data = 3;
            break;
        case 0x04:
            data = 4;
            break;
        case 0x05:
            data = 5;
            break;
    }
    printf("servo = %d°\n", (data-1)*45);
    fd = open(cur_dev->dev_name, O_RDWR);   // 通过传参来实现
    write(fd, &data, 1);

    // 驱动舵机之后，语音播报欢迎回家（我修改了，因为我发现网络开门之后没有语音播报，将人脸识别的语音播报放在舵机启动之后）
    printf("Welcome home!\n");
    pthread_create(&sound_tid, NULL, sound_dev->thread, ctrl_info_sound); // 创建执行语音播报的线程
    pthread_join(sound_tid, NULL);
    free(ctrl_info_sound);

    delay(5000);                            // 延时5s关门归0
    data = 1;
    write(fd, &data, 1);
    printf("Close door!\n");

    close(fd);
    pthread_exit(NULL);
}


// 添加舵机的设备链表
void add_servo_to_ctrl_list(gdevice** list)
{
    gdevice* node = (gdevice*)malloc(sizeof(gdevice));

    strcpy(node->device_name, "servo");
    strcpy(node->dev_name, "/dev/servo");
    node->key = 0x49;               // 我设置0x49是驱动舵机的代号(后面一位，即0x00就是输入0，0x02是45°)，这样我可以在其他模块内进行调用，且我可以直接网络发送该代号进行触发开门，不需要经过人脸识别
    node->thread = servo_thread;    // 线程函数

    node->next_dev = *list;
    *list = node;
}
