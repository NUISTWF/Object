#include "face.h"
#include "control.h"
#include "gdevice.h"
#include "op.h"
#include "ini.h"


#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0


// 解析ini文件，将数据存入设备链表
int handler_gdevice(void* user, const char* section, const char* name, const char* value)
{
//  printf("%s, %s, %s\n", section, name, value);                  // 打印所有进来的节点信息 
    gdevice* current= NULL;
    gdevice** device_list = (gdevice**)user;                       // 这个device_list就是main里面的设备链表，传入的二级指针，也可以直接定义为全局变量，就不需要这么麻烦了
    
    // 当我前面不为空节点，已经有了一个living，此时执行 else if，名字肯定不一样living不等于bed，所以会去创建新节点存放数据；而且也保证了，循环读取配置文件，到第二个节点内容时会执行else if创建新节点
    if (NULL == *device_list) {                                    // 第一个节点进来，需要开辟内存空间（也防止第二个节点进来重复给全局设备节点device_list再次开辟内存）
        *device_list = (gdevice*)malloc(sizeof(gdevice));
        memset((*device_list), 0, sizeof(struct gdevice));
        strcpy((*device_list)->device_name, section);              // 将ini文件中section的值赋给设备节点，此时section=bed
    } else if (strcmp(section, (*device_list)->device_name) != 0) {// （当前名字与节点名字不等就意味着来新节点了，需要申请空间赋值）此时第一个节点复制完毕，第二个节点名字不一样，所以需要创建新的内存空间重新赋值
        current = (struct gdevice *)malloc(sizeof(struct gdevice));
        memset(current, 0, sizeof(struct gdevice));
        strcpy(current->device_name, section);                     // 创建完毕就往里面写入section名字fan
        current->next_dev = *device_list;                          // 形成链表
        *device_list = current;                                    // 都在当前主函数空间内，不需要二级指针（此时device_list指针指向新创建的空间，此时内容为空，下面正好进行赋值）
        current = NULL;                                            // 赋值完成后，置为空，等待下一设备节点进来重新开辟空间
    }

    if (MATCH((*device_list)->device_name, "dev_name")) {          // 如果是属于此节点的变量就赋值给当前节点
        strcpy((*device_list)->dev_name, value);                   // 此时的value=/dev/gpio13
    } else if (MATCH((*device_list)->device_name, "key")) {        // 其实这个程序每一次打印一行内容都会执行一遍该函数，所以需要一个一个判断，是的话就赋值给节点存储起来
        sscanf(value, "%x", &((*device_list)->key));               // 以16进制的形式输出给key，scanf相当于value为标准输入，从终端获取值存放到后面的数据中
    } else if (MATCH((*device_list)->device_name, "thread")) {
        (*device_list)->thread = NULL;                             // 这边直接等于NULL即可，这个NULL是为了让其在receive里面判断程序走的路径，所以所有的节点都是NULL
    } else if (MATCH((*device_list)->device_name, "gpio_pin")) {
        (*device_list)->gpio_pin = atoi(value);
    } else if (MATCH((*device_list)->device_name, "gpio_mode")) {
        (*device_list)->gpio_mode = atoi(value);
    } else if (MATCH((*device_list)->device_name, "level_trig")) {
        (*device_list)->level_trig = atoi(value);
    }
    
    return 1;
}


