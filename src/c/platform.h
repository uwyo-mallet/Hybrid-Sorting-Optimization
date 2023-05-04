#ifndef PLATFORM_H_
#define PLATFORM_H_

#ifdef _WIN32
#error "Windows not yet supported."
#endif  // _WIN32

#ifdef __unix__
#include <argp.h>
#include <inttypes.h>
#include <sys/times.h>
#include <time.h>
#include <unistd.h>

struct times
{
  clock_t user;
  clock_t system;
  time_t wall_secs;
  intmax_t wall_nsecs;
};

struct times get_times();
struct times elapsed(struct times* start, struct times* end);
#else
#error "Unsupported system type."
#endif  // linux

#endif  // PLATFORM_H_
