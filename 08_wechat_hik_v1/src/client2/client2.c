// 客户端代码

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <proto.h>

// 用互斥锁加条件变量来唤醒对应的子函数模块
pthread_mutex_t mutex_register = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_login = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_add_friend = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_delete_friend = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_query_friend = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_choose_friend = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_query_record = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_delete_record = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_logout = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t cond_register = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_login = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_add_friend = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_delete_friend = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_query_friend = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_choose_friend = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_query_record = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_delete_record = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_logout = PTHREAD_COND_INITIALIZER;

static int login_info = 1;          // 用来判断是否登录成功
static int open_chat_page_info = 1; // 用来判断是否打开聊天框
static int val = 0;                 // 全局变量（用来让线程通知主进程，服务器关闭了，我们也退出）

// 统一接受消息的线程
static void *recv_msg(void *arg)
{
    int server_fd = *(int *)arg;
    MSG rcvMsg;
    int valread = 0;
    int valwrite = 0;
    FILE *localFile;
    int open_flag = 0;
    char* author = NULL;
    char* filename = NULL;

    while (1)
    {
        // 目前也只能支持双方都打开聊天框后才能接收到消息
        //（想想怎么解决这个问题【想了一晚上，所有接受的recv都用一个客户端线程来接受，然后再区分】）
        memset(&rcvMsg, 0, sizeof(rcvMsg));
        // printf("Waiting for a message ...\n");
        valread = read(server_fd, &rcvMsg, sizeof(rcvMsg)); // 接受消息
        if (valread == -1)
        {
            printf("read thread error!\n");
        }
        if (valread == 0)   // 服务器结束客户端也退出
        {
            printf("server is quit!\n");
            printf("please enter any value to exit the client:");
            // 此处通知主进程也退出（联系一下进程间通信，最直接的就是用一个全局变量，当这边改变值之后，主进程检测到就退出）
            val = 1;
            break;
        }
        else // 这里代表接收到服务器的消息了，需要区分是S发的消息还是交互信息
        {
            // printf("recv msg bytes:%d and msg type:%d\n", valread, ntohl(rcvMsg.type));
            if (ntohl(rcvMsg.type) == S)  // 这是针对类型是S发的消息发过来的消息，就直接打印出来，实时接收消息
            {
                printf("your friend:[%s] --> message:[%s]\n", rcvMsg.name, rcvMsg.data);
            }
            else if (ntohl(rcvMsg.type) == F)  // 这里收到文件
            {
                // 此时文件发送的作者和文件名以及信息都保存在 rcvMsg 中；
                // 先获取到文件名字，再打开（创建）该文件，然后将内容写入
                // 这里面可以再搞一个线程去循环读取写入，文件过大可能不断循环收到包，现在先测试一次写入的情况
                // rcvMsg.name 数据格式是（发送人#文件名），rcvMsg.data 是文件内容包
                
                // printf("recv file!\n");  // 测试语句，能进到这里
                // printf("your rcvMsg.name:[%s] --> message:[%s]\n", rcvMsg.name, rcvMsg.data);
                author = strtok(rcvMsg.name, "#");
                filename = strtok(NULL, "#");
                // strcpy(filename, strtok(NULL, "#"));
                printf("your friend:[%s] --> file:[%s]\n", author, filename);

                localFile = fopen(filename, "w");
                if (localFile == NULL)
                {
                    printf("ERROR opening local file\n");
                    exit(EXIT_FAILURE);
                }
                valwrite = fwrite(rcvMsg.data, sizeof(char), valread - sizeof(rcvMsg.type) - sizeof(rcvMsg.name), localFile); // 将读取的数据写入到本地文件
                // printf("write %d bytes\n", valwrite);   // 用sizeof就小bug，会写入一堆奇怪的字符（这边要注意发出跟收到时候的读写字节大小！！！）
                fclose(localFile);
            }
            else  // 这边就都是交互的信息，分别去通知对应的子函数模块
            {
                switch (ntohl(rcvMsg.type))
                {
                    case R:
                        // printf("register success!\n");
                        printf("register (%s).\n", rcvMsg.data);
                        pthread_mutex_lock(&mutex_register);
                        pthread_cond_signal(&cond_register); // 通知其他等待条件的进程
                        pthread_mutex_unlock(&mutex_register);
                        break;
                    case L:
                        if (!strcmp(rcvMsg.data, "ok"))
                        {
                            login_info = 0;
                        }
                        else
                        {
                            printf("Username or password is incorrect!\n");
                            login_info = 1;
                        }
                        printf("login (%s).\n", rcvMsg.data);
                        pthread_mutex_lock(&mutex_login);
                        pthread_cond_signal(&cond_login);
                        pthread_mutex_unlock(&mutex_login);
                        break;
                    case A:
                        printf("add friend (%s)!\n", rcvMsg.data);
                        pthread_mutex_lock(&mutex_add_friend);
                        pthread_cond_signal(&cond_add_friend);
                        pthread_mutex_unlock(&mutex_add_friend);
                        break;
                    case D:
                        printf("delete friend (%s)!\n", rcvMsg.data);
                        pthread_mutex_lock(&mutex_delete_friend);
                        pthread_cond_signal(&cond_delete_friend);
                        pthread_mutex_unlock(&mutex_delete_friend);
                        break;
                    case Q:
                        printf("%s\n", rcvMsg.data);
                        pthread_mutex_lock(&mutex_query_friend);
                        pthread_cond_signal(&cond_query_friend);
                        pthread_mutex_unlock(&mutex_query_friend);
                        break;
                    case C:
                        printf("%s!\n", rcvMsg.data);
                        if (!strcmp(rcvMsg.name, "ok"))
                        {
                            open_chat_page_info = 0;        // 判断打开聊天框是否成功
                        }
                        else
                        {
                            printf("open chat page error!\n");
                            open_chat_page_info = 1;
                        }
                        pthread_mutex_lock(&mutex_choose_friend);
                        pthread_cond_signal(&cond_choose_friend);
                        pthread_mutex_unlock(&mutex_choose_friend);
                        break;
                    case E:
                        printf("[%s|%s]\n", rcvMsg.name, rcvMsg.data);
                        pthread_mutex_lock(&mutex_query_record);
                        pthread_cond_signal(&cond_query_record);
                        pthread_mutex_unlock(&mutex_query_record);
                        break;
                    case T:
                        printf("%s[%s]\n", rcvMsg.name, rcvMsg.data);
                        pthread_mutex_lock(&mutex_delete_record);
                        pthread_cond_signal(&cond_delete_record);
                        pthread_mutex_unlock(&mutex_delete_record);
                        break;
                    case O:
                        printf("user:%s and %s!\n", rcvMsg.name, rcvMsg.data);
                        pthread_mutex_lock(&mutex_logout);
                        pthread_cond_signal(&cond_logout);
                        pthread_mutex_unlock(&mutex_logout);
                        break;
                    default:
                        printf("server send local valid data type!\n");
                        break; // 退出后再次循环等待接受消息
                }
            }
        }
    }

    return ;
}

