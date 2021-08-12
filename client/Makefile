# 定义了可执行文件变量
executable:= a.out
# 定义编译命令变量
cc=gcc
# 定义编译选项
CFLAGS=-Wall -g -c
# 定义了源文件列表变量
sources := $(wildcard *.c) $(wildcard src/*.c)
# 使用变量的引用替换，定义了object文件列表
OBJS:= $(sources:.c=.o) 
# 使用变量引用替换，定义依赖文件列表
#deps  := $(sources:.c=.d)
#需要调用的链接库
LIB+=-lpthread -lsqlite3
# 链接库路径
LIBPATH+=-L./ \
	-L/home/kevin/temp/libmy   
# 头文件路径
INC:=-I./inc/ \
	-I./

$(executable):$(OBJS)
	$(cc) $^ -o $@ $(LIBPATH) $(LIB)
%.o:%.c
	$(cc) $(CFLAGS) $^ $(INC) -o $@

.PHONY:clean
clean:
	rm *.o a.out -rf