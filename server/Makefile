# really crappy Makefile, which will do for now

PREFIX ?= /usr/local
CFLAGS=-Wall -g -O2
SRC=hashtable.c
HEADR=hashtable.h

all: server

server: server.c server.h $(SRC) $(HEADER)
	gcc $(CFLAGS) server.c $(SRC) -o server

client: client.c $(SRC) $(HEADER)
	gcc $(CFLAGS) client.c  $(SRC) -o client

clean:
	rm server client *.o