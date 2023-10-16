#include "platform.h"

#ifdef linux

#include <inttypes.h>
#include <linux/perf_event.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>

#define ARRAY_SIZE(x) ((sizeof x) / (sizeof *x))

static long raw_perf_event_open(struct perf_event_attr* hw_event, pid_t pid,
                                int cpu, int group_fd, unsigned long flags)
{
  int ret;
  ret = syscall(SYS_perf_event_open, hw_event, pid, cpu, group_fd, flags);
  return ret;
}

static long raw_perf_event_open_default(struct perf_event_attr* hw_event)
{
  int ret;
  ret = raw_perf_event_open(hw_event, 0, -1, -1, 0);
  return ret;
}

static void init_perf_event_attr(struct perf_event_attr* pe)
{
  memset(pe, 0, sizeof(*pe));
  pe->size = sizeof(struct perf_event_attr);
  pe->disabled = 1;
  pe->exclude_kernel = 1;
  pe->exclude_idle = 1;
}

void perf_event_open(struct perf_fds* perf)
{
  // All the performance counter fds to create
  /* HW Counters */
  struct perf_event_attr count_hw_cpu_cycles;
  struct perf_event_attr count_hw_instructions;
  struct perf_event_attr count_hw_cache_references;
  struct perf_event_attr count_hw_cache_misses;
  struct perf_event_attr count_hw_branch_instructions;
  struct perf_event_attr count_hw_branch_misses;
  struct perf_event_attr count_hw_bus_cycles;
  /* SW Counters */
  struct perf_event_attr count_sw_cpu_clock;
  struct perf_event_attr count_sw_task_clock;
  struct perf_event_attr count_sw_page_faults;
  struct perf_event_attr count_sw_context_switches;
  struct perf_event_attr count_sw_cpu_migrations;

  // Set some default attributes
  init_perf_event_attr(&count_hw_cpu_cycles);
  init_perf_event_attr(&count_hw_instructions);
  init_perf_event_attr(&count_hw_cache_references);
  init_perf_event_attr(&count_hw_cache_misses);
  init_perf_event_attr(&count_hw_branch_instructions);
  init_perf_event_attr(&count_hw_branch_misses);
  init_perf_event_attr(&count_hw_bus_cycles);

  init_perf_event_attr(&count_sw_cpu_clock);
  init_perf_event_attr(&count_sw_task_clock);
  init_perf_event_attr(&count_sw_page_faults);
  init_perf_event_attr(&count_sw_context_switches);
  init_perf_event_attr(&count_sw_cpu_migrations);

  // Set up the rest of the specific attributes.
  count_hw_cpu_cycles.type = PERF_TYPE_HARDWARE;
  count_hw_instructions.type = PERF_TYPE_HARDWARE;
  count_hw_cache_references.type = PERF_TYPE_HARDWARE;
  count_hw_cache_misses.type = PERF_TYPE_HARDWARE;
  count_hw_branch_instructions.type = PERF_TYPE_HARDWARE;
  count_hw_branch_misses.type = PERF_TYPE_HARDWARE;
  count_hw_bus_cycles.type = PERF_TYPE_HARDWARE;

  count_sw_cpu_clock.type = PERF_TYPE_SOFTWARE;
  count_sw_task_clock.type = PERF_TYPE_SOFTWARE;
  count_sw_page_faults.type = PERF_TYPE_SOFTWARE;
  count_sw_context_switches.type = PERF_TYPE_SOFTWARE;
  count_sw_cpu_migrations.type = PERF_TYPE_SOFTWARE;

  count_hw_cpu_cycles.config = PERF_COUNT_HW_CPU_CYCLES;
  count_hw_instructions.config = PERF_COUNT_HW_INSTRUCTIONS;
  count_hw_cache_references.config = PERF_COUNT_HW_CACHE_REFERENCES;
  count_hw_cache_misses.config = PERF_COUNT_HW_CACHE_MISSES;
  count_hw_branch_instructions.config = PERF_COUNT_HW_BRANCH_INSTRUCTIONS;
  count_hw_branch_misses.config = PERF_COUNT_HW_BRANCH_MISSES;
  count_hw_bus_cycles.config = PERF_COUNT_HW_BUS_CYCLES;

  count_sw_cpu_clock.config = PERF_COUNT_SW_CPU_CLOCK;
  count_sw_task_clock.config = PERF_COUNT_SW_TASK_CLOCK;
  count_sw_page_faults.config = PERF_COUNT_SW_PAGE_FAULTS;
  count_sw_context_switches.config = PERF_COUNT_SW_CONTEXT_SWITCHES;
  count_sw_cpu_migrations.type = PERF_COUNT_SW_CPU_MIGRATIONS;

  perf->fds[0] = raw_perf_event_open_default(&count_hw_cpu_cycles);
  perf->fds[1] = raw_perf_event_open_default(&count_hw_instructions);
  perf->fds[2] = raw_perf_event_open_default(&count_hw_cache_references);
  perf->fds[3] = raw_perf_event_open_default(&count_hw_cache_misses);
  perf->fds[4] = raw_perf_event_open_default(&count_hw_branch_instructions);
  perf->fds[5] = raw_perf_event_open_default(&count_hw_branch_misses);
  perf->fds[6] = raw_perf_event_open_default(&count_hw_bus_cycles);
  perf->fds[7] = raw_perf_event_open_default(&count_sw_cpu_clock);
  perf->fds[8] = raw_perf_event_open_default(&count_sw_task_clock);
  perf->fds[9] = raw_perf_event_open_default(&count_sw_page_faults);
  perf->fds[10] = raw_perf_event_open_default(&count_sw_context_switches);
  perf->fds[11] = raw_perf_event_open_default(&count_sw_cpu_migrations);
}

