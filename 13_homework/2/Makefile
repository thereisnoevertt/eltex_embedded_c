CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c11 -pthread
LDFLAGS = -lncurses -lrt -lpthread

.PHONY: all clean kill-queues

all: server client

server: server.c message.h
	$(CC) $(CFLAGS) -o server server.c $(LDFLAGS)

client: client.c message.h
	$(CC) $(CFLAGS) -o client client.c $(LDFLAGS)

clean:
	rm -f server client
	rm -f /dev/mqueue/main_chat /dev/mqueue/client_*

kill-queues:
	rm -f /dev/mqueue/main_chat /dev/mqueue/client_*