#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <unistd.h>  
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include <fcntl.h>  

#define PORT 2000
#define BUFFER_SIZE 1024  
  
void send_response(int sockfd, const char *response)
{  
    write(sockfd, response, strlen(response));  
}  
  
void retr_command(int sockfd, const char *filename)
{  
    FILE *file = fopen(filename, "rb");  // 打开需要下载的文件
    if (!file) {
        send_response(sockfd, "550 Requested action not taken. File unavailable.\r\n");  
        return;
    }

    char buffer[BUFFER_SIZE];
    size_t bytesRead;
  
    // 发送文件内容
    while ((bytesRead = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {   
        write(sockfd, buffer, bytesRead);
    }  

    fclose(file);  // 读写完毕后关闭该文件 
}

void cd_command(int sockfd, const char *directory)
{
    chdir(directory); // 改变我当前的目录到指定目录去
}

void ls_command(int sockfd, const char *data)
{
    FILE *fp = popen(data, "r"); // 创建子进程执行ls命令，对返回的文件流进行读取，读取到的数据就是ls后的文件列表
    char buffer[BUFFER_SIZE];
    size_t bytesRead;

    bytesRead = fread(buffer, 1, BUFFER_SIZE, fp);
	write(sockfd, buffer, bytesRead);			// 对socketfd也就是对应的客户端发数据

    pclose(fp);
}

int main()
{  
    int sockfd, newsockfd;  
    struct sockaddr_in serv_addr, cli_addr;  
    socklen_t clilen;  
    int n_read;

    // 创建socket  
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {  
        perror("ERROR opening socket");  
        exit(1);
    }  
  
    // 绑定socket到端口  
	memset(&serv_addr, 0, sizeof(struct sockaddr_in));
    serv_addr.sin_family = AF_INET;  
    serv_addr.sin_addr.s_addr = INADDR_ANY;  
    serv_addr.sin_port = htons(PORT);  
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {  
        perror("ERROR on binding");  
        exit(1);
    }  
  
    // 监听连接  
    listen(sockfd, 5);  

    // 接受连接
	clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);  
    if (newsockfd < 0) {  
        perror("ERROR on accept");  
        exit(1);  
    }  
    
    while (1) {
        // 读取命令 
        char buffer[BUFFER_SIZE];  
        n_read = read(newsockfd, buffer, BUFFER_SIZE - 1);   // 将读取到的命令放到buffer中，知道了客户是想获取RETR哪个文件
        buffer[BUFFER_SIZE - 1] = '\0';
        printf("buffer = %s\n", buffer);
        if (n_read == 0) {
			printf("client quit\n");
			break;
		}
        // 假设命令格式是 "RETR filename"
        if (strncmp(buffer, "RETR ", 5) == 0) {		// 如果传过来的是RETR则代表需要下载文件
            char filename[256];
            char *p;
	        p = strtok(buffer, " ");
	        p = strtok(NULL, " ");
            sprintf(filename, "%s", p);
            printf("filename = %s\n", filename);
            retr_command(newsockfd, filename);
        } else if (strncmp(buffer, "cd ", 3) == 0) {
            char filename[256];
            char *p;
	        p = strtok(buffer, " ");
	        p = strtok(NULL, " ");
            sprintf(filename, "%s", p);
            printf("filename = %s\n", filename);
            cd_command(newsockfd, filename);
        } else if (strncmp(buffer, "ls", 2) == 0) {
            memset(buffer, 0, sizeof(buffer));
            memcpy(buffer, "ls", 2);
            printf("buffer = %s\n", buffer);
            ls_command(newsockfd, buffer);
        } else {
            send_response(newsockfd, "502 Command not implemented.\r\n"); 
        }
    }
    close(newsockfd);  
    close(sockfd);  
    return 0;  
}
