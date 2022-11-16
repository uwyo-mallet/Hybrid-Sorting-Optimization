##
# C Frontend for QST
#
# @file
# @version 0.1

all: QST-c

CC = gcc
CFLAGS = -Wall -Wextra -pedantic -ggdb

QST-c: main.c
	$(CC) $(CFLAGS) -o QST-c -lz -fsanitize=address,undefined,leak main.c

clean:
	rm -f QST-c

# end
