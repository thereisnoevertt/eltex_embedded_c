CC = gcc
CFLAGS = -Wall -pthread

TARGET = main

OBJS = client.o market.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

client.o: client.c market.h
	$(CC) $(CFLAGS) -c client.c

server.o: market.c market.h
	$(CC) $(CFLAGS) -c market.c

clean:
	rm -f *.o $(TARGET)