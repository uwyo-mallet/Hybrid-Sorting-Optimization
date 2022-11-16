#include "platform.h"

#ifdef linux

#include <stdlib.h>

struct times get_times()
{
  struct timespec wall_buf;
  struct times result = {0};

  struct tms systimes;
  times(&systimes);

  clock_gettime(CLOCK_MONOTONIC_RAW, &wall_buf);

  result.user = systimes.tms_utime;
  result.system = systimes.tms_stime;

  result.wall_secs = wall_buf.tv_sec;
  result.wall_nsecs = wall_buf.tv_nsec;

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
