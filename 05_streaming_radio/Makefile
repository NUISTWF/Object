# 定义需要递归编译的子目录列表
SUBDIRS := src/client src/server

# 定义默认目标，递归调用所有子目录的 Makefile，并编译顶层文件
all:
	@for dir in $(SUBDIRS); do \
        $(MAKE) -C $$dir; \
    done

# 清理所有子目录的 Makefile 中生成的目标文件和可执行文件，以及顶层编译生成的文件
clean:
	@for dir in $(SUBDIRS); do \
        $(MAKE) -C $$dir clean; \
    done

# 如果需要其他操作，可以继续定义其他目标
# 例如执行测试或者安装等操作
test:
	@for dir in $(SUBDIRS); do \
        $(MAKE) -C $$dir test; \
    done
