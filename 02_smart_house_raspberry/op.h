#ifndef __OP_H
#define __OP_H

#include <stdio.h>
#include <wiringPi.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wiringSerial.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <time.h>
#include <pthread.h>

#include "control.h"
#include "gdevice.h"


gdevice *find_gdevice_by_key(gdevice *list, unsigned char key);

void free_control_list(control* factory);
void free_device_list(gdevice* factory);

control *find_control_by_name(control* list, char* ctrl_name);
gdevice *find_device_by_name(gdevice* list, char* dev_name);

void print_controls_name(control* factory);
void print_devices_name(gdevice* factory);


#endif