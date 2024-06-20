#include "gdevice.h"
#include "control.h"
#include "op.h"
#include "face.h"

// 进行人脸识别的线程（也可以不用这些数据，就再加一个语音播报设备节点，定义为0x50，然后遍历节点去执行）
static void *face_thread(void *arg)
{
    // 人脸识别之后，要调用两个线程，一个语音播报线程（人脸识别成功就给他发AA 55 47 00 55 AA，不成功就是AA 55 46 00 55 AA），一个开门驱动舵机线程
    // 识别完成后，这里的arg会拿到设备链表，从而找到语音节点，然后往语音节点发buffer数据去触发语音模块
    int status = 0;
    double result = 0.0;
    pthread_t servo_tid, sound_tid;
    unsigned char servo_buffer[6] = {0xAA, 0x55, 0x49, 0x03, 0X55, 0xAA};            // 舵机数据
    unsigned char sound_failed_buffer[6] = {0xAA, 0x55, 0x46, 0x00, 0X55, 0xAA};     // 识别失败播报语音数据
    ctrl_info_t *ctrl_info_servo = NULL;
    ctrl_info_t *ctrl_info_sound = NULL;
    ctrl_info_t *ctrl_info_cur = (ctrl_info_t *)arg;                // 临时变量，用来将数据取出来放到申请的空间内
    gdevice* servo_dev = NULL;
    gdevice* sound_dev = NULL;
    
    ctrl_info_servo = (ctrl_info_t *)malloc(sizeof(ctrl_info_t));   // 申请内存，防止信息被覆盖（而且需要往这块空间写东西，所以需要申请内存，如果只是读取数据，不需要申请内存，直接赋值给其他变量即可）
    ctrl_info_sound = (ctrl_info_t *)malloc(sizeof(ctrl_info_t));   // 因为我要将我的buffer数据写入这个结构体中，所以需要申请内存，不然直接写入就是未定义行为，程序出现段错误，且数据覆盖

    // ctrl_info_servo的初始化
    ctrl_info_servo->shmid = ctrl_info_cur->shmid;                  // 进行初始化赋值
    ctrl_info_servo->ctrl_phead = ctrl_info_cur->ctrl_phead;
    ctrl_info_servo->dev_phead = ctrl_info_cur->dev_phead;
    memcpy(ctrl_info_servo->handle_buffer, servo_buffer, sizeof(servo_buffer));     // 最重要的buffer存放新的数据

    // ctrl_info_sound的初始化
    ctrl_info_sound->shmid = ctrl_info_cur->shmid;                  // 进行初始化赋值
    ctrl_info_sound->ctrl_phead = ctrl_info_cur->ctrl_phead;
    ctrl_info_sound->dev_phead = ctrl_info_cur->dev_phead;
    memcpy(ctrl_info_sound->handle_buffer, sound_failed_buffer, sizeof(sound_failed_buffer));
    
    // 获取到设备链表的节点
    servo_dev = find_device_by_name(ctrl_info_servo->dev_phead, "servo");
    sound_dev = find_device_by_name(ctrl_info_sound->dev_phead, "sound");
    
    // 人脸识别
    printf("Start face recognition!\n");
    system(WGET_CMD);                           // 获取照片
    result = face_category();                   // 调用封装的C函数去执行Python代码，得到相似度
    printf("reliability:%.2lf\n", result);
    if (result > 0.5)
        status = 1;
    else      
        status = 0;

    if (status) {
        // 识别成功就去调用舵机线程开门
        printf("face recognition success!\n");
        // 识别成功，调用开门舵机线程，发送buffer数据去驱动
        pthread_create(&servo_tid, NULL, servo_dev->thread, ctrl_info_servo);           // 创建执行舵机的线程
        // 等待线程执行结束，再去释放申请的地址空间资源
        pthread_join(servo_tid, NULL);
    } else {
        // 识别失败就直接进行语音播报失败即可，不需要调用舵机
        printf("face recognition failed!\n");
        pthread_create(&sound_tid, NULL, sound_dev->thread, ctrl_info_sound);
        pthread_join(sound_tid, NULL);
    }

    // 释放资源
    free(ctrl_info_servo);
    free(ctrl_info_sound);

    pthread_exit(NULL);
}


void add_face_to_ctrl_list(gdevice** list)
{
    gdevice* node = (gdevice*)malloc(sizeof(gdevice));

    strcpy(node->device_name, "face");
    node->key = 0x44;                   // 当我说开门的时候，会传入0x44的数据，检测到有对应设备就来执行该人脸检测线程
    node->thread = face_thread;

    node->next_dev = *list;
    *list = node;
}
