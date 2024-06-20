#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	int ret = 0, val = 0;
	
	int fd = open("/dev/helloworld", O_RDWR); //打开设备驱动节点
	if (fd < 0) {
		perror("open");
		return -1;
	}
	
	if(read(fd, &val, 4) < 0) {//从设备驱动读取4个字节数据到&val地址
		perror("read");
		ret = -1;
		goto end;
	}
	printf("read val :%d\n", val);
	val = 100;
	printf("will write:%d\n", val);
	if(write(fd, &val, 4) < 0) {//将4个字节的val数据写入设备驱动中
		perror("write");
		ret = -1;
		goto end;
	}
	if(read(fd, &val, 4) < 0) {//从设备驱动读取4个字节数据到&val地址
		perror("read");
		ret = -1;
		goto end;
	}
	printf("read val :%d\n", val);
	

end:
	close (fd); //关闭打开的设备驱动节点
	return ret;
}