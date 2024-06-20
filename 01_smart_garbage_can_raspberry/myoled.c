#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include "oled.h"
#include "font.h"
// 包含头文件
#include "myoled.h"

static struct display_info disp;

int oled_show(void *arg) 
{
    unsigned char *buffer = (unsigned char *)arg;

    oled_putstrto(&disp, 0, 9+1, "This garbage is:");
    disp.font = font2;
    switch(buffer[2])
    {
        case 0x41:
            oled_putstrto(&disp, 0, 20, "dry waste");
            break;
        case 0x42:
            oled_putstrto(&disp, 0, 20, "wet waste");
            break;
        case 0x43:
            oled_putstrto(&disp, 0, 20, "recyclable waste");
            break;
        case 0x44:
            oled_putstrto(&disp, 0, 20, "hazardous waste");
            break;
        case 0x45:
            oled_putstrto(&disp, 0, 20, "recognition failed");
            break;
    }
    disp.font = font2;
    oled_send_buffer(&disp);
    return 0;
}

int myoled_init(void)
{
    int e;

    disp.address = OLED_I2C_ADDR;
    disp.font = font2;
    e = oled_open(&disp, "/dev/i2c-1");
    e = oled_init(&disp);

    return e;
}