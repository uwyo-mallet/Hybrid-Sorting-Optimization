#include "platform.h"

#ifdef linux

struct times get_times()
{
  struct timespec wall_buf;
  struct tms buf;
  struct times result = {0};

  clock_gettime(CLOCK_REALTIME, &wall_buf);

  // TODO: Fix broken user/system times
  result.user = 0;
  result.system = 0;
  result.wall = wall_buf.tv_nsec;

  return result;
}

struct times elapsed(struct times* start, struct times* end)
{
  struct times result = {
      .system = end->system - start->system,
      .user = end->user - start->user,
      .wall = end->wall - start->wall,
  };

  return result;
}

#endif
