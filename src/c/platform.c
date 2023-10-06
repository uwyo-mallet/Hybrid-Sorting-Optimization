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

void perf_event_open(struct perf_fds* fds)
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

  // clang-format off

  fds->count_hw_cpu_cycles = raw_perf_event_open_default(&count_hw_cpu_cycles);
  fds->count_hw_instructions = raw_perf_event_open_default(&count_hw_instructions);
  fds->count_hw_cache_references = raw_perf_event_open_default(&count_hw_cache_references);
  fds->count_hw_cache_misses = raw_perf_event_open_default(&count_hw_cache_misses);
  fds->count_hw_branch_instructions = raw_perf_event_open_default(&count_hw_branch_instructions);
  fds->count_hw_branch_misses = raw_perf_event_open_default(&count_hw_branch_misses);
  fds->count_hw_bus_cycles = raw_perf_event_open_default(&count_hw_bus_cycles);

  fds->count_sw_cpu_clock = raw_perf_event_open_default(&count_sw_cpu_clock);
  fds->count_sw_task_clock = raw_perf_event_open_default(&count_sw_task_clock);
  fds->count_sw_page_faults = raw_perf_event_open_default(&count_sw_page_faults);
  fds->count_sw_context_switches = raw_perf_event_open_default(&count_sw_context_switches);
  fds->count_sw_cpu_migrations = raw_perf_event_open_default(&count_sw_cpu_migrations);
  // clang-format on
}

void perf_event_close(struct perf_fds* fds)
{
  close(fds->count_hw_cpu_cycles);
  close(fds->count_hw_instructions);
  close(fds->count_hw_cache_references);
  close(fds->count_hw_cache_misses);
  close(fds->count_hw_branch_instructions);
  close(fds->count_hw_branch_misses);
  close(fds->count_hw_bus_cycles);

  close(fds->count_sw_cpu_clock);
  close(fds->count_sw_task_clock);
  close(fds->count_sw_page_faults);
  close(fds->count_sw_context_switches);
  close(fds->count_sw_cpu_migrations);
}

void perf_start(struct perf_fds* fds)
{
  ioctl(fds->count_hw_cpu_cycles, PERF_EVENT_IOC_RESET, 0);
  ioctl(fds->count_hw_instructions, PERF_EVENT_IOC_RESET, 0);
  ioctl(fds->count_hw_cache_references, PERF_EVENT_IOC_RESET, 0);
  ioctl(fds->count_hw_cache_misses, PERF_EVENT_IOC_RESET, 0);
  ioctl(fds->count_hw_branch_instructions, PERF_EVENT_IOC_RESET, 0);
  ioctl(fds->count_hw_branch_misses, PERF_EVENT_IOC_RESET, 0);
  ioctl(fds->count_hw_bus_cycles, PERF_EVENT_IOC_RESET, 0);
  ioctl(fds->count_sw_cpu_clock, PERF_EVENT_IOC_RESET, 0);
  ioctl(fds->count_sw_task_clock, PERF_EVENT_IOC_RESET, 0);
  ioctl(fds->count_sw_page_faults, PERF_EVENT_IOC_RESET, 0);
  ioctl(fds->count_sw_context_switches, PERF_EVENT_IOC_RESET, 0);
  ioctl(fds->count_sw_cpu_migrations, PERF_EVENT_IOC_RESET, 0);

  ioctl(fds->count_hw_cpu_cycles, PERF_EVENT_IOC_ENABLE, 0);
  ioctl(fds->count_hw_instructions, PERF_EVENT_IOC_ENABLE, 0);
  ioctl(fds->count_hw_cache_references, PERF_EVENT_IOC_ENABLE, 0);
  ioctl(fds->count_hw_cache_misses, PERF_EVENT_IOC_ENABLE, 0);
  ioctl(fds->count_hw_branch_instructions, PERF_EVENT_IOC_ENABLE, 0);
  ioctl(fds->count_hw_branch_misses, PERF_EVENT_IOC_ENABLE, 0);
  ioctl(fds->count_hw_bus_cycles, PERF_EVENT_IOC_ENABLE, 0);
  ioctl(fds->count_sw_cpu_clock, PERF_EVENT_IOC_ENABLE, 0);
  ioctl(fds->count_sw_task_clock, PERF_EVENT_IOC_ENABLE, 0);
  ioctl(fds->count_sw_page_faults, PERF_EVENT_IOC_ENABLE, 0);
  ioctl(fds->count_sw_context_switches, PERF_EVENT_IOC_ENABLE, 0);
  ioctl(fds->count_sw_cpu_migrations, PERF_EVENT_IOC_ENABLE, 0);
}

