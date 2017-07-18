# really crappy Makefile, which will do for now

PREFIX ?= /usr/local

all: strace-4.6/Makefile okapi snappy-1.1.1/Makefile leveldb-1.14.0/Makefile
	cd readelf-mini && make
	cd snappy-1.1.1 && make
	cd leveldb-1.14.0 && export CXXFLAGS=-I../snappy-1.1.1 && make
	cd strace-4.6 && make
	mv -f strace-4.6/strace ./ptu

install: all
	install ptu ptu-exec $(PREFIX)/bin

strace-4.6/Makefile:
	cd strace-4.6 && ./configure
	
snappy-1.1.1/Makefile:
	cd snappy-1.1.1 && ./configure --disable-shared --enable-static --with-pic

clean:
	cd readelf-mini && make clean
	cd snappy-1.1.1 && rm -f Makefile
	cd strace-4.6 && make clean
	rm -f ptu okapi

okapi: strace-4.6/okapi.c strace-4.6/okapi.h
	gcc -Wall -g -O2 -D_GNU_SOURCE -DOKAPI_STANDALONE strace-4.6/okapi.c -o okapi
