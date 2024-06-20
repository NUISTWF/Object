#ifndef PROTO_H__
#define PROTO_H__

#define  RCVPORT   "1234"
#define  RCVPORT_CH1   "1235"
// #define  SNDPORT   "1999"
#define  CLIENT_RCV_PORT  "6666"
#define  RCVFROM_CLIENT_PORT  "1998"

#define NAMESIZE  11
#define NAMEMAX  50000

// struct msg_st{
    
//     uint32_t math;
//     uint32_t chinese;
//     uint8_t name[];
// }__attribute__((packed));
struct rcv_arg{ // cam to server
    int chn; 
};

struct send_arg{ // server to client
    int chn; 
    struct sockaddr_in client_addr; // port in this addr is client main proc`s, port of client fork vlc should be  same as CLIENT_RCV_PORT
    int stream;
};

struct recordVedio {
    int chn;
    int vedio;
};

#endif