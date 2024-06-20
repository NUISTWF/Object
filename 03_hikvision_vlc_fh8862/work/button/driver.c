#include <linux/types.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/module.h>
#include <linux/kernfs.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/kmod.h>
#include <linux/gfp.h>
#include <asm/pgtable.h>
#include <linux/mm.h>
#include <linux/slab.h>

// 第一个是停止启动；第二个是翻转视频画面；第三个是开始取流
#define GPIO_STREAM 		(24)//(24) 				// 从充电口往外依次的三个按键
#define GPIO_FLIP         	(25)
#define GPIO_RECORD       	(56)

#define GPIO_STREAM_LED 	(43)  				// 灯（从左往右D7，D6，D2，D3）
#define GPIO_FLIP_LED       (42)				// 两个名字反一下即可
//#define GPIO_FLIP_RECORD    (52)

int irqno = 0;
int irqno1 = 0;
int irqno2 = 0;

int status = 0;

int values = 1;		
int flip = 0;	
int streamArg = 0;
static char *kernel_buf;
static int bufsiz = 1024*8;
int flicker = 0;
int ledflicker = 1;

int w_data = 0;
static struct cdev my_cdev;                // 字符设备对象; struct cdev 是 Linux 内核中用于表示字符设备的结构体, 表示一个字符设备对象, 包含了字符设备的基本信息和操作接口
static dev_t devno;                        // 字符设备号; 可以集成主次设备号
static char *module_name = "cam_ctrl";     // 模块名

static struct fasync_struct *async_queue;
static struct timer_list key_timer;
static struct timer_list key_timer1;
static DECLARE_WAIT_QUEUE_HEAD(wq);

static void key_timer_expire(unsigned long data)			// 定时器超时时要调用的回调函数，最后一次按键抖动结束，定时20ms后执行这串代码，获取稳定后的电平值
{
	int value;

	value = gpio_get_value(GPIO_STREAM);
	if (value == 1)
		return IRQ_HANDLED;
	if (values == 0) {
		values = 1;
	} else {
		values = 0;
	}
	gpio_direction_output(GPIO_STREAM_LED, values);
	kill_fasync(&async_queue, SIGIO, POLL_IN);
}


static irqreturn_t irq_handler(int irqno, void *dev_id)
{
	unsigned long delay = msecs_to_jiffies(20);
	mod_timer(&key_timer, jiffies + delay);				// 在当前时间的基础上延迟 20 毫秒触发名为 my_timer 的定时器

	return IRQ_HANDLED;	// IRQ_HANDLED 表示成功处理了中断，IRQ_NONE 表示未处理中断
}

static irqreturn_t irq_handler2(int irqno1, void *dev_id)
{
	int value;

	value = gpio_get_value(GPIO_FLIP);
	if (value == 1)
		return IRQ_HANDLED;

	if (flip == 0) {
		flip = 1;
	} else {
		flip = 0;
	}
	status = 1;
	wake_up_interruptible(&wq);
}

static void key_timer_expire1(unsigned long data)
{
	if (ledflicker == 0) {
		if (flicker == 1){
			flicker = 0;
			gpio_direction_output(GPIO_STREAM_LED, flicker);
		} else {
			flicker = 1;
			gpio_direction_output(GPIO_STREAM_LED, flicker);
		}

		unsigned long delay = msecs_to_jiffies(500);
		mod_timer(&key_timer1, jiffies + delay);
	}
}

static irqreturn_t irq_handler3(int irqno2, void *dev_id)
{
	int value;

	value = gpio_get_value(GPIO_RECORD);
	if (value == 1)
		return IRQ_HANDLED;

	flicker = 1;
	gpio_direction_output(GPIO_STREAM_LED, flicker);
	ledflicker = 0;
	unsigned long delay = msecs_to_jiffies(500);
	mod_timer(&key_timer1, jiffies + delay);

	strcpy(kernel_buf, "open");
}


static int hello_drv_mmap(struct file *file, struct vm_area_struct *vma)
{
	unsigned long phy = virt_to_phys(kernel_buf);
	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
	if (remap_pfn_range(vma, vma->vm_start, phy >> PAGE_SHIFT, vma->vm_end - vma->vm_start, vma->vm_page_prot)) {
		printk("mmap remap_pfn_range failed\n");
		return -ENOBUFS;
	}
	return 0;
}

int led_open(struct inode *inode, struct file *filp)
{
	printk(KERN_EMERG "open cam_ctrl\n");
    return 0;
}

int led_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static ssize_t led_read(struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
	wait_event_interruptible(wq, status != 0);
    copy_to_user(buf, &flip, size);
	status = 0;
    return size;
}

