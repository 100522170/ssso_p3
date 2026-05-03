CC=gcc
CFLAGS = -g -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -O2
OBJ= queue.o parking_manager.o
LIBS= -pthread

all: parking

parking: $(OBJ)
	$(CC) $(CFLAGS) -o parking $(OBJ) $(LIBS)

queue: queue.c 
	$(CC) $(CFLAGS) -c queue.c -o queue.o

parking_manager: parking_manager.c queue.o
	$(CC) $(CFLAGS) -c parking_manager.c -o parking_manager.o

clean:
	rm -f parking *.o

.PHONY: all clean
