// 通信协议

#ifndef _HIK_PROTO_H
#define _HIK_PROTO_H

#ifdef __cplusplus
extern "C" {
#endif

#define SERVER_PORT         "8888"      // server port
#define MAX_CLIENTS         10          // connect client numbers

#define FILE_NAME_SIZE      128
#define BUFFER_SIZE         1024
#define SQL_BUFFER_SIZE     (3 * BUFFER_SIZE)
#define DATA_INFO           "wechat.db"
#define QUEUE_NAME          "/my_queue"

enum {
    R = 1,                  // R: user - register
    L,                      // L: user - login
    O,                      // O: user - logout
    A,                      // A: user - add friend
    Q,                      // Q: user - query friendlist
    D,                      // D: user - delete friend
    C,                      // C: user - choose friend chat box
    S,                      // S: user - send msg to friend
    F,                      // F: user - send file to friend
    E,                      // R: user - query chat record
    T                       // T: user - delete chat record
};

typedef struct {            // client and server communicate proto
    int type;               // type (R: register; L: login; Q: query; H: history; O: logout)
    char name[BUFFER_SIZE]; // username
    char data[BUFFER_SIZE]; // password or send data
} MSG;

#ifdef __cplusplus
}
#endif

#endif