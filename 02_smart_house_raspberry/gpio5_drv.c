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

#define NAME        "gpio5"
#define gpio5       5

static int major = 0;
static struct class *gpio5_class;


static ssize_t gpio5_drv_write(struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
    int data = 0;

    copy_from_user(&data, buf, size);
    if (data == 1) {
        gpio_set_value(gpio5, 1);               // 写入1代表输出高电平
    } else if (data == 0) {
        gpio_set_value(gpio5, 0);               // 写入0代表输出低电平
    }

    return size;    
}


static long gpio5_ioctl(struct file *filp, unsigned int command, unsigned long arg)
{
	if (command == 1) {
        gpio_direction_output(gpio5, 1);        // 如果是写入1就代表输出模式，0就是输入模式
    } else if (command == 0) {
        gpio_direction_input(gpio5);
    }

	return 0;
}


static struct file_operations gpio5_drv = {
	.owner	= THIS_MODULE,
	.write  = gpio5_drv_write,
    .unlocked_ioctl = gpio5_ioctl,
};

static int __init gpio5_init(void)
{
	gpio_request(gpio5, NAME);
	
	major = register_chrdev(0, NAME, &gpio5_drv);
	gpio5_class = class_create(THIS_MODULE, "gpio5_class");
	device_create(gpio5_class, NULL, MKDEV(major, 0), NULL, NAME);
	return 0;
}

static void __exit gpio5_exit(void)
{
	device_destroy(gpio5_class, MKDEV(major, 0));
	class_destroy(gpio5_class);
	unregister_chrdev(major, "gpio5");
}

module_init(gpio5_init);
module_exit(gpio5_exit);

MODULE_LICENSE("GPL");