static ssize_t led_write(struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{   
    copy_from_user(&w_data, buf, size);
	if (w_data == 1) {
		ledflicker = 1;
		gpio_direction_output(GPIO_STREAM_LED, 1);
	}
    return size;
}

static int my_fasync(int fd, struct file *filp, int mode){
	printk(KERN_EMERG "combine button and process\n");
	return fasync_helper(fd, filp, mode, &async_queue);
}

static const struct file_operations led_fops =
{
	.owner = THIS_MODULE,
    .open = led_open,
    .read = led_read,
    .write = led_write,
    .release = led_release,
	.fasync = my_fasync,
	.mmap = hello_drv_mmap,
};

static int __init led_init(void)
{   
	int ret = 0;
	volatile unsigned int *addr = NULL;
	volatile unsigned int *addr2 = NULL;
	volatile unsigned int *addr3 = NULL;
	volatile unsigned int *addr4 = NULL;

	kernel_buf = kmalloc(bufsiz, GFP_KERNEL);
	strcpy(kernel_buf, "hello");

    cdev_init(&my_cdev, &led_fops);                        // 初始化字符设备结构体且将文件IO与字符设备cdev相关联，这个字符的io操作对应我定义的这些函数
    ret = alloc_chrdev_region(&devno, 0, 1, module_name);  // 动态申请设备号，firstminor设置为0表示从第一个次设备号开始分配，count设置为1表示只分配一个设备号（也可以给该主设备号分配两个次设备号）
	if (ret < 0) {
        printk("alloc_chrdev_region error:%d\n", ret);
        cdev_del(&my_cdev);                                // 注销设备
        return ret;
	}

    ret = cdev_add(&my_cdev, devno, 1);                    // （霸占一个次设备号）向内核注册一个字符设备对象，成功调用 cdev_add 后，字符设备对象就会被添加到内核的字符设备链表中，并且可以通过相应的设备文件进行访问
	if (ret < 0) {
        printk("Failed to register character device error:%d\n", ret);
        unregister_chrdev_region(devno, 1);				   // 出错就先释放设备号
		cdev_del(&my_cdev);								   // 再注销设备
        return ret;
	} 

	addr = (volatile unsigned int *)ioremap(0x04020068, 4);
	writel(0x01001000, addr);							   // 是一个在 Linux 内核中用于向硬件寄存器（Memory Mapped I/O）写入数据的函数；在驱动程序中，当需要向硬件设备的寄存器写入数据时，可以使用 writel 函数
	iounmap(addr);										   // 端口转为gpio模式后就释放地址映射
	addr4 = (volatile unsigned int *)ioremap(0x04020064, 4);
	writel(0x01001000, addr4);							   // 是一个在 Linux 内核中用于向硬件寄存器（Memory Mapped I/O）写入数据的函数；在驱动程序中，当需要向硬件设备的寄存器写入数据时，可以使用 writel 函数
	iounmap(addr4);

	addr2 = (volatile unsigned int *)ioremap(0x04020134, 4);
	writel(0x01001000, addr2);							   // 将这个地址的io口转为gpio模式(一个io口可能处在其他模式下，并不是gpio模式)
	iounmap(addr2);	
	
	addr3 = (volatile unsigned int *)ioremap(0x040200b4, 4);   // gpio56
	writel(0x1001000, addr3);
	iounmap(addr3);

	gpio_request(GPIO_STREAM_LED, "led_gpio"); 			   	   // 申请led的gpio
	gpio_request(GPIO_STREAM, "key_gpio");
	gpio_request(GPIO_FLIP, "key2_gpio");
	gpio_request(GPIO_RECORD, "key3_gpio");
	gpio_request(GPIO_FLIP_LED, "led2_gpio");

	gpio_direction_output(GPIO_STREAM_LED, 1); 				   // 初始化就默认亮该led
	gpio_direction_output(GPIO_FLIP_LED, 1);

	gpio_direction_input(GPIO_STREAM);
	gpio_direction_input(GPIO_FLIP_LED);
	gpio_direction_input(GPIO_FLIP);
	gpio_direction_input(GPIO_RECORD);

	irqno = gpio_to_irq(GPIO_STREAM);						   // 将次gpio引脚设为中断引脚，得到中断号
	request_irq(irqno, irq_handler, IRQF_TRIGGER_FALLING, "led", &my_cdev);		// 申请中断，下降沿触发

	irqno1 = gpio_to_irq(GPIO_FLIP);						   // 将次gpio引脚设为中断引脚，得到中断号
	request_irq(irqno1, irq_handler2, IRQF_TRIGGER_FALLING, "key2", &my_cdev);

	irqno2 = gpio_to_irq(GPIO_RECORD);						   // 将次gpio引脚设为中断引脚，得到中断号
	request_irq(irqno2, irq_handler3, IRQF_TRIGGER_FALLING, "key3", &my_cdev);


	setup_timer(&key_timer, key_timer_expire, 0);
	//timer_setup(&key_timer, key_timer_expire, 0);
	key_timer.expires = ~0;				// 定时器的过期时间，即定时器何时触发（超时时间设为无穷大）
	add_timer(&key_timer);

	setup_timer(&key_timer1, key_timer_expire1, 0);
	//timer_setup(&key_timer, key_timer_expire, 0);
	key_timer1.expires = ~0;				// 定时器的过期时间，即定时器何时触发（超时时间设为无穷大）
	add_timer(&key_timer1);

	printk("request gpio success!\n");
	
	printk ("led init\n");
	
	return 0;
}

static void __exit led_exit(void)
{
    cdev_del(&my_cdev);								// 注销设备
    unregister_chrdev_region(devno, 1);				// 释放设备号
	gpio_direction_output(GPIO_STREAM_LED, 0); 			// 默认熄灭该led
	gpio_free(GPIO_STREAM_LED);							// 释放已经请求的 GPIO 资源
	gpio_free(GPIO_STREAM);
	gpio_free(GPIO_FLIP);
	gpio_free(GPIO_RECORD);
	gpio_free(GPIO_FLIP_LED);
	del_timer(&key_timer);
	kfree(kernel_buf);
	printk ("led exit\n");
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL"); 
MODULE_AUTHOR ("wangfei");
MODULE_DESCRIPTION ("this is led module");
