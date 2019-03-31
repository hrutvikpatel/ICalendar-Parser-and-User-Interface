CC=gcc
CFLAGS=-Wall -pedantic -std=c11 -g -fpic -Iinclude/
LDFLAGS = -L../ -Wl,-rpath=./

.PHONY: all simpleRun valgrind

all: list parser

simpleRun: test
	./bin/test

valgrind: test
	valgrind --leak-check=full --track-origins=yes --show-leak-kinds=all -v ./bin/test

list: liblist.o
	$(CC) $(CFLAGS) $(LDFLAGS) -shared bin/liblist.o -o ../liblist.so

liblist.o: include/LinkedListAPI.h
	$(CC) $(CFLAGS) $(LDFLAGS) -c src/LinkedListAPI.c -o bin/liblist.o

parser: libcal.o list
	$(CC) $(CFLAGS) $(LDFLAGS) -shared bin/libcal.o bin/JSON.o -o ../libcal.so -llist

libcal.o: include/CalendarParser.h include/LinkedListAPI.h
	$(CC) $(CFLAGS) $(LDFLAGS) -c src/JSON.c -o bin/JSON.o
	$(CC) $(CFLAGS) $(LDFLAGS) -c src/CalendarParser.c -o bin/libcal.o

test: test.o parser.o list.o json.o
	$(CC) $(CFLAGS) $(LDFLAGS) bin/parser.o bin/list.o bin/test.o bin/json.o -o bin/test
 
test.o:
	$(CC) $(CFLAGS) -c src/test.c -o bin/test.o

parser.o:
	$(CC) $(CFLAGS) -c src/CalendarParser.c -o bin/parser.o

list.o:
	$(CC) $(CFLAGS) -c src/LinkedListAPI.c -o bin/list.o

json.o:
	$(CC) $(CFLAGS) -c src/JSON.c -o bin/json.o

clean:
	rm -f bin/*.o ../*.so bin/test core
