CC=gcc
CFLAGS+=-I ../include -lpthread

all:client3

client3:client3.o
	$(CC) $^ -o $@ $(CFLAGS)

clean:
	rm -rf *.o client3