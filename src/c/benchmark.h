#ifndef BENCHMARK_H_
#define BENCHMARK_H_

#include <inttypes.h>
#include <time.h>

#include "platform.h"

struct times measure_sort_time(int method, int64_t* data, const size_t n,
                               const int threshold);

#endif  // BENCHMARK_H_