// 获取一行内容时，去除最后的换行符
static void remove_newline(char *str)
{
    size_t len = strlen(str);
    if (len > 0 && str[len - 1] == '\n')
    {
        str[len - 1] = '\0';
    }
}

// 注册函数
static void do_register(int server_fd)
{
    MSG cur;

    printf("register ...\n");
    printf("Please enter your username and password.\n");

    cur.type = htonl(R);    // 4 bytes need to be converted to network byte order

	scanf("%s", cur.name);
    scanf("%s", cur.data);

    printf("Information entered successfully and you usr:%s, password:%s.\n", cur.name, cur.data);

    // send() menu1 information
    if (send(server_fd, &cur, sizeof(cur), 0) < 0)
	{
        perror("send() menu1");
        exit(EXIT_FAILURE);
    }

    // 负责发，但是这边可以搞一个条件变量卡住，等待回复
    // recv() waiting for the server's response
    // if (recv(server_fd, &cur, sizeof(cur), 0) < 0)
	// {
    //     perror("recv() menu1");
    //     exit(EXIT_FAILURE);
    // }
    pthread_mutex_lock(&mutex_register);
    pthread_cond_wait(&cond_register, &mutex_register); // 休眠等待信号，有信号过来就加
    pthread_mutex_unlock(&mutex_register);

    // printf("register func return.\n"); // 调试语句，查看该锁是否解锁，且程序是否走到这里

    return ;
}

