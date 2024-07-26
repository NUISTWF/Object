CC=gcc
CFLAGS+=-I ../include

all:client1

client1:client1.o
	$(CC) $^ -o $@ $(CFLAGS) -lpthread

clean:
	rm -rf *.o client1