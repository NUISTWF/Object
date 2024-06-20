#ifndef PROTO_H__
#define PROTO_H__

#define NAMESIZE        32
#define DATASIZE        1024 // Because the number of bytes transferred by the history record is large
#define SERVERIP        "192.168.88.155"
#define SERVERPORT      "2000"
#define DATABASE        "dictionary.db"
#define DISTIONARY_PATH "../../dictionary.txt"

enum {
    R = 1,                  // R: user - register
    L,                      // L: user - login
    Q,                      // Q: user - query
    H,                      // H: user - history
    O                       // O: user - logout
};

typedef struct {            // client and server communicate proto
    int type;               // type (R: register; L: login; Q: query; H: history)
    char name[NAMESIZE];    // username
    char data[DATASIZE];    // password or query word
} MSG;

#endif