// 登录函数（登录成功之后要跑一个线程，一直可以接受服务器发来的消息）
static void do_login(MSG* msg, int server_fd)
{
    MSG cur;

    printf("login ...\n");
    printf("Please enter your username and password.\n");

    cur.type = htonl(L);
    scanf("%s", cur.name);
    scanf("%s", cur.data);
    *msg = cur;  	// 回填登录进去的个人信息（用户名和密码）

    // send() person information
    if (send(server_fd, &cur, sizeof(cur), 0) < 0)
	{
        perror("client send() person information");
        exit(EXIT_FAILURE);
    }

    // recv() waiting for the server's response
    // if (recv(server_fd, &cur, sizeof(cur), 0) < 0)
	// {
    //     perror("client recv() person information response");
    //     exit(EXIT_FAILURE);
    // }
    pthread_mutex_lock(&mutex_login);
    pthread_cond_wait(&cond_login, &mutex_login); // 休眠等待信号，有信号过来就加
    pthread_mutex_unlock(&mutex_login);

    // printf("login func return.\n");
}

// 注销账户
static void do_logout(int server_fd)
{
    MSG cur, rcvMsg;

    printf("logout ...\n");
    printf("Please enter your username and password.\n");

    cur.type = htonl(O);
    scanf("%s", cur.name);
    scanf("%s", cur.data);

    if (send(server_fd, &cur, sizeof(cur), 0) < 0)
	{
        perror("client send() history record!");
        exit(EXIT_FAILURE);
    }

    // memset(&rcvMsg, 0, sizeof(rcvMsg));
    // if (recv(server_fd, &rcvMsg, sizeof(rcvMsg), 0) < 0)
	// {
    //     perror("client recv() history record response.");
    //     exit(EXIT_FAILURE);
    // }
    pthread_mutex_lock(&mutex_logout);
    pthread_cond_wait(&cond_logout, &mutex_logout); // 休眠等待信号，有信号过来就加
    pthread_mutex_unlock(&mutex_logout);

    // printf("logout func return.\n");

    return ;
}

// 添加好友，主要就是输入好友用户名，然后发给服务器，服务器查询一下，如果有就给他添加到对应名字的
static void do_add_friend(MSG* msg, int server_fd)
{
    MSG cur;
    
    printf("add friend ...\n");
    printf("Please enter your friend's username.\n");
    
    cur.type = htonl(A);
    strcpy(cur.data, msg->name); // 将用户的名字赋值给data，这样服务器才知道将目标用户添加到谁的好友列表中
    scanf("%s", cur.name);

    if (send(server_fd, &cur, sizeof(cur), 0) < 0)
	{
        perror("client send() add friend!");
        exit(EXIT_FAILURE);
    }

    pthread_mutex_lock(&mutex_add_friend);
    pthread_cond_wait(&cond_add_friend, &mutex_add_friend); // 休眠等待信号，有信号过来就加
    pthread_mutex_unlock(&mutex_add_friend);

    // printf("add_friend func return.\n");

    return ;
}

