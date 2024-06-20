#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <unistd.h>  
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <netdb.h>  
#include <arpa/inet.h>  
#include <fcntl.h>  
#include <errno.h>

#define SERVER 	"192.168.1.3"     	// FTP服务器地址  
#define PORT 	"2000"              // FTP端口  
#define BUFFER_SIZE 1024  
  
int main(int argc, char *argv[])
{  
    int sockfd, n;  
    struct sockaddr_in serv_addr; 
    FILE *localFile;  
    char buffer[BUFFER_SIZE] = {0};  
    char temp[128] = {0};
    int err;

    if (argc < 1) {  
        fprintf(stderr, "Usage %s server filename(./c RETR 0h264.ps)\n", argv[0]);  
        exit(0);  
    }  
  
    // 创建socket  
    sockfd = socket(AF_INET, SOCK_STREAM, 0);  
    if (sockfd < 0) {  
        perror("ERROR opening socket");  
        exit(1);  
    }

  	memset(&serv_addr, 0, sizeof(struct sockaddr_in));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(PORT));
	inet_aton(SERVER, &serv_addr.sin_addr);
    // 连接到FTP服务器  
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {  
        perror("ERROR connecting");  
        exit(1);
    }

    // 设置接收超时  
    struct timeval tv;
    tv.tv_sec = 2;  // 秒
    tv.tv_usec = 0; // 微秒  
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1) {  
        perror("setsockopt failed");  
        exit(EXIT_FAILURE);  
    }

    while (1) {
        printf("input:\n");
        gets(temp);
        //getchar();
        //printf("command = %s\n", temp);
        if (strncmp(temp, "RETR ", 5) == 0) {
            // 发送RETR命令以及我想要获取的文件名字
            char *p;
	        p = strtok(temp, " ");
	        p = strtok(NULL, " ");
            sprintf(buffer, "RETR %s", p);
            write(sockfd, buffer, strlen(buffer));

            // 打开本地文件用于写入获取到的文件内容，相当于复制了一份
            localFile = fopen(p, "wb");   // 以二进制写模式打开文件  
            if (localFile == NULL) {
                perror("ERROR opening local file");
                exit(1);
            }
            int cnt = 0;
            // 读取服务器发送的数据并写入到本地文件  
            while (1) { 
                n = read(sockfd, buffer, BUFFER_SIZE);
                fwrite(buffer, 1, n, localFile); // 将读取的数据写入到本地文件
                if (n <= 0) {   // 小BUG很奇怪
                    err = errno;  
                    if (err == EAGAIN || err == EWOULDBLOCK) {  
                        // 如果是EAGAIN或EWOULDBLOCK，通常表示非阻塞模式下没有数据可读
                        // 设置的是接收超时，所以这里应该处理超时情况
                        printf("File download completed!\n");
                        break;  
                    }
                }
            }
            fclose(localFile);
        } else if (strncmp(temp, "cd", 2) == 0) {
            char *p;
	        p = strtok(temp, " ");
	        p = strtok(NULL, " ");
            sprintf(buffer, "cd %s", p);
            write(sockfd, buffer, strlen(buffer));
        } else if (strcmp(temp, "ls") == 0) {
            sprintf(buffer, "ls");
            write(sockfd, buffer, strlen(buffer));

            read(sockfd, buffer, BUFFER_SIZE);
            printf("---------------------------\n");
            printf("\n%s\n", buffer);   // 读取到的内容打印在桌面
            printf("---------------------------\n");
        }
        memset(temp, 0, 128);
        memset(buffer, 0, 128);
    }
    close(sockfd);
  
    return 0;  
}
