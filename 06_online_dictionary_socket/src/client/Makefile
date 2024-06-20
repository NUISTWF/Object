CC=gcc
CFLAGS+=-I ../include	# -I specifies the location of the header file

all:client				# Equivalent to make client

client:client.o			# gcc client.c -o client.o -I ../include
	$(CC) $^ -o $@ $(CFLAGS)

clean:
	rm -rf *.o client