// 查询好友列表，直接发送一个请求给服务器，服务器查询一下，然后返回给客户端
static void do_query_friendlist(MSG* msg, int server_fd)
{
    MSG cur, rcvMsg;

    printf("query friendlist ...\n");

    cur.type = htonl(Q);
    strcpy(cur.name, msg->name); // 查询好友列表不需要输入内容，直接查询显示出来（将自己当前用户的名字输入）
    strcpy(cur.data, "friendlist");

    if (send(server_fd, &cur, sizeof(cur), 0) < 0)
	{
        perror("client send() query friendlist!");
        exit(EXIT_FAILURE);
    }

    // memset(&rcvMsg, 0, sizeof(rcvMsg));
    // if (recv(server_fd, &rcvMsg, sizeof(rcvMsg), 0) < 0)
	// {
    //     perror("client recv() query friendlist response.");
    //     exit(EXIT_FAILURE);
    // }

    printf("The %s user's friends list:\n", cur.name);
    pthread_mutex_lock(&mutex_query_friend);
    pthread_cond_wait(&cond_query_friend, &mutex_query_friend); // 休眠等待信号，有信号过来就加
    pthread_mutex_unlock(&mutex_query_friend);

    // printf("query_friendlist func return.\n");

    return ;
}

// 删除好友
static void do_delete_friend(MSG* msg, int server_fd)
{
    MSG cur, rcvMsg;
    
    printf("delete friend ...\n");
    printf("Please enter your friend's username.\n");

    cur.type = htonl(D);
    strcpy(cur.name, msg->name); // 将用户的名字赋值给name，这样服务器才知道将目标用户从谁的好友列表中删除
    scanf("%s", cur.data);

    if (send(server_fd, &cur, sizeof(cur), 0) < 0)
	{
        perror("client send() delete friend!");
        exit(EXIT_FAILURE);
    }
    
    // memset(&rcvMsg, 0, sizeof(rcvMsg));
    // if (recv(server_fd, &rcvMsg, sizeof(rcvMsg), 0) < 0)
	// {
    //     perror("client recv() delete friend response.");
    //     exit(EXIT_FAILURE);
    // }

    pthread_mutex_lock(&mutex_delete_friend);
    pthread_cond_wait(&cond_delete_friend, &mutex_delete_friend); // 休眠等待信号，有信号过来就加
    pthread_mutex_unlock(&mutex_delete_friend);

    // printf("delete_friend func return.\n");
    
    return ;
}

// 选择好友，打开聊天框
static void do_choose_friend(MSG* msg, char* sendfriend, int server_fd)
{
    MSG cur, rcvMsg;
    
    printf("choose friend ...\n");
    printf("Please open your friend chat box.\n");

    cur.type = htonl(C);
    strcpy(cur.name, msg->name);
    scanf("%s", cur.data);  // 输入要聊天的好友
    strcpy(sendfriend, cur.data);

    if (send(server_fd, &cur, sizeof(cur), 0) < 0)
	{
        perror("client send() delete friend!");
        exit(EXIT_FAILURE);
    }
    
    // memset(&rcvMsg, 0, sizeof(rcvMsg));
    // if (recv(server_fd, &rcvMsg, sizeof(rcvMsg), 0) < 0)
	// {
    //     perror("client recv() delete friend response.");
    //     exit(EXIT_FAILURE);
    // }
    
    pthread_mutex_lock(&mutex_choose_friend);
    pthread_cond_wait(&cond_choose_friend, &mutex_choose_friend); // 休眠等待信号，有信号过来就加
    pthread_mutex_unlock(&mutex_choose_friend);

    // printf("choose_friend func return.\n");
}

