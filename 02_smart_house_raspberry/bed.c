#include "gdevice.h"
#include "control.h"
#include "op.h"

void add_bed_to_ctrl_list(gdevice** list)
{
    gdevice* node = (gdevice*)malloc(sizeof(gdevice));

    strcpy(node->device_name, "bed");
    strcpy(node->dev_name, "/dev/gpio13");
    node->key = 0x42;               // 0x42卧室灯 0x43风扇 0x44人脸识别 0x45烟雾报警模块
    node->thread = NULL;            // 没有对应的线程函数，要赋值为空，会在receive函数内判断
    node->gpio_pin = 13;            // 控制的gpio引脚（这个其实没必要，已经配置好了13号引脚的驱动了）
    node->gpio_mode = 1;            // 输入输出模式（1代表输出，0代表输入）
    node->level_trig = 1;           // (高电平触发)写一个电平触发状态0/1，1代表高电平触发，0代表低电平触发；不然没法一劳永逸，低电平触发的话，通用的高电平就不成立了

    node->next_dev = *list;
    *list = node;
}
