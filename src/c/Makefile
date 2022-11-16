##
# C Frontend for QST
#
# @file
# @version 0.1

all: QST-c

CC = gcc
CFLAGS = -Wall -Wextra -Wno-missing-field-initializers -pedantic -O2
DEBUGFLAGS = -fsanitize=address,undefined,leak -ggdb

OBJS = main.o benchmark.o platform.o sort.o

QST-c: $(OBJS)
	$(CC) $(CFLAGS) $(DEBUGFLAGS) -o QST-c -lz $(OBJS)

main.o: main.c platform.h
	$(CC) $(CFLAGS) $(DEBUGFLAGS) main.c -c

platform.o: platform.c platform.h
	$(CC) $(CFLAGS) $(DEBUGFLAGS) platform.c -c

benchmark.o: benchmark.c benchmark.h
	$(CC) $(CFLAGS) $(DEBUGFLAGS) benchmark.c -c

sort.o: sort.c sort.h
	$(CC) $(CFLAGS) $(DEBUGFLAGS) sort.c -c

clean:
	rm -f QST-c $(OBJS)

# end
