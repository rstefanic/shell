CC = gcc
CFLAGS = -Wall -g

TARGET = shell

OBJS = main.o memory.o

run: $(TARGET)
	@./$(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

main.o: main.c
	$(CC) $(CFLAGS) -c main.c -o main.o

memory.o: memory.c memory.h
	$(CC) $(CFLAGS) -c memory.c -o memory.o

clean:
	rm -f $(TARGET) $(OBJS)
