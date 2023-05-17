CC = gcc
CFLAGS = -O3 -Wall `pkg-config --cflags gtk+-3.0`
LIBS = -pthread `pkg-config --libs gtk+-3.0`
SRC = src

all: server.out client.out

server.out: $(SRC)/server.c
	$(CC) $(CFLAGS) -o server.out $(SRC)/server.c $(LIBS)

client.out: $(SRC)/client.c
	$(CC) $(CFLAGS) -o client.out $(SRC)/client.c $(LIBS)

.PHONY: clean
clean:
	rm -f *.out

