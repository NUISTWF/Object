#include <linux/fs.h>        //file_operations声明
#include <linux/module.h>    //module_init  module_exit声明
#include <linux/init.h>      //__init  __exit 宏定义声明
#include <linux/device.h>    //class  devise声明
#include <linux/uaccess.h>   //copy_from_user 的头文件
#include <linux/types.h>     //设备号  dev_t 类型声明
#include <asm/io.h>          //ioremap iounmap的头文件

static struct class *pin4_class;  
static struct device *pin4_class_dev;

static dev_t devno;                     //设备号
static int major = 256;                 //主设备号
static int minor = 0;                   //次设备号
static char *module_name = "gpio4";     //模块名

//配置寄存器，第0组内有4号引脚，指针存放地址，volatile的作用是作为指令关键字，确保本条指令不会因编译器的优化而省略，且要求每次直接读值(数据读到内存就会缓存在里面，防止下次读取内存里的备份，时效性不强)
volatile unsigned int *GPFSEL0 = NULL;  //选择引脚功能
volatile unsigned int *GPSET0  = NULL;  //SET置1
volatile unsigned int *GPCLR0  = NULL;  //CLR清0

static int pin4_open(struct inode *inode, struct file *file)
{
    printk("pin4_open\n");              //内核的打印函数和printf类似

    //配置pin4引脚为输出引脚，将14-12位配置成001，为输出模式；001 = GPIO Pin 4 is an output
    *GPFSEL0 &= ~(0x6 << 12);           //按位操作，110（其余都是0）左移12位再取反，把13 、14置为0；此时12位与上1不改变本来的值，所以还要修改12位的值
    *GPFSEL0 |= (0x1 << 12);            //防止12位本来就是0，再把12置为1
    // 上述寄存器相当于 wiringPi 的 pinMode(4, OUTPUT); 设置 gpio4 为输出模式
    return 0;
}

static ssize_t pin4_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    int usercmd;

    printk("pin4_write\n");               //内核的打印函数和printf类似

    copy_from_user(&usercmd, buf, count); //将应用层用户输入的指令读入usercmd里面（获取上层write函数的值）
    if (usercmd == 1) {                   //根据值来操作IO口
        printk("set 1\n");
        *GPSET0 |= (0x01 << 4);           //电平拉高，手册写明，想拉高几号引脚就将那一位置1；相当于digitalWrite(4, HIGH);
    } else if (usercmd == 0) {
        printk("set 0\n");
        *GPCLR0 |= (0x01 << 4);           //电平拉低，手册写明，想拉低几号引脚就将那一位置1
    } else {
        printk("undo\n");
    }

    return 0;
}

static struct file_operations pin4_fops = {
    .owner = THIS_MODULE,
    .open  = pin4_open,
    .write = pin4_write,
};

int __init pin4_drv_init(void)
{
    printk("hello\n");

    int ret;
    devno = MKDEV(major, minor);                              //创建设备号
    ret   = register_chrdev(major, module_name, &pin4_fops);  //注册驱动  告诉内核，把这个驱动加入到内核驱动的链表中

    pin4_class = class_create(THIS_MODULE, "my_first_class"); //将设备节点与设备类 (class) 关联起来，以便系统可以根据设备类进行设备的管理和分类
    pin4_class_dev = device_create(pin4_class, NULL, devno, NULL, module_name);  //创建设备文件，类下面创建设备

    //物理地址转成虚拟地址，IO口寄存器映射成普通内存单元进行访问
    GPFSEL0 = (volatile unsigned int *)ioremap(0xFE200000, 4);//32位，4个字节大小的地址
	GPSET0  = (volatile unsigned int *)ioremap(0xFE20001C, 4);
    GPCLR0  = (volatile unsigned int *)ioremap(0xFE200028, 4);

    printk("insmod driver pin4 success\n");

    return 0;
}

void __exit pin4_drv_exit(void)
{
    printk("bye\n");

    iounmap(GPFSEL0);
    iounmap(GPSET0);
    iounmap(GPCLR0);                        //卸载驱动时释放地址映射

    device_destroy(pin4_class, devno);
    class_destroy(pin4_class);
    unregister_chrdev(major, module_name);  //卸载驱动
}

module_init(pin4_drv_init);  //入口; 内核加载该驱动的时候，这个宏会被调用
module_exit(pin4_drv_exit);  //出口

MODULE_LICENSE("GPL v2");
MODULE_VERSION("V0.2");
MODULE_AUTHOR("NUISTWF");