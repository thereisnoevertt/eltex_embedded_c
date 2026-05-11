CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic
LDFLAGS = -lpthread
 
TARGETS = server client
 
.PHONY: all clean run-server run-client
 
all: $(TARGETS)
 
server: server.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)
 
client: client.c
	$(CC) $(CFLAGS) -o $@ $<
 
run-server: server
	./server
 
run-client: client
	./client
 
clean:
	rm -f $(TARGETS)