// 发送信息或者文件
static void do_send_msg(MSG* msg, char* sendfriend, int server_fd)
{
    MSG cur;
    MSG rcvMsg;
    int choose = 0;
    char* sendname = sendfriend;  // sendname就是我想要发送的对象
    int valread = 0;
    FILE* file;
    char filename[BUFFER_SIZE] = {0};
    char buffer[BUFFER_SIZE];
    size_t bytesRead;
    int offset = 0;  // 用来在cur.name中存储对方名字和文件名字，中间以#隔开

    printf("send msg ...\n");
    printf("Please input quit[0], msg[1] or file[2].\n");

    scanf("%d", &choose);
    while (1)
    {
        if (choose == 0)
        {
            break;
        }
        if (choose == 1)
        {
            while (1)  // 本来的主进程就发信息
            {
                memset(&cur, 0, sizeof(cur));
                cur.type = htonl(S);
                strcpy(cur.name, sendname);     // 所发的好友名字
                printf("Please input msg [# --> quit]:");
                scanf("%s", cur.data);          // 输入要发送的消息
                if (strcmp(cur.data, "#") == 0) // 输入#代表退出聊天
                {
                    break;
                }
                // 往服务端发送数据
                if (send(server_fd, &cur, sizeof(cur), 0) < 0)  // 往服务端发送数据
                {
                    perror("client send() friend msg!");
                    exit(EXIT_FAILURE);
                }
            }
            return ;
        }
        else if (choose == 2)    // 2就是选择发文件（输入想要发送的文件名字）
        {
            while (1)  // 不断选择当前目录的任意文件发过去
            {
                memset(&cur, 0, sizeof(cur));
                cur.type = htonl(F);            // 代表发文件
                strcpy(cur.name, sendname);     // 所发的目标好友名字
                printf("Please input file name [# --> quit]:");
                scanf("%s", filename);          // 输入要发送的文件名字
                if (strcmp(filename, "#") == 0) // 输入#代表退出聊天
                {
                    break;
                }
                offset = strlen(sendname);
                strcpy(cur.name + offset, "#");  // 在好友名字后面加上#，方便服务端判断
                strcpy(cur.name + offset + 1, filename);  // 在#后面加上文件名字

                // 往服务端发送数据（将文件打开，内容发过去，文件会保存在服务器当前界面的目录下）
                file = fopen(filename, "rb");  // 打开需要发送的文件
                if (!file)
                {
                    printf("please input right file name!\n");
                    continue;
                }
                
                // 发送文件内容（读取到 cur.data 再发送）
                while ((bytesRead = fread(cur.data, sizeof(char), BUFFER_SIZE, file)) > 0)
                {
                    if (send(server_fd, &cur, sizeof(cur) - sizeof(cur.data) + bytesRead, 0) < 0)  // 往服务端发送数据
                    {
                        printf("client send() file to friend!");
                        exit(EXIT_FAILURE);
                    }
                }  

                fclose(file);  // 读写完毕后关闭该文件 
            }
            return ;
        }
        else
        {
            printf("Please choose right number.\n");
        }
    }
    
    return ;
}

// 查看聊天记录（需要输入自己的名字和好友的名字，查看我和谁的聊天记录）
static void do_query_chat_record(MSG* msg, int server_fd)
{
    MSG cur;
    
    printf("choose friend query chat record...\n");
    printf("Please input your friend name.\n");

    cur.type = htonl(E);
    strcpy(cur.name, msg->name);        // 用户自己的名字
    scanf("%s", cur.data);              // 输入想要查看的好友名字

    if (send(server_fd, &cur, sizeof(cur), 0) < 0)
	{
        perror("client send() query chat record friend!");
        exit(EXIT_FAILURE);
    }
    
    pthread_mutex_lock(&mutex_query_record);
    pthread_cond_wait(&cond_query_record, &mutex_query_record); // 休眠等待信号，有信号过来就加
    pthread_mutex_unlock(&mutex_query_record);

    // printf("query friend chat record func return.\n");
}

// 删除聊天记录
static void do_delete_chat_record(MSG* msg, int server_fd)
{
    MSG cur;
    
    printf("choose friend delete chat record...\n");
    printf("Please input your friend name.\n");

    cur.type = htonl(T);
    strcpy(cur.name, msg->name); // 用户自己的名字
    scanf("%s", cur.data);       // 输入想要删除记录的好友名字

    if (send(server_fd, &cur, sizeof(cur), 0) < 0)
	{
        perror("client send() query chat record friend!");
        exit(EXIT_FAILURE);
    }
    
    pthread_mutex_lock(&mutex_delete_record);
    pthread_cond_wait(&cond_delete_record, &mutex_delete_record); // 休眠等待信号，有信号过来就加
    pthread_mutex_unlock(&mutex_delete_record);

    // printf("delete friend chat record func return.\n");
}

