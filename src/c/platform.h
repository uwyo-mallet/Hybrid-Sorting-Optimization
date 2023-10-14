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

struct perf_data
{
  /* HW Counters */
  uint64_t count_hw_cpu_cycles;
  uint64_t count_hw_instructions;
  uint64_t count_hw_cache_references;
  uint64_t count_hw_cache_misses;
  uint64_t count_hw_branch_instructions;
  uint64_t count_hw_branch_misses;
  uint64_t count_hw_bus_cycles;

  /* SW Counters */
  uint64_t count_sw_cpu_clock;
  uint64_t count_sw_task_clock;
  uint64_t count_sw_page_faults;
  uint64_t count_sw_context_switches;
  uint64_t count_sw_cpu_migrations;
};

struct perf_fds
{
  /* HW Counters */
  int count_hw_cpu_cycles;
  int count_hw_instructions;
  int count_hw_cache_references;
  int count_hw_cache_misses;
  int count_hw_branch_instructions;
  int count_hw_branch_misses;
  int count_hw_bus_cycles;

  /* SW Counters */
  int count_sw_cpu_clock;
  int count_sw_task_clock;
  int count_sw_page_faults;
  int count_sw_context_switches;
  int count_sw_cpu_migrations;
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
