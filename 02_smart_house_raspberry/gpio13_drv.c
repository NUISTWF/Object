#include "acpi/acoutput.h"
#include "asm-generic/errno-base.h"
#include "asm-generic/gpio.h"
#include "asm/gpio.h"
#include "asm/uaccess.h"
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/stat.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/tty.h>
#include <linux/kmod.h>
#include <linux/gfp.h>
#include <linux/gpio/consumer.h>
#include <linux/platform_device.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/fcntl.h>
#include <linux/timer.h>

#define NAME        "gpio13"
#define gpio13      13

static int major = 0;
static struct class *gpio13_class;


static ssize_t gpio13_drv_write(struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
    int data = 0;

    copy_from_user(&data, buf, size);
    if (data == 1) {
        gpio_set_value(gpio13, 1);              // 写入1代表输出高电平
    } else if (data == 0) {
        gpio_set_value(gpio13, 0);              // 写入0代表输出低电平
    }

    return size;    
}


static long gpio13_ioctl(struct file *filp, unsigned int command, unsigned long arg)
{
	if (command == 1) {
        gpio_direction_output(gpio13, 1);   // 如果是写入1就代表输出模式，0就是输入模式
    } else if (command == 0) {
        gpio_direction_input(gpio13);
    }

	return 0;
}


static struct file_operations gpio13_drv = {
	.owner	= THIS_MODULE,
	.write  = gpio13_drv_write,
    .unlocked_ioctl = gpio13_ioctl,
};

static int __init gpio13_init(void)
{
	gpio_request(gpio13, NAME);
	
	major = register_chrdev(0, NAME, &gpio13_drv);
	gpio13_class = class_create(THIS_MODULE, "gpio13_class");
	device_create(gpio13_class, NULL, MKDEV(major, 0), NULL, NAME);
	return 0;
}

static void __exit gpio13_exit(void)
{
	device_destroy(gpio13_class, MKDEV(major, 0));
	class_destroy(gpio13_class);
	unregister_chrdev(major, "gpio13");
}

module_init(gpio13_init);
module_exit(gpio13_exit);

MODULE_LICENSE("GPL");