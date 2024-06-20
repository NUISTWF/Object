#include<stdlib.h>
#include<stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include<pthread.h>
#include<ctype.h>
#include<sys/syscall.h>
#include<string.h>
#include <net/if.h>
#include <sys/time.h>
#include <time.h>


#include "proto.h"

char rbuf_ch0[NAMEMAX];
char rbuf_ch1[NAMEMAX];

int vedio;
int recordTime = 0;

void* thread_function(void* arg)
{
    for (int i = recordTime; i > 0; i--) {
        printf("%d\n", i);
        sleep(1); // 等待1秒
    }
    
    pthread_exit(NULL);
}

int main(int argc, char* argv[]){

    if(argc < 3){
        printf("./vlc_stream 0(1280*720)/1(768*448) record/stream 60/15\n");
        exit(0);
    }

    struct rcv_arg argg;
    int err;
    pthread_t ch0,ch1;
    if(*argv[1] == '0')
        argg.chn = 0;
    else    
        argg.chn = 1;
    int fd,sfd;

    if (argc == 4)
        recordTime = atoi(argv[3]);
    else
        recordTime = 60;

    if(strcmp(argv[2], "record") == 0) {
        vedio = 0;
    } else if (strcmp(argv[2], "stream") == 0) {
        vedio = 1;
    } else {
        vedio = 2;
    }

    if (vedio == 0 && argg.chn == 0) {
        printf("Welcome to record, the resolution is 1280*720 video recording for %d seconds\n", recordTime);
    } else if (vedio == 0 && argg.chn == 1) {
        printf("Welcome to record, the resolution is 768*448 video recording for %d seconds\n", recordTime);
    } else {
        printf("Welcome to watch the live broadcast!");
    }
    //printf("vedio = %d, recordTime = %d\n", vedio, recordTime);

    struct sockaddr_in laddr,raddr;
    
    socklen_t raddr_len;
    sfd = socket(AF_INET,SOCK_DGRAM,0/*IPPROTO_UDP*/);
    if(sfd<0){
        perror("socket()\n");
        pthread_exit(NULL);
    }
    memset(&laddr, 0, sizeof(laddr));
    laddr.sin_family = AF_INET;
    raddr.sin_family = AF_INET;
    inet_pton(AF_INET,"0.0.0.0",&laddr.sin_addr);/*能够匹配任何地址(当前地址)*/
    inet_pton(AF_INET,"192.168.1.1",&raddr.sin_addr);
    laddr.sin_port = htons(0);
    raddr.sin_port = htons(atoi(RCVFROM_CLIENT_PORT)); 
    if(bind(sfd,(void*)&laddr,sizeof(laddr))<0)/*因为bind要的是一个sock_addr,而ipv4规定的地址结构体 是sockaddr_in,所以用void转换一下  */
    {
        perror("bind()");
        pthread_exit(NULL);
    }

    int optval = 1;
    socklen_t optlen = sizeof(optval);
    setsockopt(sfd,SOL_SOCKET,SO_BROADCAST,&optval,optlen);
    // int ff = 0;
    int rcvlen;
    int curlen;
    raddr_len = sizeof(raddr);
    
    // struct sockaddr_in check;
    // socklen_t check_len;
    // memset(&check,0,check_len);

    socklen_t laddr_len = sizeof(laddr);
    if(getsockname(sfd, (struct sockaddr*)&laddr, &laddr_len)){
        perror("getsockname\n");
        exit(1);
    }
    // printf("laddr_port:%d", ntohs(laddr.sin_port));
    struct recordVedio record;
    record.chn = argg.chn;
    record.vedio = vedio;   // vedio 为0是录视频

    //printf("chn = %d, vedio = %d\n", record.chn, record.vedio);
    sendto(sfd, &record, 128, 0, (void*)&raddr, raddr_len); // 0是录视频，告诉服务器，把流发给我这个客户端就行，不要发给加1的端口
    

    // printf("client send address\n");
    // 1就是直播
    if (vedio == 1) {
        char port_str[6];
        char vlc_command[256];

        snprintf(port_str, sizeof(port_str), "%d", ntohs(laddr.sin_port + 1));
        snprintf(vlc_command, sizeof(vlc_command), "udp://@:%s", port_str);

        int pid;
        pid = fork();

        if(pid == 0){ // child
            
            execl("/snap/bin/vlc","vlc",vlc_command,NULL);
            perror("execl()");
            exit(1);
        }
        else{
            exit(0);
        }
    } else if (vedio == 0) {
        // 录视频，收到服务器发来的文件流
        printf("Start Recording!\n");
        char wbuf_ch0[NAMEMAX];
        FILE *ps_file = NULL;
        ps_file = fopen("./1test.264", "w+"); 
        time_t start, end;  
        double diff;  
        time(&start); // 获取当前时间 
        struct sockaddr_in raddrr;//server`s main port and thread port is not same.so need another addr.
        socklen_t raddr_lenn;
        raddr_lenn = sizeof(raddrr);
        int curlen;
        pthread_t tid0;
	    pthread_create(&tid0, NULL, thread_function, NULL);

        while (1) {
            curlen = recvfrom(sfd, wbuf_ch0, NAMEMAX, 0, (struct sockaddr *)&raddrr, &raddr_lenn);
            fwrite(wbuf_ch0, sizeof(char), curlen, ps_file);

            time(&end);
            diff = difftime(end, start);
            if (diff > recordTime) { // 如果时间差大于60秒  
                break; // 退出循环  
            }
        }
        printf("Recording ends in %d seconds!\n", recordTime);

        if (system("/home/fh8862/work/listen5/live/testProgs/testOnDemandRTSPServer") == -1) {  
            perror("system failed");
            exit(EXIT_FAILURE);
        }
    }


    
    // free(ipstr);
    // close(fd);
    close(sfd);
}
