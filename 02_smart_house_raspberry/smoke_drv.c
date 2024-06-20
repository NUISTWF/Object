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

#define smoke       20
#define buzz        16

static int major = 0;
static struct class *smoke_class;
int irqno = 0;

static DECLARE_WAIT_QUEUE_HEAD(wq);
static int event_flag = 0;

static irqreturn_t irq_handler(int irqno, void *dev_id)
{
	int value = 0;

	value = gpio_get_value(smoke);
	if (value == 0) {
		gpio_direction_output(buzz, 1);
		event_flag = 1;  				// 设置事件标志位
        wake_up_interruptible(&wq);  	// 唤醒等待队列
		return IRQ_HANDLED;
	} else if (value == 1) {
		gpio_direction_output(buzz, 0);
		event_flag = 0;  				// 清除事件标志位
		return IRQ_HANDLED;
	}
	return IRQ_HANDLED;
}


static ssize_t smoke_read(struct file *file, char __user *buf, size_t count, loff_t *offset)
{
	int err = -1;

    wait_event_interruptible(wq, event_flag != 0);  // 等待事件发生
	err = copy_to_user(buf, &event_flag, 4);

    return count;
}


static struct file_operations smoke_drv = {
	.owner	 = THIS_MODULE,
	.read    = smoke_read,
};


static int __init smoke_init(void)
{
	gpio_request(smoke, "smoke");           // 申请引脚
    gpio_request(buzz, "smoke");
    gpio_direction_output(buzz, 0); 		// 初始化就默认高电平给蜂鸣器，高电平有效
	gpio_direction_input(smoke);

    irqno = gpio_to_irq(smoke);             // 烟雾输入引脚设置为中断引脚
    request_irq(irqno, irq_handler, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "smoke", &irqno);     // 申请中断，上升沿下降沿触发

	major = register_chrdev(0, "smoke", &smoke_drv);
	smoke_class = class_create(THIS_MODULE, "smoke_class");
	device_create(smoke_class, NULL, MKDEV(major, 0), NULL, "smoke");

	return 0;
}


static void __exit smoke_exit(void)
{
	device_destroy(smoke_class, MKDEV(major, 0));
	class_destroy(smoke_class);
	unregister_chrdev(major, "smoke");
    gpio_free(smoke);							// 释放已经请求的 GPIO 资源
	gpio_free(buzz);
}


module_init(smoke_init);
module_exit(smoke_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR ("wf");
MODULE_DESCRIPTION ("this is smoke module");
