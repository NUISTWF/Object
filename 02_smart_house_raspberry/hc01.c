// 蓝牙模块，通过usb连接串口使用，可以手机远程控制
// ls -l /dev/ttyU* 
#include "control.h"

static int hc01_fd = -1;


// 初始化，打开串口设备，得到文件描述符
static int hc01_init(void)
{
	if ((hc01_fd = serialOpen("/dev/ttyUSB0", 9600)) == -1)
        return -1;
	return hc01_fd;  // 代表了打开串口的句柄
}


// 关闭文件描述符
static void hc01_final(void)
{
    if (-1 != hc01_fd) {
        close(hc01_fd);
        hc01_fd = -1;
    }
}


// 蓝牙线程（获取手机通过蓝牙发过来的指令，将数据发到共享内存中）
static void *hc01_get(void *arg)
{
    struct shared_use_st *shared = NULL;
    unsigned char buffer[6] = {0x00, 0x00, 0x00, 0x00, 0X00, 0x00};
    char r_buf[32] = {'\0'};
    int len = 0;
    int i = 0;
    int shmid;
    char *shm = NULL;
    ctrl_info_t *ctrl_info= NULL;

    ctrl_info = (ctrl_info_t *)arg;

    shmid = ctrl_info->shmid;
    shm = shmat(shmid, NULL, 0);
    shared = (struct shared_use_st *)shm;

    hc01_fd = hc01_init();

    while (1) {
        len = read(hc01_fd, buffer, 6);
        if (buffer[0] != 0xAA) {      // 只要读取到第1个数据是0xAA才是收到正确指令，往下执行
            continue;
        }
        pthread_mutex_lock(&shared->mutex);
        if (len > 0) {
            if ((buffer[0] == 0xAA) && (buffer[1] == 0x55) && (buffer[4] == 0x55) && (buffer[5] == 0xAA)) {      
                memcpy(shared->shm_buffer, buffer, sizeof(buffer));   // 往共享内存的地址上写数据（传输数据）
                pthread_cond_signal(&shared->cond);
            }
            memset(buffer, 0, 6);                                     // 每次归0，循环重新读取命令数据
        }
        pthread_mutex_unlock(&shared->mutex);
    }
    
    shmdt(shm);
    pthread_exit(NULL);
}


// 初始化蓝牙模块, 并且将其加入
void add_hc01_to_ctrl_list(control** list)
{
    control* node = (control*)malloc(sizeof(control));

    strcpy(node->ctrl_name, "hc01");
    strcpy(node->dev_name, "/dev/ttyUSB2");
    node->init = hc01_init;
    node->final = hc01_final;
    node->get = hc01_get;
    node->set = NULL;

    node->next_ctrl = *list;
    *list = node;
}
