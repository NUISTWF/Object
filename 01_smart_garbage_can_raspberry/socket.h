#ifndef __SOCKET_H
#define __SOCKET_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <netinet/tcp.h>

int socket_init(const char *port);
void client_KeepAlive(int newfd);

#endif