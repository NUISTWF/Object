#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <ctype.h>
#include <sys/syscall.h>
#include <string.h>
#include <sys/time.h>

#include "proto.h"

// #define  IPSTRSIZE  40

int write_size_ch0;
int write_size_ch1;


char rbuf_ch0[NAMEMAX];
char rbuf_ch1[NAMEMAX];

char wbuf_ch0[NAMEMAX];
char wbuf_ch1[NAMEMAX];

// int write_tag_ch0 = 0;
// int write_tag_ch1 = 0;

int customer_num_ch0 = 0;
int write_num_ch0 = 0;
int customer_num_ch1 = 0;
int write_num_ch1 = 0;

int newjoin_tag_ch0;
int newjoin_tag_ch1; //one client new join





static pthread_mutex_t lock_ch0 = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t lock_ch1 = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond_ch0_maker = PTHREAD_COND_INITIALIZER;
static pthread_cond_t cond_ch1_maker = PTHREAD_COND_INITIALIZER;
static pthread_cond_t cond_ch0_customer = PTHREAD_COND_INITIALIZER;     // 客户端
static pthread_cond_t cond_ch1_customer = PTHREAD_COND_INITIALIZER;




void *rcv_stream(void *arg){
    int fd,sfd;
    struct rcv_arg argg = *(struct rcv_arg*)arg;
    // if(argg.chn == 0){
    //     fd = open("/mnt/nfs_share/server/vi", O_RDWR);
    // }  
    // else{
    //     fd = open("/mnt/nfs_share/server/vi_ch1", O_RDWR);
    // }
        
    struct sockaddr_in laddr,raddr;
    
    socklen_t raddr_len;
    sfd = socket(AF_INET,SOCK_DGRAM,0/*IPPROTO_UDP*/);
    if(sfd<0){
        perror("socket()\n");
        pthread_exit(NULL);
    }
    laddr.sin_family = AF_INET;
    inet_pton(AF_INET,"0.0.0.0",&laddr.sin_addr);/*能够匹配任何地址(当前地址)*/
    // ================区分通道几，对应不同的端口号，两个端口号分别不同的接受线程在运行接受板子发来的视频流数据包================
    if(argg.chn == 0)
        laddr.sin_port = htons(atoi(RCVPORT));
    else    
        laddr.sin_port = htons(atoi(RCVPORT_CH1));
    if(bind(sfd,(void*)&laddr,sizeof(laddr))<0)/*因为bind要的是一个sock_addr,而ipv4规定的地址结构体 是sockaddr_in,所以用void转换一下  */
    {
        perror("bind()");
        pthread_exit(NULL);
    }

    int optval = 1;
    socklen_t optlen = sizeof(optval);
    setsockopt(sfd,SOL_SOCKET,SO_BROADCAST,&optval,optlen);
    int ff = 0;
    int rcvlen;
    int curlen;

    // struct timeval now;
    // struct timespec outtime;


    if(argg.chn == 0){
        while(1){
            // printf("ff = %d\n", ff);
            // fflush(stdout);
            rcvlen = 0;
            while(rcvlen < NAMEMAX-1316){
                curlen = recvfrom(sfd,rbuf_ch0 + rcvlen,1316,0,(void*)&raddr,&raddr_len);
                // inet_ntop(AF_INET,&raddr.sin_addr,ipstr,IPSTRSIZE);
                rcvlen += curlen;
            }
            pthread_mutex_lock(&lock_ch0);
            while(write_num_ch0 < customer_num_ch0){ //if the num of customer who already write equil to customer num, maker should make new msg.
                pthread_cond_wait(&cond_ch0_maker, &lock_ch0);
                pthread_mutex_lock(&lock_ch0); //prepare for fake awake, make sure while() condition in the lock.
            }

            write_num_ch0 = 0;              // new msg has no customer write yet.
            memcpy(wbuf_ch0, rbuf_ch0, (size_t)rcvlen);
            write_size_ch0 = rcvlen;
            pthread_cond_broadcast(&cond_ch0_customer);
            pthread_mutex_unlock(&lock_ch0);
            ff++;
        }
    }
    else{
        while(1){
            // printf("ff = %d\n", ff);
            // fflush(stdout);
            rcvlen = 0;
            while(rcvlen < NAMEMAX-1316){
                curlen = recvfrom(sfd,rbuf_ch1 + rcvlen,1316,0,(void*)&raddr,&raddr_len);
                // inet_ntop(AF_INET,&raddr.sin_addr,ipstr,IPSTRSIZE);
                rcvlen += curlen;
            }
            pthread_mutex_lock(&lock_ch1);
            while(write_num_ch1 < customer_num_ch1){
                pthread_cond_wait(&cond_ch1_maker, &lock_ch1);
                pthread_mutex_lock(&lock_ch1);
            }
            // write_tag_ch1 = 1;
            // write(fd, rbuf_ch1, rcvlen);
            write_num_ch1 = 0; 
            memcpy(wbuf_ch1, rbuf_ch1, (size_t)rcvlen);
            write_size_ch1 = rcvlen;
            pthread_cond_broadcast(&cond_ch1_customer);
            pthread_mutex_unlock(&lock_ch1);
            
            // ff++;
        }
    }
    
    // free(ipstr);
    close(fd);
    close(sfd);
    pthread_exit(NULL);
}


