CC = gcc
CFLAGS = -Wall -g -std=c99
NCURSES_LIB = ncurses/lib/libncurses.a

all: client

client: client.o map.o simulation.o allocs.o
	$(CC) $(CFLAGS) client.o map.o simulation.o allocs.o $(NCURSES_LIB) -o client

client.o: client.c map.h simulation.h
	$(CC) $(CFLAGS) -c client.c -o client.o

map.o: map.c map.h
	$(CC) $(CFLAGS) -c map.c -o map.o

simulation.o: simulation.c simulation.h
	$(CC) $(CFLAGS) -c simulation.c -o simulation.o

allocs.o: allocs.c allocs.h
	$(CC) $(CFLAGS) -c allocs.c -o allocs.o

clean:
	rm *.o a.out
