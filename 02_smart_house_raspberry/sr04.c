#include "control.h"

int sr04_fd;
#define CMD_TRIG 100

// 初始化，打开串口设备，得到文件描述符
static int sr04_init(void)
{
    sr04_fd = open("/dev/sr04", O_RDWR);

	return sr04_fd;
}


static void *sr04_get(void *arg)
{
    unsigned char buffer[6] = {0xAA, 0x55, 0x00, 0x00, 0X55, 0xAA};
    int data = 0;
    int val = 0;
    struct shared_use_st *shared = NULL;
    int shmid;
    char *shm = NULL;
    ctrl_info_t *ctrl_info= NULL;

    ctrl_info = (ctrl_info_t *)arg;
    shmid = ctrl_info->shmid;

    shm = shmat(shmid, NULL, 0);
    shared = (struct shared_use_st *)shm;       // 设置共享内存

    sr04_fd = sr04_init();

    while (1) {
        sleep(10);                              // 每隔10s获取一次距离
        ioctl(sr04_fd, CMD_TRIG);
		if (read(sr04_fd, &data, 4) == 4) {
            val = data*17/1000000;
            printf("get distance: %dcm\n", val);
        } else {
            printf("get distance err\n");
            val = -1;                           // 这样识别错误就不会传输开门0x44的数据
        }
        if ((val <= 100) && (val >= 0)) {       // 当距离大于0且小于100cm即在1m内的时候触发人脸识别线程
            buffer[2] = 0x44;
            pthread_mutex_lock(&shared->mutex);
            memcpy(shared->shm_buffer, buffer, sizeof(buffer));         // 往共享内存的地址上写数据
            pthread_cond_signal(&shared->cond);
            pthread_mutex_unlock(&shared->mutex);
        }
        val = 0;                                // 每次都清0
        data = 0;   
    }
    
    pthread_exit(NULL);
}


// 初始化sr04模块, 并且将其加入
void add_sr04_to_ctrl_list(control** list)
{
    control* node = (control*)malloc(sizeof(control));

    strcpy(node->ctrl_name, "sr04");
    strcpy(node->dev_name, "/dev/sr04");
    node->init = sr04_init;
    node->get = sr04_get;

    node->next_ctrl = *list;
    *list = node;
}
