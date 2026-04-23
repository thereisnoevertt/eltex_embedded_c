CC = gcc
CFLAGS = -Wall -Wextra -O2 -pthread
LDFLAGS = -pthread -lrt

# Для клиента добавляем ncurses
CLIENT_LDFLAGS = -pthread -lrt -lncurses

TARGETS = server client

all: $(TARGETS)

server: server.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

client: client.c
	$(CC) $(CFLAGS) -o $@ $< $(CLIENT_LDFLAGS)

clean:
	rm -f $(TARGETS)

.PHONY: all clean