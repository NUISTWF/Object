#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>

/* 假设有个全局的内核资源 */
static int g_data = 1;

/*文件打开函数*/
int hello_open(struct inode *inode, struct file *filp)
{    
    return 0; //这个是个空函数，默认都能成功打开
}

/*文件关闭函数*/
int hello_release(struct inode *inode, struct file *filp)
{
    return 0; //这个是个空函数，默认都能成功打开
}

/*读函数*/
static ssize_t hello_read(struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
  /* 将内核空间的数据copy到用户空间 */
    if (copy_to_user(buf, &g_data, size)) {
        return -EFAULT;
    }
	
    return size;
}

/*写函数*/
static ssize_t hello_write(struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{   
    /* 将用户空间数据copy到内核空间 */
    if (copy_from_user(&g_data, buf, size)) {
        return -EFAULT;
    }
 
    return size;
}

/*文件操作结构体*/
static const struct file_operations hello_fops =
{
	.owner = THIS_MODULE,
    .open = hello_open,   	//对应open 
    .read = hello_read,   	//对应read
    .write = hello_write, 	//对应write
    .release = hello_release, //对应close
};

struct cdev cdev; //静态定义字符设备cdev结构
dev_t devno;  //静态定义字符设备号

/*设备驱动模块加载函数*/
static int __init helloworld_init(void)
{   
	int ret = -1;
    cdev_init(&cdev, &hello_fops);/* 初始化cdev结构， 并将文件IO与字符设备的cdev建立链接 */
    ret = alloc_chrdev_region(&devno, 0, 1, "helloworld"); /* 动态申请设备号， */
	if (ret) {
		printk("alloc_chrdev_region error:%d\n", ret);
		cdev_del(&cdev);   /*注销设备*/
		return ret;
	}
    cdev_add(&cdev, devno, 1); /* 向linux系统添加一个字符设备 */
	
	printk ("hellorold init\n");
	
	return 0;
}

/*模块卸载函数*/
static void __exit helloworld_exit(void)
{
    cdev_del(&cdev);   /*注销设备*/
    unregister_chrdev_region(devno, 1); /*释放设备号*/
	
	printk ("helloworld exit\n");
}
module_init(helloworld_init); //内核模块加载函数
module_exit(helloworld_exit); //内核模块卸载函数
MODULE_LICENSE("GPL"); 
MODULE_AUTHOR ("zhangsan");
MODULE_DESCRIPTION ("this is a helloworld module");
