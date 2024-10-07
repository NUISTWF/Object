# Makefile for compiling C++ files

# 目标文件
TARGET = example

# 源文件
SOURCES = example.cpp Xml.cpp

# 编译器参数
CXXFLAGS = -Wall

# 链接器参数
LDFLAGS = 

# 定义编译命令
CXX = g++

# 编译规则
$(TARGET) : $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# 清理规则
clean:
	rm -f $(TARGET)

.PHONY: clean

# 使命令在每次运行时都重新计算依赖关系，避免缓存问题
-include .depend
