#include "control.h"
#include "gdevice.h"
#include "op.h"
#include "myoled.h"

/* 控制客厅灯的线程
static void *led_thread(void *arg)
{
    int fd;
    int data = (int)arg;
    printf("tmp = %d\n", data);

    fd = open("/dev/gpio4", O_RDWR);
    write(fd, &data, 1);
    close(fd);

    return NULL;
}
*/


// oled显示的线程（oled线程与其他操作设备的线程并发）
static void *oled_thread(void *arg)
{
    unsigned char *buffer = (unsigned char *)arg;

    oled_show(buffer);

    return NULL;
}


// 数据处理的线程函数，根据不同数据创建不同的线程去执行
static void *handle_thread(void *arg) 
{
    ctrl_info_t *ctrl_info = NULL;
    control* cur_ctrl = NULL;            // 控制链表
    gdevice* cur_dev = NULL;             // 设备链表
    gdevice* current = NULL;             // 查找到的设备节点
    unsigned char buffer[6] = {0xAA, 0x55, 0x00, 0x00, 0X55, 0xAA};
    pthread_t tid, oled_tid;
    int data = 0;
    int fd = -1;

    ctrl_info = (ctrl_info_t *)arg;
    cur_ctrl = ctrl_info->ctrl_phead;                           // 拿到控制链表存放在cur_ctrl，此时cur_ctrl就是取出来的控制链表
    cur_dev = ctrl_info->dev_phead;
    memcpy(buffer, ctrl_info->handle_buffer, sizeof(buffer));   // 将获取到的共享内存数据保存到该线程的buffer中

    pthread_create(&oled_tid, NULL, oled_thread, buffer);       // 任何操作我都会创建oled线程实时显示命令
    pthread_detach(oled_tid);

    switch (buffer[2]) {                                        // 对事件进行屏幕打印说明
        case 0x40: {
            printf("wake up xiao mo successfully!\n");          // 打印唤醒成功
            break;
        }
        case 0x41: {
            if (buffer[3] == 0x00)
                printf("Turn on the living room light!\n");
            else if (buffer[3] == 0x01)
                printf("Turn off the living room light!\n");
            break;
        }
        case 0x42: {
            if (buffer[3] == 0x00)
                printf("Turn on the bedroom light!\n");
            else
                printf("Turn off the bedroom light!\n");
            break;
        }
        case 0x43: {
            if (buffer[3] == 0x00)
                printf("Turn on the fan!\n");
            else
                printf("Turn off the fan!\n");
            break;
        }
        case 0x44: {
            printf("Prepare to open the door!\n");
            break;
        }
        case 0x45: {                                            // 烟雾传感器
            printf("Smoke alarm!\n");
            break;
        }
        case 0x49: {                                            // 舵机
            printf("The network opens the door!\n");
            break;
        }
    }

    // 我语音设置的不好，buffer[2]应该设置为0x50，就是tiktok的key，然后buffer[3]再设置不同的操作，就不需要这样加这一段代码了
    if ((buffer[2] == 0x50) || (buffer[2] == 0x51) || (buffer[2] == 0x52) || (buffer[2] == 0x53)) {
        current = find_device_by_name(cur_dev, "tiktok");
        pthread_create(&tid, NULL, current->thread, ctrl_info);
        pthread_detach(tid);
        pthread_exit(NULL);
    }

    // 此时buffer里的数据就是需要处理的数据，根据 buffer[2] 和 buffer[3] 判断是做什么操作
    // 一个gpio引脚控制线程，一个oled显示线程，一个语音播报线程，一个人脸识别线程（再包含着语音播报）

    // 根据指令的第二个参数查找需要操作的节点（说白了下面操作就是根据第二个参数来执行对应的线程函数）
    current = find_gdevice_by_key(cur_dev, buffer[2]);
    if (current == NULL) {                                      // 有可能是唤醒指令或者烟雾指令，不存在在设备节点中
        pthread_exit(NULL);       
    }

    // 这个if针对只有节点数据没有线程函数的设备，如卧室灯；当然这样做可以通用gpio输入输出高低电平的通用接口，不需要每次写线程函数，只需要跟bed.c一样设置变量即可
    if (current->thread == NULL) {                              // 该设备节点没有对应的线程函数，如果没有对应的线程函数则这就是只有一个包含数据的节点，就执行下面的操作
        fd = open(current->dev_name, O_RDWR);                   // 卡在这里执行程序的话，会不会阻塞住其他需要穿过来的命令，答案是不会，因为这就是一个线程，也就是为什么下面receive线程需要创建一个处理线程来执行；来一个新命令会创建单独的线程去执行程序，此时这个程序不会影响其他程序，并发执行
        ioctl(fd, current->gpio_mode);                          // 对其进行输出输入模式的控制               
        if (buffer[3] == 0x00) {                                // 0x00就是开启该设备
            if (current->level_trig)                            // 如果是高电平触发即去输入高电平开启设备
                data = 1;
            else
                data = 0;
            write(fd, &data, 4);                                // 输入高低电平
        } else if (buffer[3] == 0x01) {
            if (current->level_trig)                            // 如果是高电平触发，0x01代表关闭该设备
                data = 0;
            else
                data = 1;
            write(fd, &data, 4);                                // 输入高低电平
        }
    } else {
    // 拿到这个要操作设备的节点后创建对应的函数线程去处理，具体实现在设备线程函数内实现，这边作为一个通用框架，不需要改变
        pthread_create(&tid, NULL, current->thread, ctrl_info); // 将节点链表和数据buffer的信息一并传过去
        pthread_detach(tid);  // 执行完，该线程就自动释放
    }

/*  可以直接判断，然后创建线程执行对应的操作，也可以创建设备节点来添加模块，这样这个函数代码基本就不用动了，不然一直需要修改
    switch (buffer[2]) {
        case 0x41:  // 处理客厅灯，创建客厅灯线程
        {
            if (buffer[3] == 0x00) {            // 00是开灯
                data = 1;
            } else if (buffer[3] == 0x01) {     // 01是关灯
                data = 0;
            }
            printf("data = %d\n", data);
            pthread_create(&led_tid, NULL, led_thread, (void *)data);
            pthread_detach(led_tid);            // 执行完开关灯，该线程就自动释放
            break;
        }
        default:
            pthread_exit(NULL);
    }
*/
    pthread_exit(NULL);
}


