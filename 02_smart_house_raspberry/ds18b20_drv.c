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

struct gpio_desc {
	int gpio;
    char *name;
	char *class;
} ;

static struct gpio_desc gpios[1] = {
    {25, "ds18b20", "my_ds18b20_class"},
};

static int major = 0;
static struct class *gpio_class;

static spinlock_t ds18b20_spinlock;		// 自旋锁


/* DS18B20的CRC8校验程序参考: https://www.cnblogs.com/yuanguanghui/p/12737740.html */   
static unsigned char calcrc_1byte(unsigned char abyte)   
{   
	unsigned char i,crc_1byte;     
	crc_1byte=0;                //设定crc_1byte初值为0  
	for (i = 0; i < 8; i++) {   
		if(((crc_1byte^abyte)&0x01)) {   
			crc_1byte^=0x18;     
			crc_1byte>>=1;   
			crc_1byte|=0x80;   
		} else {
			crc_1byte>>=1;   
		}
		abyte>>=1;         
	}   
	return crc_1byte;   
}

static unsigned char calcrc_bytes(unsigned char *p,unsigned char len)  
{  
	unsigned char crc = 0;  
	while (len--) { 			// len为总共要校验的字节数  
		crc = calcrc_1byte(crc^*p++);  
	}  

	return crc;  				// 若最终返回的crc为0，则数据传输正确  
}

static int ds18b20_verify_crc(unsigned char *buf)
{
    unsigned char crc;

	crc = calcrc_bytes(buf, 8);

    if (crc == buf[8])
		return 0;
	else
		return -1;
}


static void ds18b20_udelay(int us)
{
	u64 time = ktime_get_ns();						// 测试内核当前精确的时间是ns，所以下面需要乘1000转换为纳秒
	while ((ktime_get_ns() - time) < us*1000);		// 480us*1000 = 480000ns, 当内核过了480us后就退出循环
}


static int ds18b20_reset_and_wait_ack(void)	// 初始化 
{
	int timeout = 100;

	gpio_set_value(gpios[0].gpio, 0);		// 设置为0（一开始申请该gpio口的时候就是给的高电平）
	ds18b20_udelay(480);					// 延时480us
	gpio_direction_input(gpios[0].gpio);	// 转为输入引脚，由ds18b20发信号（变成input的同时也自动拉高了电平）

	/* 等待ACK */
	while (gpio_get_value(gpios[0].gpio) && (timeout--)) {	// 当该引脚为低电平的时候说明模块响应直接退出死循环，或者时间100次减没了也退出循环（防止死在这里）
		ds18b20_udelay(1);					// 等待1us，然后去判断一下while的条件，说白了就执行了100次延时1us，也就是延时100us，如果等了100us退出该循环的话就意味着响应电平不是低电平，则出错（在while里面做了一个延时）
	}
	if (timeout == 0)						// 如果是100us后就执行这里，超时退出
		return -EIO;

	/* 等待ACK结束，会自动释放总线，拉高电平*/
	timeout = 300;							// 等待300us，会自动拉高电平，循环判断是否为高电平，是的话即向下执行，不是即卡死在这，除非超时退出
	while (!gpio_get_value(gpios[0].gpio) && (timeout--)) {
		ds18b20_udelay(1);
	}
	if (!timeout)							// 时间耗尽就出错
		return -EIO;
	
	return 0;
}


static void ds18b20_write_cmd(unsigned char cmd)		// 完成0xcc这种类型的命令到发实际0或1数据的指令转换
{
	int i;
	
	gpio_direction_output(gpios[0].gpio, 1);			// 此时主机需要发信号，将引脚变为输出模式，初始置为1

	for (i = 0; i < 8; i++) {							// 发送8位数据 (0xcc, 0x44)，先发的是最低位
		if (cmd & (1<<i)) {								// 如果最低位是1的话即发1（发送1）
			gpio_direction_output(gpios[0].gpio, 0);	// 拉低电平；准备发送数据前，先拉低最少1us
			ds18b20_udelay(2);							// 拉低2us
			gpio_set_value(gpios[0].gpio, 1);			// 紧接着就拉高电平表示输出1
			ds18b20_udelay(60);							// 延时60us等待DS18B20采样完毕
		} else {										// 如果最低位是0的话即发0（发送0）
			gpio_direction_output(gpios[0].gpio, 0);	// 拉低电平
			ds18b20_udelay(60);							// 写0的话就一直维持低电平即可，同样的延时60us等待DS18B20在这60us内采样完毕
			gpio_direction_output(gpios[0].gpio, 1);	// 写完0，DS18B20采样完毕拉高电平
		}
	}
}


