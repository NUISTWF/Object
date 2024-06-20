#include "gdevice.h"
#include "control.h"
#include "op.h"

void add_fan_to_ctrl_list(gdevice** list)
{
    gdevice* node = (gdevice*)malloc(sizeof(gdevice));

    strcpy(node->device_name, "fan");       // 名字没起到作用，可以通过名字查找到该节点
    strcpy(node->dev_name, "/dev/gpio5");
    node->key = 0x43;               // 0x43风扇
    node->thread = NULL;            // 没有对应的线程函数，要赋值为空，会在receive函数内判断
    node->gpio_pin = 5;             // 控制的gpio引脚（这个其实没必要，已经配置好了5号引脚的驱动了）
    node->gpio_mode = 1;            // 输入输出模式（1代表输出，0代表输入）
    node->level_trig = 0;           // 低电平触发（正极接在3v的电池上，引脚供电不够，不会转；或者可以连接继电器使用即可，因为继电器也是低电平闭合，5号引脚接在继电器的in上，风扇正极直接接到洞洞板的电池3v，负极接到继电器，等待继电器闭合，就连接到电池负极的洞洞板）

    node->next_dev = *list;
    *list = node;
}
