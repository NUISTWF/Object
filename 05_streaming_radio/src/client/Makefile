
CC=gcc
CFLAGS+=-I ../include	#-I是指定头文件所在位置

all:client				#相当于make client

client:client.o			#gcc client.c -o client.o -I ../include
	$(CC) $^ -o $@ $(CFLAGS)

clean:
	rm -rf *.o client
