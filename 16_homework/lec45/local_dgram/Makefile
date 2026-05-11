CC = gcc
CFLAGS = -Wall -Wextra -g

target: server client

server: server.c
	$(CC) $(CFLAGS) -o server server.c

sender: client.c
	$(CC) $(CFLAGS) -o client client.c

clean:
	rm -f server client