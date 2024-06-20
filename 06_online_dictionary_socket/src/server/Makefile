CC=gcc
CFLAGS+=-I ../include -lsqlite3

all:server

server:server.o
	$(CC) $^ -o $@ $(CFLAGS)

clean:
	rm -rf *.o server