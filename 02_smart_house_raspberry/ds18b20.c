#include "control.h"

int ds18b20_fd;


// 初始化，打开串口设备，得到文件描述符
static int ds18b20_init(void)
{
    ds18b20_fd = open("/dev/ds18b20", O_RDWR);

	return ds18b20_fd;
}


static void *ds18b20_get(void *arg)
{
    unsigned char buffer[6] = {0xAA, 0x55, 0x00, 0x00, 0X55, 0xAA};
    int buf[2];
    struct shared_use_st *shared = NULL;
    int shmid;
    char *shm = NULL;
    ctrl_info_t *ctrl_info= NULL;

    ctrl_info = (ctrl_info_t *)arg;
    shmid = ctrl_info->shmid;

    shm = shmat(shmid, NULL, 0);
    shared = (struct shared_use_st *)shm;       // 设置共享内存

    ds18b20_fd = ds18b20_init();

    while (1) {
        sleep(15);
        read(ds18b20_fd, buf, 8);
        printf("Temperature: %d.%d\n", buf[0], buf[1]);
        buffer[2] = 0x48;
        buffer[3] = buf[0];
        pthread_mutex_lock(&shared->mutex);
        memcpy(shared->shm_buffer, buffer, sizeof(buffer));     // 往共享内存的地址上写数据
        pthread_cond_signal(&shared->cond);
        pthread_mutex_unlock(&shared->mutex);
    }
    
    pthread_exit(NULL);
}


// 初始化ds18b20模块, 并且将其加入
void add_ds18b20_to_ctrl_list(control** list)
{
    control* node = (control*)malloc(sizeof(control));

    strcpy(node->ctrl_name, "ds18b20");
    strcpy(node->dev_name, "/dev/ds18b20");
    node->init = ds18b20_init;
    node->get = ds18b20_get;

    node->next_ctrl = *list;
    *list = node;
}