static void ds18b20_read_data(unsigned char *buf)		// 读取到一个8bit的完整的1个字节的数据借指针传回buf数组中
{
	int i;
	unsigned char data = 0;

	gpio_direction_output(gpios[0].gpio, 1);			// 此时主机发信号，引脚输出模式，初始置为1

	for (i = 0; i < 8; i++) {							// 8次循环，将 DS18B20 发出来的每一位1或0都存储到data中
		gpio_set_value(gpios[0].gpio, 0);				// 一开始就拉低电平持续最少1us
		ds18b20_udelay(2);								// 拉低至少1us，这里拉低2us
		gpio_direction_input(gpios[0].gpio);			// 此时转为 DS18B20 发数据给主机，gpio引脚转为输入模式，此时默认高电平
		ds18b20_udelay(15);								// 延时15us，15us之后 DS18B20 就已经发出数据了，接着下面可以检测引脚的值
		if (gpio_get_value(gpios[0].gpio)) {			// 检测引脚的值，读到1即高电平就给data置位
			data |= (1<<i);								// 若为1则将用data来存放最低位，此时data的8个bit的最低位就是1，否则默认就是0，因为data初始化就为0
		}
		ds18b20_udelay(50);								// 延时50us，表示对方一次数据发送完成，主机一次数据接受结束
		gpio_direction_output(gpios[0].gpio, 1);		// 一次数据传输结束转为输出模式且置1，进入下一轮循环再拉低表示告诉DS18B20我要开始读数据
	}

	*buf = data;										// data数据放回buf数组中
	// buf[0] = data;
}


static void ds18b20_calc_val(unsigned char *ds18b20_buf, int *result)		// 转化数据
{
	unsigned char tempL = 0, tempH = 0;
	int Temperature;
	int integer;

	tempL = ds18b20_buf[0]; 							// 读温度低8位（第一个字节的数据是低8位）
	tempH = ds18b20_buf[1]; 							// 读温度高8位（第二个字节的数据是高8位）
/*	
	if (tempH > 0x7f) {     							// 最高位为1时温度是负（高五位全1代表负，全0代表正）；因为0x7f在加1就变成0x80，只要前五位为1就是大于0x7f

	} else {											// 正数温度
		Temperature = (tempH<<8)|tempL; 				// 高八位与低八位进行整合
		integer = Temperature/16;						// 除16相当于右移4位，只需要温度的整数
	}

	result[0] = integer;
*/

	unsigned char decimal1, decimal2, decimal;
	if (tempH > 0x7f) {     							// 最高位为1时温度是负（高五位全1代表负，全0代表正）；因为0x7f在加1就变成0x80，只要前五位为1就是大于0x7f
		tempL    = ~tempL;         				    	// 补码转换，取反加一
		tempH    = (~tempH)+1;							// 高8位取反加1
		integer  = tempL/16+tempH*16;      				// 整数部分
		decimal1 = (tempL&0x0f)*10/16; 					// 小数第一位
		decimal2 = (tempL&0x0f)*100/16%10;				// 小数第二位
		decimal  = decimal1*10+decimal2; 				// 小数两位
	} else {											// 正数温度							
		integer  = tempL/16+tempH*16;      				// 整数部分
		decimal1 = (tempL&0x0f)*10/16; 					// 小数第一位
		decimal2 = (tempL&0x0f)*100/16%10;				// 小数第二位
		decimal  = decimal1*10+decimal2; 				// 小数两位
	}

	result[0] = integer;
	result[1] = decimal;

}


