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

#define NAME "servo"
#define Servo 21

static int major = 0;
static struct class *servo_class;

static void servo_udelay(int us)
{
	u64 time = ktime_get_ns();						// 测试内核当前精确的时间，ns
	while ((ktime_get_ns() - time) < us*1000);		// 480us*1000 = 480000ns, 当内核过了480us后就退出循环
}

static ssize_t servo_drv_write(struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
	int err;
	int cmd = 0;

	int minPulse = 500;  // 舵机位置最小脉冲宽度（0度）  0.5ms
    int maxPulse = 2500; // 舵机位置最大脉冲宽度（180度）2.5ms
    int pulseWidth;

	err = copy_from_user(&cmd, buf, size);
	pulseWidth = (maxPulse - minPulse) * cmd / 4;
	printk("%d\n", pulseWidth);

	gpio_set_value(Servo, 1); 
	servo_udelay(pulseWidth);		// 1000->45
	gpio_set_value(Servo, 0);
	servo_udelay(50000);			// 延时50ms
	
	return 0;
}

static struct file_operations servo_drv = {
	.owner	 = THIS_MODULE,
	.write   = servo_drv_write,
};

static int __init servo_init(void)
{
	gpio_request(Servo, NAME);
	gpio_direction_output(Servo, 1);
	major = register_chrdev(0, NAME, &servo_drv);
	servo_class = class_create(THIS_MODULE, "servo_class");
	device_create(servo_class, NULL, MKDEV(major, 0), NULL, NAME);
	return 0;
}

static void __exit servo_exit(void)
{
	device_destroy(servo_class, MKDEV(major, 0));
	class_destroy(servo_class);
	unregister_chrdev(major, "servo");
}

module_init(servo_init);
module_exit(servo_exit);

MODULE_LICENSE("GPL");