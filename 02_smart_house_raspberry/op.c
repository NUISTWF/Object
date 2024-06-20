#include "op.h"
#include "control.h"
#include "gdevice.h"


// 根据key值(buffer[2])查找设备节点
gdevice *find_gdevice_by_key(gdevice *list, unsigned char key)
{
    gdevice *current = list;

    if (NULL == current) {
        return NULL;
    } else {
        while (current != NULL) {
            if ((current->key) == key) {
                return current;
            }
            current = current->next_dev;
        }
        return NULL;
    }
}


// 控制链表的销毁
void free_control_list(control* factory)
{
    control* current = factory;
    while (current->next_ctrl != NULL) {
        control* new = current->next_ctrl;
        free(current);
        current = new;
    }
    free(current);
    printf("control list free success!\n");
}


// 设备链表的销毁
void free_device_list(gdevice* factory)
{
    gdevice* current = factory;
    while (current->next_dev != NULL) {
        gdevice* new = current->next_dev;     // new 用来保存下一个的地址位置，销毁了就找不到了
        free(current);
        current = new;
    }
    free(current);
    printf("device list free success!\n");
}


// 根据控制名字查找控制节点
control *find_control_by_name(control* list, char* ctrl_name)
{
    control *current = list;
    if (current == NULL) {
        return NULL;
    } else {
        while (current != NULL) {
            if (!strcmp(current->ctrl_name, ctrl_name)) {
                return current;
            }
            current = current->next_ctrl;
        }
        return NULL;
    }
}


// 根据设备名字查找设备节点
gdevice *find_device_by_name(gdevice* list, char* device_name)
{
    gdevice *current = list;
    if (current == NULL) {
        return NULL;
    } else {
        while (current != NULL) {
            if (!strcmp(current->device_name, device_name)) {
                return current;
            }
            current = current->next_dev;
        }
        return NULL;
    }
}


// 打印控制工厂内所有控制的名字
void print_controls_name(control* factory)
{
    control* current = factory;
    while (current != NULL) {
        printf("%s\n", current->ctrl_name);
        current = current->next_ctrl;
    }
    return;
}


// 打印设备工厂内所有设备的名字
void print_devices_name(gdevice* factory)
{
    gdevice* current = factory;
    while (current != NULL) {
        printf("%s\n", current->device_name);
        current = current->next_dev;
    }
    return;
}
