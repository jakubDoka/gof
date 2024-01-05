CC = gcc
CFLAGS = -Wall -g -std=c99
NCURSES_LIB = ncurses/lib/libncurses.a

all: client server test

test: test.o map.o simulation.o
	$(CC) $(CFLAGS) test.o map.o protocol.o -o test

server: server.o map.o protocol.o
	$(CC) $(CFLAGS) server.o map.o simulation.o protocol.o -o server

client: client.o map.o simulation.o
	$(CC) $(CFLAGS) client.o map.o simulation.o $(NCURSES_LIB) -o client

client.o: client.c map.h simulation.h
	$(CC) $(CFLAGS) -c client.c -o client.o

map.o: map.c map.h
	$(CC) $(CFLAGS) -c map.c -o map.o

simulation.o: simulation.c simulation.h
	$(CC) $(CFLAGS) -c simulation.c -o simulation.o

protocol.o: protocol.c protocol.h
	$(CC) $(CFLAGS) -c protocol.c -o protocol.o

clean:
	rm *.o client server test
