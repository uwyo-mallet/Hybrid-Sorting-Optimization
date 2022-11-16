#ifndef BENCHMARK_H_
#define BENCHMARK_H_

#include <inttypes.h>
#include <time.h>

extern const char* METHODS[];
enum METHOD_TOK
{
  QSORT = 0,
  EMPTY,
  MSORT_HYBRID,
};

struct times
{
  double user;
  double system;
  long wall;
};

struct times measure_sort_time(int method, int64_t* data, const size_t n,
                               const int threshold);

#endif  // BENCHMARK_H_
