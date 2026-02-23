CC = gcc
CFLAGS = -Wall -Wextra -Wconversion -pedantic
DEBUG_CFLAGS = -g -O0 -fsanitize=address -DDEBUG

config ?= debug

ifeq ($(config), debug)
	CFLAGS += $(DEBUG_CFLAGS)
	DEBUG_OBJS = debug.o
endif

TARGET = shell

OBJS = main.o memory.o lexer.o string.o parser.o hashtable.o log.o evaluator.o $(DEBUG_OBJS)

run: $(TARGET)
	@./$(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

main.o: main.c
	$(CC) $(CFLAGS) -c main.c -o main.o

memory.o: memory.c memory.h
	$(CC) $(CFLAGS) -c memory.c -o memory.o

string.o: string.c string.h
	$(CC) $(CFLAGS) -c string.c -o string.o

lexer.o: lexer.c
	$(CC) $(CFLAGS) -c lexer.c -o lexer.o

parser.o: parser.c
	$(CC) $(CFLAGS) -c parser.c -o parser.o

hashtable.o: hashtable.c
	$(CC) $(CFLAGS) -c hashtable.c -o hashtable.o

log.o: log.c
	$(CC) $(CFLAGS) -c log.c -o log.o

evaluator.o: evaluator.c
	$(CC) $(CFLAGS) -c evaluator.c -o evaluator.o

debug.o: debug.c
	$(CC) $(CFLAGS) -c debug.c -o debug.o

clean:
	rm -f $(TARGET) $(OBJS)
