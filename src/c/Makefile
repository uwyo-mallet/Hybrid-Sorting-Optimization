##
# C Frontend for HSO
#
# @file
# @version 0.1

all: HSO-c

CC = gcc
CFLAGS = -Wall -Wextra -Wno-missing-field-initializers -pedantic -O3 -DNDEBUG
# CFLAGS += -DSORT_LARGE_STRUCTS
# DEBUGFLAGS = -ggdb -fsanitize=address,undefined,leak -Og
# DEBUGFLAGS = -ggdb -O3
TESTFLAGS = -Wno-unused-function -Wno-unused-const-variable

OBJS = benchmark.o platform.o sort.o msort_opt.o

HSO-c: $(OBJS) main.o
	$(CC) $(CFLAGS) $(DEBUGFLAGS) -o HSO-c main.o $(OBJS) -lz

main.o: main.c platform.h
	$(CC) $(CFLAGS) $(DEBUGFLAGS) main.c -c

platform.o: platform.c platform.h
	$(CC) $(CFLAGS) $(DEBUGFLAGS) platform.c -c

benchmark.o: benchmark.c benchmark.h
	$(CC) $(CFLAGS) $(DEBUGFLAGS) benchmark.c -c

sort.o: sort.c sort.h
	$(CC) $(CFLAGS) $(DEBUGFLAGS) sort.c -c

msort_opt.o: msort_opt.c sort.h
	$(CC) $(CFLAGS) $(DEBUGFLAGS) msort_opt.c -c

check: HSO-c tests.c
	$(CC) $(CFLAGS) $(DEBUGFLAGS) $(TESTFLAGS) -o tests tests.c $(OBJS) -lcmocka

check-glibc-var:
ifndef GLIBC
		$(error GLIBC (glibc build path) is unset. Try again with 'make GLIBC=/foo/bar glibc')
endif

glibc: check-glibc-var
	$(CC) -Wl,-rpath="$(GLIBC):$(GLIBC)/math:\
                    $(GLIBC)/elf:\
                    $(GLIBC)/dlfcn:\
                    $(GLIBC)/nss:\
                    $(GLIBC)/nis:\
                    $(GLIBC)/rt:\
                    $(GLIBC)/resolv:\
                    $(GLIBC)/crypt:\
                    $(GLIBC)/nptl:\
                    $(GLIBC)/dfp" \
        -Wl,--dynamic-linker=$(GLIBC)/elf/ld.so \
        $(CFLAGS) $(DEBUGFLAGS) -o HSO-c-glibc \
        main.c \
        platform.c \
        benchmark.c \
        sort.c \
        msort_opt.c -lz

clean:
	rm -f HSO-c $(OBJS)

# end
