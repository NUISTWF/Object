# 编译器
CC = gcc

# 编译选项
CFLAGS = -Wall -pthread

# 目标可执行文件
TARGET = example

# 源文件
SRCS = example.c thpool.c

# 生成的对象文件
OBJS = $(SRCS:.c=.o)

# 默认目标
all: $(TARGET)

# 链接目标可执行文件
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# 编译源文件
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 清理生成的文件
clean:
	rm -f $(OBJS) $(TARGET)

# 伪目标
.PHONY: all clean