void *write_stream(void *arg){
    struct send_arg argg = *(struct send_arg*)arg;

    int sfd;
    socklen_t addr_len;
    sfd = socket(AF_INET,SOCK_DGRAM,0);
    if(sfd<0){
        perror("socket()\n");
        pthread_exit(NULL);
    }

    struct sockaddr_in laddr;   //only send, dont rcv, dont need local addr.
    
    laddr.sin_family = AF_INET;
    inet_pton(AF_INET,"0.0.0.0",&laddr.sin_addr);/*能够匹配任何地址(当前地址)*/
    laddr.sin_port = htons(0); // local port random, no need to rcv, only send.

    if(bind(sfd,(void*)&laddr,sizeof(laddr))<0)/*因为bind要的是一个sock_addr,而ipv4规定的地址结构体 是sockaddr_in,所以用void转换一下  */
    {
        perror("bind()");
        pthread_exit(NULL);
    }

    int optval = 1;
    socklen_t optlen = sizeof(optval);
    setsockopt(sfd,SOL_SOCKET,SO_BROADCAST,&optval,optlen);


    if(argg.chn == 0){
        pthread_mutex_lock(&lock_ch0); 
        newjoin_tag_ch0 = 1;
        if(customer_num_ch0 > 1)  // in case first customer cant send signal to maker
            pthread_cond_wait(&cond_ch0_customer, &lock_ch0);  
        pthread_mutex_unlock(&lock_ch0); 
    }
    else{
        pthread_mutex_lock(&lock_ch1); 
        newjoin_tag_ch1 = 1;
        if(customer_num_ch1 > 1)
            pthread_cond_wait(&cond_ch1_customer, &lock_ch1);  
        pthread_mutex_unlock(&lock_ch1); 
    }
    
    addr_len = sizeof(argg.client_addr);
    int curget_tag = 0;
    int ff= 0 ;
    // int write_size_ch00, write_size_ch11;//
    
    while(1){
        if(argg.chn == 0){
            pthread_mutex_lock(&lock_ch0); 
            if(write_num_ch0 >= customer_num_ch0){ // if all customer has got, tell maker to make, tell curget_tag new round begin.
                if(newjoin_tag_ch0 == 1){
                    // printf("customer_num_ch0++\n");
                    customer_num_ch0++;
                    write_num_ch0++;//this round has already finish, so make write_num_ch0 == customer_num_ch0
                    newjoin_tag_ch0 = 0;
                }       
                pthread_cond_signal(&cond_ch0_maker);   
                pthread_cond_wait(&cond_ch0_customer, &lock_ch0);  
                pthread_mutex_unlock(&lock_ch0);
                curget_tag = 0;
                continue;
            }
            pthread_mutex_unlock(&lock_ch0);
            if(curget_tag == 0){ 
                // ====================发送给连接过来的客户端=============================
                sendto(sfd, wbuf_ch0, write_size_ch0, 0, (void*)&argg.client_addr, addr_len);
                
                
                // printf("server send size:%d\n", write_size_ch0);
                // printf("write_num_ch0:%d\n customer_num_ch0:%d\n", write_num_ch0, customer_num_ch0);
                // fflush(stdout);
                ff++;
                curget_tag = 1;                                            
                pthread_mutex_lock(&lock_ch0);
                write_num_ch0++;
                pthread_mutex_unlock(&lock_ch0);
            }

        }
        else{
            pthread_mutex_lock(&lock_ch1); 
            if(write_num_ch1 >= customer_num_ch1){ // if all customer has got, tell maker to make, tell curget_tag new round begin.
                if(newjoin_tag_ch1 == 1){
                    customer_num_ch1++;
                    write_num_ch1++;//this round has already finish, so make write_num_ch1 == customer_num_ch1
                    newjoin_tag_ch1 = 0;
                }      
                pthread_cond_signal(&cond_ch1_maker);   
                pthread_cond_wait(&cond_ch1_customer, &lock_ch1);  
                pthread_mutex_unlock(&lock_ch1);
                curget_tag = 0;
                continue;
            }
            pthread_mutex_unlock(&lock_ch1);
            if(curget_tag == 0){ 
                // pthread_mutex_lock(&lock_ch1);
                // write_size_ch11 = write_size_ch1;
                // pthread_mutex_unlock(&lock_ch1);
                sendto(sfd, wbuf_ch1, write_size_ch1, 0, (void*)&argg.client_addr, addr_len); //wbuf could be used by all send thread. 
                                                            //if in the lock, there will be no difference with sigle thread with for().
                curget_tag = 1;                                            
                pthread_mutex_lock(&lock_ch1);
                write_num_ch1++;
                pthread_mutex_unlock(&lock_ch1);
            }
        }
    }
}