void perf_stop(struct perf_fds* fds)
{
  ioctl(fds->count_hw_cpu_cycles, PERF_EVENT_IOC_DISABLE, 0);
  ioctl(fds->count_hw_instructions, PERF_EVENT_IOC_DISABLE, 0);
  ioctl(fds->count_hw_cache_references, PERF_EVENT_IOC_DISABLE, 0);
  ioctl(fds->count_hw_cache_misses, PERF_EVENT_IOC_DISABLE, 0);
  ioctl(fds->count_hw_branch_instructions, PERF_EVENT_IOC_DISABLE, 0);
  ioctl(fds->count_hw_branch_misses, PERF_EVENT_IOC_DISABLE, 0);
  ioctl(fds->count_hw_bus_cycles, PERF_EVENT_IOC_DISABLE, 0);
  ioctl(fds->count_sw_cpu_clock, PERF_EVENT_IOC_DISABLE, 0);
  ioctl(fds->count_sw_task_clock, PERF_EVENT_IOC_DISABLE, 0);
  ioctl(fds->count_sw_page_faults, PERF_EVENT_IOC_DISABLE, 0);
  ioctl(fds->count_sw_context_switches, PERF_EVENT_IOC_DISABLE, 0);
  ioctl(fds->count_sw_cpu_migrations, PERF_EVENT_IOC_DISABLE, 0);
}

void perf_dump_to_csv(FILE* fp, int write_header, struct perf_fds* fds)
{
  if (write_header)
  {
    fprintf(
        fp,
        "hw_cpu_cycles,hw_instructions,hw_cache_references,hw_cache_misses,"
        "hw_branch_instructions,hw_branch_misses,sw_cpu_clock,sw_task_clock,"
        "sw_page_faults,sw_context_switches,sw_cpu_migrations\n");
  }

  uint64_t buffer;
  read(fds->count_hw_cpu_cycles, &buffer, sizeof(uint64_t));
  fprintf(fp, "%" PRIu64 ",", buffer);

  read(fds->count_hw_instructions, &buffer, sizeof(uint64_t));
  fprintf(fp, "%" PRIu64 ",", buffer);

  read(fds->count_hw_cache_references, &buffer, sizeof(uint64_t));
  fprintf(fp, "%" PRIu64 ",", buffer);

  read(fds->count_hw_cache_misses, &buffer, sizeof(uint64_t));
  fprintf(fp, "%" PRIu64 ",", buffer);

  read(fds->count_hw_branch_instructions, &buffer, sizeof(uint64_t));
  fprintf(fp, "%" PRIu64 ",", buffer);

  read(fds->count_hw_branch_misses, &buffer, sizeof(uint64_t));
  fprintf(fp, "%" PRIu64 ",", buffer);

  read(fds->count_sw_cpu_clock, &buffer, sizeof(uint64_t));
  fprintf(fp, "%" PRIu64 ",", buffer);

  read(fds->count_sw_task_clock, &buffer, sizeof(uint64_t));
  fprintf(fp, "%" PRIu64 ",", buffer);

  read(fds->count_sw_page_faults, &buffer, sizeof(uint64_t));
  fprintf(fp, "%" PRIu64 ",", buffer);

  read(fds->count_sw_context_switches, &buffer, sizeof(uint64_t));
  fprintf(fp, "%" PRIu64 ",", buffer);

  read(fds->count_sw_cpu_migrations, &buffer, sizeof(uint64_t));
  fprintf(fp, "%" PRIu64 "\n", buffer);
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

  clock_gettime(CLOCK_MONOTONIC_RAW, &wall_buf);

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

struct times elapsed(struct times* start, struct times* end)
{
  struct times result = {
      .user = end->user - start->user,
      .system = end->system - start->system,
      .wall_secs = end->wall_secs - start->wall_secs,
      .wall_nsecs = end->wall_nsecs - start->wall_nsecs,
  };

  return result;
}

#endif
