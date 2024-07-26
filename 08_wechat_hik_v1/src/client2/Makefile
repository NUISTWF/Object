CC=gcc
CFLAGS+=-I ../include -lpthread

all:client2

client2:client2.o
	$(CC) $^ -o $@ $(CFLAGS)

clean:
	rm -rf *.o client2