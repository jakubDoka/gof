CC = gcc
CFLAGS = -Wall -g -std=c99

all: main

main: main.o map.o simulation.o
	$(CC) $(CFLAGS) main.o map.o simulation.o wincon/pdcurses.a -o main

main.o: main.c map.h simulation.h
	$(CC) $(CFLAGS) -c main.c -o main.o

map.o: map.c map.h
	$(CC) $(CFLAGS) -c map.c -o map.o

simulation.o: simulation.c simulation.h curses.h
	$(CC) $(CFLAGS) -c simulation.c -o simulation.o

clean:
	del main main.o map.o simulation.o main.exe
