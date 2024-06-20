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
    oled_putstrto(&disp, 0, 10, "current control");
    disp.font = font2;
    switch(buffer[2])
    {
        case 0x41: {
            if (buffer[3] == 0x00)
                oled_putstrto(&disp, 0, 30, "living   open!");
            else
                oled_putstrto(&disp, 0, 30, "living  close!");
            break;
        }
        case 0x42: {
            if (buffer[3] == 0x00)
                oled_putstrto(&disp, 0, 30, "bedroom  open!");
            else
                oled_putstrto(&disp, 0, 30, "bedroom close!");
            break;
        }
        case 0x43: {
            if (buffer[3] == 0x00)
                oled_putstrto(&disp, 0, 30, "fan      open!");
            else
                oled_putstrto(&disp, 0, 30, "fan     close!");
            break;
        }
        case 0x44: {
                oled_putstrto(&disp, 0, 30, "Prepare  open!");
            break;
        }
        case 0x45: {
                oled_putstrto(&disp, 0, 30, "smoke   alarm!");
            break;
        }
        case 0x49: {                                            // 舵机
                oled_putstrto(&disp, 0, 30, "socket   open!");
            break;
        }
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