#include "control.h"

static int serial_fd = -1;


// 初始化，打开串口设备，得到文件描述符
static int voice_init(void)
{
	if ((serial_fd = serialOpen("/dev/serial0", 115200)) == -1)
        return -1;
	return serial_fd;  // 代表了打开串口的句柄
}


// 关闭文件描述符
static void voice_final(void)
{
    if (-1 != serial_fd) {
        close(serial_fd);
        serial_fd = -1;
    }
}


// 语音唤醒线程（获取语音指令，将数据发到消息队列中）
static void *voice_get(void *arg)           // msgid应该是线程参数arg传参
{
    struct shared_use_st *shared = NULL;
    unsigned char buffer[6] = {0x00, 0x00, 0x00, 0x00, 0X00, 0x00};
    int len = 0;
    int i = 0;
    int shmid;
    char *shm = NULL;
    ctrl_info_t *ctrl_info= NULL;

    if (NULL != arg)
        ctrl_info = (ctrl_info_t *)arg;     // 传值，此时ctrl_info就包含了链表中的该控制项和共享内存标识符

    if (NULL != ctrl_info) {
        shmid = ctrl_info->shmid;           // 取出共享内存标识符
    }
    
    serial_fd = voice_init();
    if (-1 == serial_fd) {
        pthread_exit(NULL);
    }

    shm = shmat(shmid, NULL, 0);            // 将共享内存连接到当前进程的地址空间，此时shm就是共用的地址空间
    if ((void *)-1 == shm) {
        fprintf(stderr, "shmat failed\n");
        exit(EXIT_FAILURE);
    }
    // 设置共享内存
    shared = (struct shared_use_st *)shm;

    while (1) {      // 该线程循环不退出，一直读取语音命令，将获取到的命令数据发给消息队列
        len = read(serial_fd, buffer, 6);
//        printf("%s|%s|%d: %d, 0x%X, 0x%X, 0x%X, 0x%X, 0x%X, 0x%X\n", __FILE__, __func__, __LINE__, len, buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5]);
        if (buffer[0] != 0xAA) {            // 只要读取到第1个数据是0xAA才是收到唤醒指令，往下执行
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
    
    shmdt(shm);     // 将共享内存从当前线程的地址空间分离
    pthread_exit(NULL);
}


// 初始化语音模块, 并且将其加入
void add_voice_to_ctrl_list(control** list) // 要是传入一级指针，只是将地址信息拷贝一份过来，让list也指向这个地址，最后这个函数执行结束，原来的地址值没发生变化
{
    control* node = (control*)malloc(sizeof(control));

    strcpy(node->ctrl_name, "voice");
    strcpy(node->dev_name, "/dev/serial0"); // ttyAMA0
    node->init = voice_init;
    node->final = voice_final;
    node->get = voice_get;
    node->set = NULL;

    node->next_ctrl = *list;
    *list = node;
}