// 接受共享内存的信息
static void *receive_get(void *arg) 
{
    unsigned char buffer[6] = {0xAA, 0x55, 0x00, 0x00, 0X55, 0xAA};
    int shmid;
    pthread_t tid;
    char *shm = NULL;
    ctrl_info_t *ctrl_info = NULL;
    ctrl_info_t *ctrl_cur = NULL;
    struct shared_use_st *shared;   // 共享内存的数据段

    ctrl_cur = (ctrl_info_t *)arg; // 赋值，此时ctrl_cur就包含了链表中的该控制项设备项和共享内存标识符
    ctrl_info = (ctrl_info_t *)malloc(sizeof(ctrl_info_t));      // 下面赋值了就需要申请空间，不然指向错误
    // 初始化操作
    ctrl_info->shmid = ctrl_cur->shmid;
    ctrl_info->ctrl_phead = ctrl_cur->ctrl_phead;
    ctrl_info->dev_phead = ctrl_cur->dev_phead;    
    memcpy(ctrl_info->handle_buffer, buffer, sizeof(buffer));

    shmid = ctrl_info->shmid;                                    // 取出共享内存标识符

    shm = shmat(shmid, NULL, 0);                                 // 将共享内存连接到当前线程的地址空间，此时shm就是共用的地址空间
    shared = (struct shared_use_st*)shm;                         // 设置共享内存，此时shared指向的空间就是共享的空间

    while (1) {
        memset(buffer, 0, sizeof(buffer));                       // 每次循环清空buffer（每次循环buffer都要接受共享内存的数据）
        pthread_mutex_lock(&shared->mutex);
        pthread_cond_wait(&shared->cond, &shared->mutex);
        memcpy(buffer, shared->shm_buffer, sizeof(buffer));      // 复制共享内存中的数据到buffer中
        memcpy(ctrl_info->handle_buffer, buffer, sizeof(buffer));// 将buffer中取出来的共享内存数据复制到申请空间的结构体中，方便下面传输数据
        pthread_create(&tid, NULL, handle_thread, ctrl_info);    // 创建handle线程处理ctrl_info传过来的结构体数据
        pthread_detach(tid);                                     // 分离该线程，执行完自动释放
        pthread_mutex_unlock(&shared->mutex);
    }

    free(ctrl_info);                                             // 释放申请的空间
    shmdt(shm);                                                  // 将共享内存从当前线程的地址空间分离
    pthread_exit(NULL);
}


// 初始化接受消息处理模块, 并且将其加入控制链表
void add_receive_to_ctrl_list(control** list)
{
    control* node = (control*)malloc(sizeof(control));

    strcpy(node->ctrl_name, "receive");
    memset(node->dev_name, '\0', sizeof(node->dev_name));
    node->get = receive_get;

    node->next_ctrl = *list;
    *list = node;
}
