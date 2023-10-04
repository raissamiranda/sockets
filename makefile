CC = gcc
CFLAGS = -Wall -g -c -I .

all: server client

server: server.o common.o
	$(CC) -Wall -g server.o common.o -o server

server.o: server.c
	$(CC) server.c $(CFLAGS)

client: client.o common.o
	$(CC) -Wall -g client.o common.o -o client

client.o: client.c
	$(CC) client.c $(CFLAGS)

common.o: common.c
	$(CC) common.c $(CFLAGS)

clean:
	rm -f *.o server client