static ssize_t ds18b20_read(struct file *file, char __user *buf, size_t size, loff_t *offset)
{
	int err, i;
	unsigned char kern_buf[9];
	int result_buf[2];

	if (size != 8)
		return -EINVAL;

	/* 1. 启动温度转换 */
	// 1.1 关中断 获取自旋锁并禁用本地中断
	unsigned long flags;
	spin_lock_irqsave(&ds18b20_spinlock, flags);	// 可以确保在临界区内执行时不会被中断打断，从而保证了临界区的原子性

	/* 1.2 发出reset信号并等待回应（对应图中的初始化ds18b20模块并且等待它的回应） */
	err = ds18b20_reset_and_wait_ack();				// 简单来说就是初始化 Init_DS18B20
	if (err) {
		spin_unlock_irqrestore(&ds18b20_spinlock, flags);		// 用于释放自旋锁并恢复中断状态
		printk("ds18b20_reset_and_wait_ack err\n");
		return err;
	}

	/* 1.3 一切正常就发出命令: skip ROM, 0xcc（跳过读序号列号的操作） */
	ds18b20_write_cmd(0xcc);

	/* 1.4 发出温度转换命令: 启动温度转换, 0x44 （启动温度转换）*/
	ds18b20_write_cmd(0x44);

	/* 1.5 恢复中断 */
	spin_unlock_irqrestore(&ds18b20_spinlock, flags);

	/* 2. 等待温度转换成功 : 可能长达1s */
	schedule_timeout(msecs_to_jiffies(1000));	// 用于在内核中让当前进程休眠一段时间，并允许其他进程运行（延迟1000毫秒即1秒）

	/* 3. 读取温度 */
	/* 3.1 关中断 */
	spin_lock_irqsave(&ds18b20_spinlock, flags);

	/* 3.2 再次发出reset信号并等待回应（这一次开始需要读取或者写入数据） */
	err = ds18b20_reset_and_wait_ack();
	if (err) {
		spin_unlock_irqrestore(&ds18b20_spinlock, flags);
		printk("ds18b20_reset_and_wait_ack second err\n");
		return err;
	}
	/* 3.3 发出命令: skip rom, 0xcc */
	ds18b20_write_cmd(0xcc);					// 跳过读序号列号的操作

	/* 3.4 发出读数据命令: read scratchpad, 0xbe */
	ds18b20_write_cmd(0xbe);					// 读取温度寄存器

	/* 3.5 读9字节数据 */
	for (i = 0; i < 9; i++) {
		ds18b20_read_data(&kern_buf[i]);		// 该函数是每次读1个字节的数据，拿到所有的9个字节的数据存到kern_buf中（根据手册图可以看到一共获取9个字节的数据）
	}

	/* 3.6 恢复中断 */
	spin_unlock_irqrestore(&ds18b20_spinlock, flags);

	/* 3.7 计算CRC验证数据 */
	err = ds18b20_verify_crc(kern_buf);
	if (err) {
		printk("ds18b20 verify crc err\n");
		return err;
	}

	/* 4. copy_to_user */
	ds18b20_calc_val(kern_buf, result_buf);		// 将每一位去计算转为能看懂的温度
	
	copy_to_user(buf, result_buf, 8);

	return 8;
}


static struct file_operations gpio_ds18b20_drv = {
	.owner	 = THIS_MODULE,
	.read    = ds18b20_read,
};


static int __init ds18b20_init(void)
{
	spin_lock_init(&ds18b20_spinlock);
		
	gpio_request(gpios[0].gpio, gpios[0].name);
	gpio_direction_output(gpios[0].gpio, 1);		// 初始输出模式，输出高电平

	major = register_chrdev(0, gpios[0].name, &gpio_ds18b20_drv);
	gpio_class = class_create(THIS_MODULE, gpios[0].class);
	device_create(gpio_class, NULL, MKDEV(major, 0), NULL, gpios[0].name);
	
	return 1;
}


static void __exit ds18b20_exit(void)
{
	device_destroy(gpio_class, MKDEV(major, 0));
	class_destroy(gpio_class);
	unregister_chrdev(major, gpios[0].name);
	gpio_free(gpios[0].gpio);
}


module_init(ds18b20_init);
module_exit(ds18b20_exit);

MODULE_LICENSE("GPL");