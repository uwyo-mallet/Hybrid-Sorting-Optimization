##
# C Frontend for Hybrid Sorting Optimization
#

all: HSO-c

WARNFLAGS = -Wall -Wextra -pedantic # -fsanitize=address

# CC = gcc
CC = clang
CFLAGS += -O3
CFLAGS += $(WARNFLAGS)
CFLAGS += -DSORT_INTS
# CFLAGS += -DSORT_LARGE_STRUCTS

CXX = clang++
CXXFLAGS += $(CFLAGS)
THIS_FILE := $(lastword $(MAKEFILE_LIST))

OBJS = benchmark.o platform.o sort.o msort_opt.o

HSO-c: $(OBJS) main.o
	$(CC) $(CFLAGS) -o HSO-c main.o $(OBJS) -lz

main.o: main.c platform.h
	$(CC) $(CFLAGS) main.c -c

platform.o: platform.c platform.h
	$(CC) $(CFLAGS) platform.c -c

benchmark.o: benchmark.c benchmark.h
	$(CC) $(CFLAGS) benchmark.c -c

sort.o: sort.c sort.h
	$(CC) $(CFLAGS) sort.c -c

msort_opt.o: msort_opt.c sort.h
	$(CC) $(CFLAGS) msort_opt.c -c

debug: CFLAGS = -Og -g --coverage $(WARNFLAGS)
debug: $(OBJS)

tests: CFLAGS = -Og -g --coverage $(WARNFLAGS)
tests: CXXFLAGS += --coverage
tests: tests.cpp tests.hpp
	$(MAKE) -f $(THIS_FILE) clean
	$(MAKE) -f $(THIS_FILE) debug
	$(CXX) $(CXXFLAGS) $(DEBUGFLAGS) $(TESTFLAGS) -o tests tests.cpp $(OBJS) \
        -Wl,-Bstatic -lboost_unit_test_framework \
        -Wl,-Bdynamic

check: tests
	./tests || true
	lcov --no-external --capture --directory . \
        --output-file test-coverage.info  \
        --exclude */benchmark.c --exclude */platform.c
	genhtml test-coverage.info -o coverage/

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
	rm -f HSO-c tests $(OBJS) main.o *.gcda *.gcno test-coverage.info
	rm -rf ./coverage

# Frogbert has blessed this place.
# end
