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

#define GPIO_LED 		(43)  		// 需要操作的gpio引脚

// static struct cdev my_cdev;                // 字符设备对象; struct cdev 是 Linux 内核中用于表示字符设备的结构体, 表示一个字符设备对象, 包含了字符设备的基本信息和操作接口
// static dev_t devno;                        // 字符设备号; 可以集成主次设备号
// static char *module_name = "led";    	  // 模块名

static struct class *led_class;  
static struct device *led_class_dev;

static dev_t devno;                     //设备号
static int major = 250;                 //主设备号
static int minor = 0;                   //次设备号
static char *module_name = "led";  		//模块名


static ssize_t led_write(struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{   
	int usercmd;

    copy_from_user(&usercmd, buf, size);
	if (usercmd == 1) {
		printk("set 1\n");
		gpio_direction_output(GPIO_LED, 1);
	} else if (usercmd == 0) {
		printk("set 0\n");
		gpio_direction_output(GPIO_LED, 0);
	} else {
        printk("undo\n");
    }

    return size;
}


static const struct file_operations led_fops =
{
	.owner = THIS_MODULE,
    .write = led_write,
};


static int __init led_init(void)
{   
	int ret = 0;
	volatile unsigned int *addr = NULL;

    // cdev_init(&my_cdev, &led_fops);
    // ret = alloc_chrdev_region(&devno, 0, 1, module_name);
	// if (ret < 0) {
    //     printk("alloc_chrdev_region error:%d\n", ret);
    //     cdev_del(&my_cdev);
    //     return ret;
	// }

    // ret = cdev_add(&my_cdev, devno, 1);
	// if (ret < 0) {
    //     printk("Failed to register character device error:%d\n", ret);
    //     unregister_chrdev_region(devno, 1);
	// 	cdev_del(&my_cdev);
    //     return ret;
	// } 
	
    devno = MKDEV(major, minor);                              //创建设备号
    ret   = register_chrdev(major, module_name, &led_fops);   //注册驱动  告诉内核，把这个驱动加入到内核驱动的链表中

    led_class = class_create(THIS_MODULE, "my_first_class");  //将设备节点与设备类 (class) 关联起来，以便系统可以根据设备类进行设备的管理和分类
    led_class_dev = device_create(led_class, NULL, devno, NULL, module_name);

	addr = (volatile unsigned int *)ioremap(0x04020068, 4);
	writel(0x01001000, addr);
	iounmap(addr);
	
	gpio_request(GPIO_LED, "led_gpio");
	gpio_direction_output(GPIO_LED, 1);

	printk("request gpio success!\n");
	printk ("led init\n");
	
	return 0;
}


static void __exit led_exit(void)
{
    // cdev_del(&my_cdev);								// 注销设备
    // unregister_chrdev_region(devno, 1);				// 释放设备号
	gpio_direction_output(GPIO_LED, 0); 			// 默认熄灭该led
	// gpio_free(GPIO_LED);							// 释放已经请求的 GPIO 资源

	device_destroy(led_class, devno);
    class_destroy(led_class);
    unregister_chrdev(major, module_name);  //卸载驱动

	printk ("led exit\n");
}


module_init(led_init);
module_exit(led_exit);


MODULE_LICENSE("GPL"); 
MODULE_AUTHOR ("NUISTWF");
MODULE_DESCRIPTION ("this is led module");
