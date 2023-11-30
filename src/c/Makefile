##
# C Frontend for Hybrid Sorting Optimization
#

all: HSO-c

WARNFLAGS = -Wall -Wextra -Wno-sign-compare -Wno-missing-field-initializers

CFLAGS += $(WARNFLAGS) -O3
# CFLAGS += -DSORT_LARGE_STRUCTS

CXXFLAGS += $(WARNFLAGS) -O3 -fpic -shared

# CFLAGS += -g3 -fsanitize=address,undefined
# CXXFLAGS += -g3 -fsanitize=address,undefined

THIS_FILE := $(lastword $(MAKEFILE_LIST))

OBJS = benchmark.o platform.o sort.o msort_opt.o data.o sort_cxx.o

HSO-c: $(OBJS) main.o
	$(CC) $(CFLAGS) -o HSO-c main.o $(OBJS) -lz -lstdc++

alphadev: $(OBJS) main.o asm_sort.c asm_sort.h
	$(CC) $(CFLAGS) -Wno-language-extension-token asm_sort.c -c
	$(CC) $(CFLAGS) -o HSO-c main.o $(OBJS) asm_sort.o -lz

main.o: main.c platform.h
	$(CC) $(CFLAGS) main.c -c

platform.o: platform.c platform.h
	$(CC) $(CFLAGS) platform.c -c

benchmark.o: benchmark.c benchmark.h
	$(CC) $(CFLAGS) benchmark.c -c

sort.o: sort.c sort.h
	$(CC) $(CFLAGS) sort.c -c

data.o: data.c data.h
	$(CC) $(CFLAGS) data.c -c

msort_opt.o: msort_opt.c sort.h
	$(CC) $(CFLAGS) msort_opt.c -c

sort_cxx.o: sort_cxx.cpp sort_cxx.h
	$(CXX) $(CXXFLAGS) sort_cxx.cpp -c

debug: CFLAGS = -Og -g3 --coverage $(WARNFLAGS)
debug: $(OBJS) HSO-c

tests: CFLAGS = -Og -g3 --coverage $(WARNFLAGS)
tests: CXXFLAGS += --coverage
tests: tests.cpp tests.hpp
	$(MAKE) -f $(THIS_FILE) clean
	$(MAKE) -f $(THIS_FILE) debug
	$(CXX) $(CFLAGS) $(DEBUGFLAGS) $(TESTFLAGS) -o tests tests.cpp $(OBJS) \
        -Wl,-Bstatic -lboost_unit_test_framework \
        -Wl,-Bdynamic -lz

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
	rm -f HSO-c tests $(OBJS) main.o *.gcda *.gcno test-coverage.info asm_sort.o
	rm -rf ./coverage

# Frogbert has blessed this place.
# end
