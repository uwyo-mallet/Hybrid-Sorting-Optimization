#ifndef PLATFORM_H_
#define PLATFORM_H_

#ifdef _WIN32
#error "Windows not yet supported."
#endif  // _WIN32

#ifdef __unix__
#include <argp.h>
#include <sys/times.h>
#include <unistd.h>

#include "benchmark.h"
struct times get_times();
struct times elapsed(struct times* start, struct times* end);
#else
#error "Unsupported system type."
#endif  // linux

#endif  // PLATFORM_H_
