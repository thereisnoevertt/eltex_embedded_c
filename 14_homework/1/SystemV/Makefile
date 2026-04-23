CC = gcc
CFLAGS = -Wall -lrt

all: client server

client: client.c
	$(CC) $(CFLAGS) -o client client.c -lpthread

server: server.c
	$(CC) $(CFLAGS) -o server server.c -lpthread

clean:
	rm -f client server