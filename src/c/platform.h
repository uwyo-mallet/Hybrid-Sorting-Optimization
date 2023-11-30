#ifndef PLATFORM_H_
#define PLATFORM_H_

#ifdef _WIN32
#error "Windows not yet supported, and never will be."
#endif  // _WIN32

#ifdef __unix__

#ifdef __clang__
#define ALPHADEV
#include "asm_sort.h"
#endif  // __clang__

#include <argp.h>
#include <inttypes.h>
#include <sys/times.h>
#include <time.h>
#include <unistd.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif  // _GNU_SOURCE

#define ARRAY_SIZE(x) ((sizeof x) / (sizeof *x))
#define MIN(a, b)           \
  ({                        \
    __typeof__(a) _a = (a); \
    __typeof__(b) _b = (b); \
    _a < _b ? _a : _b;      \
  })

struct perf_data
{
  /*
   * Fields:
   *  count_hw_cpu_cycles
   *  count_hw_instructions
   *  count_hw_cache_references
   *  count_hw_cache_misses
   *  count_hw_branch_instructions
   *  count_hw_branch_misses
   *  count_hw_bus_cycles
   *  count_sw_cpu_clock
   *  count_sw_task_clock
   *  count_sw_page_faults
   *  count_sw_context_switches
   *  count_sw_cpu_migrations
   */
  uint64_t counters[12];
};

struct perf_fds
{
  int fds[12];
};

struct times
{
  clock_t user;
  clock_t system;
  time_t wall_secs;
  intmax_t wall_nsecs;

  struct perf_data perf;
};

void perf_event_open(struct perf_fds* perf);
void perf_start(struct perf_fds* perf);
void perf_stop(struct perf_fds* perf);
void perf_reset(struct perf_fds* perf);
void perf_event_close(struct perf_fds* perf);
void perf_dump(struct perf_data* data, struct perf_fds* fds);

struct times get_times(int start, struct perf_fds* fds);
struct times elapsed(struct times* start, struct times* end, struct perf_fds*);

#else
#error "Unsupported system type."
#endif  // linux

#endif  // PLATFORM_H_