int main(int argc, char* argv[])
{
    int i = 0;
    int cnt = 0;                        // 计数多少个控制节点
    unsigned char buffer[6] = {0x00, 0x00, 0x00, 0x00, 0X00, 0x00}; // 初始都给0
    int shmid;                          // 共享内存标识符
    char *shm;                          // 共享内存的地址空间
    pthread_t* tid = NULL;
    ctrl_info_t* ctrl_data = NULL;
    control* control_list = NULL;       // 控制链表
    gdevice* device_list = NULL;        // 设备链表
    control* current = NULL;            // 中转作用
    struct shared_use_st *shared = NULL;// 共享内存空间转为我想划分的地址存储数据（一个锁一个条件变量，还有个buffer存数据）

    face_init();                        // 人脸识别初始化
    myoled_init();                      // oled初始化

    // 创建共享内存
    shmid = shmget((key_t)1111, 1024, 0666|IPC_CREAT);
    if (-1 == shmid) {
        fprintf(stderr, "shmat failed\n");
        exit(EXIT_FAILURE);
    }

    // 将共享内存映射到当前进程的地址空间
    shm = shmat(shmid, NULL, 0);
    if ((void *)-1 == shm) {
        fprintf(stderr, "shmat failed\n");
        exit(EXIT_FAILURE);
    }
    shared = (struct shared_use_st*)shm;// 共享内存转化为我定义的结构体空间形式
    
    for (int i = 0; i < 6; i++) {
		shared->shm_buffer[i] = 0;      // 共享内存内的数据全部初始化为0，也可以用memcpy赋值
	}

    // 初始化锁和条件变量（都设置为共享的）
    pthread_mutexattr_t attr1;
    pthread_mutexattr_init(&attr1);
    pthread_mutexattr_setpshared(&attr1, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&shared->mutex, &attr1);

    pthread_condattr_t attr2;
    pthread_condattr_init(&attr2);
    pthread_condattr_setpshared(&attr2, PTHREAD_PROCESS_SHARED);
    pthread_cond_init(&shared->cond, &attr2);


    // 添加控制模块到控制链表中
    add_voice_to_ctrl_list(&control_list);          // 将语音控制模块加入控制链表
    add_socket_to_ctrl_list(&control_list);         // 将网络控制模块加入控制链表
    add_smoke_to_ctrl_list(&control_list);          // 将烟雾控制模块加入控制链表（驱动层面实现的）
    add_ds18b20_to_ctrl_list(&control_list);        // ds18b20温度检测模块（驱动实现）
    add_sr04_to_ctrl_list(&control_list);           // 超声波测距模块（驱动实现）
    add_receive_to_ctrl_list(&control_list);        // 接受线程，用来中转数据，将共享内存上的数据对应上要执行的设备线程
    add_hc01_to_ctrl_list(&control_list);           // 蓝牙模块，手机发信息操控

    // 去解析ini配置文件；传二级指针，在函数里面将链表传递好，不然就定义全局变量则不需要传参
    if (ini_parse("gdevice.ini", handler_gdevice, (void *)&device_list) < 0) {
        printf("Can't load 'gdevice.ini'\n");
        return 1;
    } 
    // 添加设备模块到设备链表
    add_living_to_ctrl_list(&device_list);          // 客厅灯（驱动移位寄存器实现）
//  add_bed_to_ctrl_list(&device_list);             // 卧室灯（驱动实现，由用户决定输入输出模式以及高低电平）
//  add_fan_to_ctrl_list(&device_list);             // 风扇（ini配置文件，在不修改源代码的情况下更改程序的行为，因此可以用于实现动态配置）
//  此时可以通过ini配置文件进行添加通用的设备链表，直接修改外部的ini文件即可，程序完全不需要动
    add_servo_to_ctrl_list(&device_list);           // 舵机模仿开门（驱动实现）
    add_sound_to_ctrl_list(&device_list);           // 语音播报
    add_face_to_ctrl_list(&device_list);            // 人脸识别（阿里云平台）
    add_tiktok_to_ctrl_list(&device_list);

//  print_devices_name(device_list);                        // 打印所有的设备链表中的名字


    // 传输给线程共享内存的标识，控制链表，设备链表，buffer存放数据
    ctrl_data = (ctrl_info_t*)malloc(sizeof(ctrl_info_t));  // 申请内存地址，存放这些数据；在线程函数内不需要malloc是因为，将指针指向了传过去的地址值，可以直接对这块地址数据进行提取
    ctrl_data->shmid = shmid;                               // 进行初始化赋值
    ctrl_data->ctrl_phead = control_list;
    ctrl_data->dev_phead = device_list;
    memcpy(ctrl_data->handle_buffer, buffer, sizeof(buffer));

    // 初始化所有控制节点，且计数多少个节点
    current = control_list;
    while (NULL != current) {
        current->init;                    // 将控制链表中的所有控制节点全部初始化
        current = current->next_ctrl;
        cnt++;                            // 计数有多少个控制节点，方便下面知道创建多大的线程标识符空间
    }

    // 依次循环给每一个控制节点创建线程
    tid = (pthread_t*)malloc(sizeof(pthread_t) * cnt);              // 为线程标识数组分配内存，存放每一个线程标识tid
    current = control_list;                                         // current重新指回头节点，依次把每一个控制节点都去创建线程
    while (NULL != current) {
        pthread_create(&tid[i], NULL, current->get, ctrl_data);     // 创建线程，并且将数据传输给线程
        current = current->next_ctrl;
        i++;
    }

    printf("The smart home has been opened, please tell [ Xiao Mo Tong Xue ]!\n");
    /* 查找节点
    print_controls_name(control_list);                                      // 打印所有的控制链表中的名字
    control* find_control1 = find_control_by_name(control_list, "voice");   // 根据名字找控制节点
    printf("%s\n", find_control1->dev_name);
    */

//    while (1);                                    // while主线程不退出
    
    // 下面都是结束操作
    for (i = 0; i < cnt; i++) {                     // 释放tid申请的内存
        pthread_join(tid[i], NULL);                 // 等待线程结束（主程序会卡在这里等待第一个voice线程退出）
        free(tid);
        current->final();                           // 关闭打开的文件描述符
        current->next_ctrl;
    }
    face_final();

    shmctl(shmid, IPC_RMID, NULL);                  // 删除共享内存段，谁创建谁销毁
    
    free(ctrl_data);                                // 释放控制数据（控制链表的头节点和共享内存的标识符）

    free_device_list(device_list);                  // 销毁设备链表
    free_control_list(control_list);                // 销毁控制链表

    return 0;
}
