#include "asm-generic/gpio.h"
#include "asm/delay.h"
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

#define trig       23
#define echo       24
#define CMD_TRIG  100

static int major = 0;
static struct class *sr04_class;
int irqno = 0;
struct timer_list key_timer;

static DECLARE_WAIT_QUEUE_HEAD(wq);
static int event_flag = 0;
static int data = 0;

// 中断程序
static irqreturn_t irq_handler(int irq, void *dev_id)
{
	static ktime_t rising_time = 0;	    // static修饰就会保存这个数值，下一次再进来就不会重新初始化执行这一行代码
	ktime_t time;

	int val = gpio_get_value(echo);
	if (val) {		                    // 触发中断即获取到高电平了已经，此时获取echo引脚的值就是1；开始计时
		rising_time = ktime_get_ns();
		del_timer(&key_timer);          // 删除定时器（防止模块废了）
	} else {		                    // 下降沿执行else内的代码
		if (rising_time == 0)
			return IRQ_HANDLED;

		/* 下降沿记录结束时间, 并计算时间差, 计算距离 */
		time = ktime_get_ns() - rising_time;            // 计算时间差
		rising_time = 0;
		data = time;		            // 时间保存到全局变量中

		wake_up_interruptible(&wq);				        // 中断唤醒等待队列（唤醒read休眠等待的时候，唤醒poll定时等待的时候）
	}

	return IRQ_HANDLED;
}


static ssize_t sr04_read(struct file *file, char __user *buf, size_t count, loff_t *offset)
{
	int err = -1;

    wait_event_interruptible(wq, data != 0);  // 等待事件发生
	err = copy_to_user(buf, &data, 4);

    return count;
}


static long sr04_ioctl(struct file *filp, unsigned int command, unsigned long arg)
{
	// send trig 
	switch (command) {
		case CMD_TRIG: {
			// 发出触发信号
			gpio_set_value(trig, 1);	        // 初始化该引脚的时候已经给低电平了，此时直接拉高即可
			udelay(20);							// 持续20us的高电平
			gpio_set_value(trig, 0);            // 再拉低
			// 启动定时器 - mod修改定时器就是，删除定时器，设置定时时间，添加到内核定时器队列，三步的汇总
			mod_timer(&key_timer, jiffies + msecs_to_jiffies(50)); // 修改定时器，定时50ms 
		}
	}

	return 0;
}


static void sr04_timer_func(struct timer_list  *timer)
{
	data = -1;									    	// 往数据内输入一个值（定时函数主要就是定时50ms，若还没启动echo触发中断，则模块存在问题）
	wake_up_interruptible(&wq);					        // 唤醒在休眠的read
}


static struct file_operations sr04_drv = {
	.owner	 = THIS_MODULE,
	.read    = sr04_read,
    .unlocked_ioctl = sr04_ioctl,
};


static int __init sr04_init(void)
{
	gpio_request(trig, "trig");             // 申请引脚
    gpio_request(echo, "echo");
    gpio_direction_output(trig, 0); 		// 初始化
	gpio_direction_input(echo);

    irqno = gpio_to_irq(echo);              // echo接收信号引脚设置为中断引脚
    request_irq(irqno, irq_handler, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "echo", &irqno);     // 申请中断，上升沿下降沿触发
	
	// setup_timer
    timer_setup(&key_timer, sr04_timer_func, 0);	// 上面这行的定时器版本较低，现在高版本内核只支持这个timer_setup

	major = register_chrdev(0, "sr04", &sr04_drv);
	sr04_class = class_create(THIS_MODULE, "sr04_class");
	device_create(sr04_class, NULL, MKDEV(major, 0), NULL, "sr04");

	return 0;
}


static void __exit sr04_exit(void)
{
	device_destroy(sr04_class, MKDEV(major, 0));
	class_destroy(sr04_class);
	unregister_chrdev(major, "sr04");
    gpio_free(trig);						// 释放已经请求的 GPIO 资源
	gpio_free(echo);
}


module_init(sr04_init);
module_exit(sr04_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR ("wf");
MODULE_DESCRIPTION ("this is sr04 module");