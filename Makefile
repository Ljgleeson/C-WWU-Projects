
CC=gcc
CFLAGS= -g -Wall

ush: ush.o expand.o builtin.o strmode.o
	$(CC) $(CFLAGS) -o ush ush.o expand.o builtin.o strmode.o

clean:
	rm -r ush ush.o expand.o builtin.o strmode.o

# dependency list
# "both ush.o and expand.o depend on defn.h"
ush.o expand.o builtin.o strmode.o: defn.h
