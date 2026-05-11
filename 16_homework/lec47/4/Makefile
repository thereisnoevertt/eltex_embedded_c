CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic
LDFLAGS = -lpthread
 
TARGETS = server tcp_client udp_client
 
.PHONY: all clean run-server run-tcp-client run-udp-client
 
all: $(TARGETS)
 
server: server.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)
 
tcp_client: tcp_client.c
	$(CC) $(CFLAGS) -o $@ $<

udp_client: udp_client.c
	$(CC) $(CFLAGS) -o $@ $<
 
run-server: server
	./server
 
run-tcp-client: tcp-client
	./tcp-client

run-udp-client: udp-client
	./udp-client
 
clean:
	rm -f $(TARGETS)