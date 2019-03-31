CC=gcc
CFLAGS=-Wall -pedantic -std=c11 -g -fpic -Iinclude/
LDFLAGS = -L../

.PHONY: all simpleRun valgrind

all: list parser

list: liblist.o
	$(CC) $(CFLAGS) $(LDFLAGS) -shared bin/liblist.o -o ../liblist.so

liblist.o: include/LinkedListAPI.h
	$(CC) $(CFLAGS) $(LDFLAGS) -c src/LinkedListAPI.c -o bin/liblist.o

parser: libcal.o
	$(CC) $(CFLAGS) $(LDFLAGS) -shared bin/libcal.o bin/liblist.o -o ../libcal.so

libcal.o: include/CalendarParser.h include/LinkedListAPI.h
	$(CC) $(CFLAGS) $(LDFLAGS) -c src/CalendarParser.c -o bin/libcal.o

clean:
	rm -f bin/*.o ../*.o bin/*.so bin/test core
