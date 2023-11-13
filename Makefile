# gol - game of life
.POSIX:

include config.mk

SRC = main.c world.c
OBJ = $(SRC:.c=.o)

all: gof

gof.o: world.h

gof: $(OBJ)
	$(CC) -o $@ $(OBJ) $(STLDFLAGS)

.PHONY: all