int main(){
    // int fd,fd_ch1;
    // int sfd,sfd_ch1;

    int err;
    pthread_t ch0,ch1;

    struct rcv_arg arg_ch0, arg_ch1;
    arg_ch0.chn = 0;
    arg_ch1.chn = 1;
    err = pthread_create(&ch0, NULL, rcv_stream, (void*)&arg_ch0);
    if(err){
        perror("pthread_create()\n");
        exit(1);
    }
    err = pthread_create(&ch1, NULL, rcv_stream, (void*)&arg_ch1);
    if(err){
        perror("pthread_create()\n");
        exit(1);
    }
    ////////////////////////////////////////////////////////////////////// rcv client`s request
    struct sockaddr_in laddr,raddr;
    socklen_t raddr_len = sizeof(raddr);
    memset(&raddr,0,raddr_len);
    int sfd;
    sfd = socket(AF_INET,SOCK_DGRAM,0/*IPPROTO_UDP*/);
    if(sfd<0){
        perror("socket()\n");
        pthread_exit(NULL);
    }
    laddr.sin_family = AF_INET;
    inet_pton(AF_INET,"0.0.0.0",&laddr.sin_addr);/*能够匹配任何地址(当前地址)*/
    // 定义的这个服务器的端口号 1998 
    laddr.sin_port = htons(atoi(RCVFROM_CLIENT_PORT));

    if(bind(sfd,(void*)&laddr,sizeof(laddr))<0)/*因为bind要的是一个sock_addr,而ipv4规定的地址结构体 是sockaddr_in,所以用void转换一下  */
    {
        perror("bind()");
        pthread_exit(NULL);
    }
    int optval = 1;
    socklen_t optlen = sizeof(optval);
    setsockopt(sfd,SOL_SOCKET,SO_BROADCAST,&optval,optlen);
    
    //int tmpp; // save request chn.
    struct recordVedio recordTmp;
    pthread_t c[10];
    int nn = 0;
    struct send_arg argg;
    struct send_arg argg1;
    while(nn < 10){
        // 阻塞等待连接
        err = recvfrom(sfd, &recordTmp, 128, 0, (void*)&raddr, &raddr_len);
        if (recordTmp.vedio == 0) {
            printf("vedio!!!\n");
        } else {
            printf("stream!!!\n");
            raddr.sin_port = raddr.sin_port+1;
        }
        if( err > 0 && (recordTmp.chn == 1 || recordTmp.chn == 0)){ // client`s request include chn,ip.
            argg.chn = recordTmp.chn;
            argg.client_addr = raddr;
            argg.stream = 1;
            printf("client`s vlc port: %d\nrcvlen:%d\ntmpp = %d\n", ntohs(raddr.sin_port), err, recordTmp.chn);
            // fflush(stdout);
            // ===================argg里面保存了我客户端想要的通道以及对应客户端的ip端口等信息=======================
            err = pthread_create(c + nn, NULL, write_stream, (void*)&argg);
            if(err){
                perror("pthread_create()\n");
                exit(1);
            }
            nn++;
        }
    }

    pthread_join(ch0, NULL);
    pthread_join(ch1, NULL);

    
    exit(0);
}