void perf_event_close(struct perf_fds* perf)
{
  for (size_t i = 0; i < ARRAY_SIZE(perf->fds); ++i)
  {
    close(perf->fds[i]);
  }
}

void perf_start(struct perf_fds* perf)

{
  for (size_t i = 0; i < ARRAY_SIZE(perf->fds); ++i)
  {
    ioctl(perf->fds[i], PERF_EVENT_IOC_RESET, 0);
  }
  for (size_t i = 0; i < ARRAY_SIZE(perf->fds); ++i)
  {
    ioctl(perf->fds[i], PERF_EVENT_IOC_ENABLE, 0);
  }
}

void perf_stop(struct perf_fds* perf)
{
  for (size_t i = 0; i < ARRAY_SIZE(perf->fds); ++i)
  {
    ioctl(perf->fds[i], PERF_EVENT_IOC_DISABLE, 0);
  }
}

void perf_dump(struct perf_data* data, struct perf_fds* perf)
{
  for (size_t i = 0; i < ARRAY_SIZE(perf->fds); ++i)
  {
    read(perf->fds[i], &data->counters[i], sizeof(uint64_t));
  }
}

struct times get_times(int start, struct perf_fds* fds)
{
  if (!start)
  {
    perf_stop(fds);
  }

  struct timespec wall_buf;
  struct times result = {0};

  struct tms systimes;
  times(&systimes);

  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &wall_buf);

  result.user = systimes.tms_utime;
  result.system = systimes.tms_stime;

  result.wall_secs = wall_buf.tv_sec;
  result.wall_nsecs = wall_buf.tv_nsec;

  if (start)
  {
    perf_start(fds);
  }

  return result;
}

struct times elapsed(struct times* start, struct times* end,
                     struct perf_fds* perf)
{
  struct timespec tmp;

  if (end->wall_nsecs - start->wall_nsecs < 0)
  {
    tmp.tv_sec = end->wall_secs - start->wall_secs - 1;
    tmp.tv_nsec = 1e9 + end->wall_nsecs - start->wall_nsecs;
  }
  else
  {
    tmp.tv_sec = end->wall_secs - start->wall_secs;
    tmp.tv_nsec = end->wall_nsecs - start->wall_nsecs;
  }

  struct times result = {
      .user = end->user - start->user,
      .system = end->system - start->system,
      .wall_secs = tmp.tv_sec,
      .wall_nsecs = tmp.tv_nsec,
      .perf = {0},
  };

  perf_dump(&result.perf, perf);

  return result;
}

#endif
