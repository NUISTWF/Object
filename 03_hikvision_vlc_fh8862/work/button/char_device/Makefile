#对应内核源码路径(若有差异，需要学生修改为自己的内核源码路径)
KDIR = /home/fh8862/resource/FH8862_IPC_V1.0.0_20221111/board_support/kernel/linux-4.9

#处理器
ARCH = arm 
#交叉编译工具链前缀
CROSS_COMPILE = arm-fullhanv3-linux-uclibcgnueabi- 
.PHONY:Kbuild
Kbuild:
	make -C $(KDIR) M=`pwd` ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE)
clean:
	make -C $(KDIR) M=`pwd` clean
	
