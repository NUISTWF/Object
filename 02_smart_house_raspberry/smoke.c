#include "control.h"

static int fd = -1;

static int smoke_init()
{
    fd = open("/dev/smoke", O_RDWR);

    return fd;
}


// 关闭文件描述符
static void smoke_final(void)
{
    if (fd > 0)
        close(fd);
    fd = -1;
}


// 检测烟雾触发线程，当触发烟雾报警后发数据给共享内存
static void *smoke_get(void *arg) 
{
    unsigned char buffer[6] = {0xAA, 0x55, 0x00, 0x00, 0X55, 0xAA};
    unsigned char buf[1];
    int shmid;
    char *shm = NULL;
    ctrl_info_t *ctrl_info = NULL;
    struct shared_use_st *shared;

    ctrl_info = (ctrl_info_t *)arg;                 // 传值，此时ctrl_info就包含了链表中的该控制项和共享内存标识符
    shmid = ctrl_info->shmid;                       // 取出共享内存标识符

    shm = shmat(shmid, NULL, 0);                    // 将共享内存连接到当前线程的地址空间，此时shm就是共用的地址空间
    // 设置共享内存
    shared = (struct shared_use_st*)shm;

    fd = smoke_init();
    if (-1 == fd)
        pthread_exit(NULL);

    while (1) {
        read(fd, buf, 1);                           // 读取会阻塞，只要等到烟雾触发蜂鸣器后才会有数据
//	    printf("data: %d\n", buf[0]);
        pthread_mutex_lock(&shared->mutex);
        if (buf[0] == 1) {
            // 只要读出来是1就代表有烟雾，进行语音播报（往共享内存发数据）
            buffer[2] = 0x45;
            buffer[3] = 0x00;
            memcpy(shared->shm_buffer, buffer, sizeof(buffer));    // 往共享内存的地址上写数据（传输数据）
            pthread_cond_signal(&shared->cond);
        }
        pthread_mutex_unlock(&shared->mutex);
        sleep(5);                                   // 每隔5秒检测一次
    }

    shmdt(shm);                                     // 将共享内存从当前线程的地址空间分离
    close(fd);
    pthread_exit(NULL);
}


// 初始化烟雾报警模块, 并且将其加入控制链表
void add_smoke_to_ctrl_list(control** list)
{
    control* node = (control*)malloc(sizeof(control));

    strcpy(node->ctrl_name, "smoke");
    memset(node->dev_name, '\0', sizeof(node->dev_name));
    node->init = smoke_init;
    node->final = smoke_final;
    node->get = smoke_get;
    node->set = NULL;                                   // 没有用到的就置为NULL

    node->next_ctrl = *list;
    *list = node;
}