// socket初始化
static int socket_init()
{
	int server_fd;
	struct sockaddr_in address;

	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1)
	{
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	memset(&address, 0, sizeof(struct sockaddr_in));
	address.sin_family = AF_INET;
	address.sin_port = htons(atoi(SERVER_PORT));
	address.sin_addr.s_addr = INADDR_ANY;            // 0.0.0.0
	if (connect(server_fd, (struct sockaddr *)&address, sizeof(struct sockaddr)) == -1)
	{
		perror("connect failed");
		exit(EXIT_FAILURE);
	}

	return server_fd;
}

// 主函数
int main(int argc, char **argv)
{
	int server_fd;
	int valread;
	char readBuf[BUFFER_SIZE];
	int menu1 = 0, menu2 = 0;
	int ret = -1;
	MSG msg;
    char sendfriend[BUFFER_SIZE];
    pthread_t tid;

	server_fd = socket_init();
    pthread_create(&tid, NULL, recv_msg, (void *)&server_fd);    // 客户端接受服务器消息的线程
    
    // 下面是跑着的主进程
	while (1)
	{
        printf("\n-------------------------------------------\n");
        printf("|                                         |\n");
        printf("|  1.register  2.login  3.quit  4.logout  |\n");
        printf("|                                         |\n");
        printf("-------------------------------------------\n");
        printf("Please select a number:\n");
        scanf("%d", &menu1);

        switch (menu1)
		{
            case 1:
                do_register(server_fd);
                break;
            case 2:
                do_login(&msg, server_fd);   // msg:Get personal information
                if (!login_info)
                {
                    // printf("login ok!\n");
                    goto next;      // 第一个页面登陆成功后跳转到第二个界面
                }
                // printf("Please reselect!\n"); // 调试信息
                break;
            case 3:
                close(server_fd);
                exit(EXIT_SUCCESS);
                break;
            case 4:
                do_logout(server_fd);  // 注销账户（不需要传用户名，注销什么账户是自己输入用户名以及密码）
                break;
            default:
                printf("Please enter valid data!\n");
                break;
        }
    }
	
next:   // login ok! 登录成功之后可以查询好友列表，添加或者删除好友，选择好友聊天，查询聊天记录，删除聊天记录
    while (1)
	{
        printf("\n--------------------------------------------------------------------------------\n");
        printf("|                                                                              |\n");
        printf("|  1.add friend  2.query friendlist  3.delete friend  4.choose friend send msg |\n");
		printf("|  5.query friend chat record    6.delete friend chat record   7.quit          |\n");
        printf("|                                                                              |\n");
        printf("--------------------------------------------------------------------------------\n");
        printf("Please select a number:\n");
        scanf("%d", &menu2);
        // 如果服务器退出，则正在跑的接受线程就结束，此时也要告知客户端，此时这里应该退出
        if (val == 1)
        {
            break;
        }

        switch (menu2)
		{
            case 1:
                do_add_friend(&msg, server_fd);
                break;
            case 2:  // 查询好友列表
                do_query_friendlist(&msg, server_fd); // msg:save record (personal information)
                // printf("query friendlist ok!\n");  // 调试信息
                break;
			case 3:
                do_delete_friend(&msg, server_fd);
                break;
            case 4:
                do_choose_friend(&msg, sendfriend, server_fd);  // sendfriend是为了拿到选择的朋友传给下面的send函数，知道发给谁
                if (!open_chat_page_info)
                {
                    do_send_msg(&msg, sendfriend, server_fd);   // 打开了聊天框再发信息
                }
                break;
            case 5:
                do_query_chat_record(&msg, server_fd);
                break;
            case 6:
                do_delete_chat_record(&msg, server_fd);
                break;
            case 7:
                close(server_fd);
                exit(EXIT_SUCCESS);
                break;
            default:
                printf("Please enter valid data!\n");
                break;
        }
    }

	printf("Byb!\n");

    close(server_fd);

	exit(EXIT_SUCCESS);
}