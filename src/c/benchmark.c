#include "benchmark.h"

#include <stdlib.h>

#include "platform.h"
#include "sort.h"

struct times measure_sort_time(int method, int64_t* data, const size_t n,
                               const int threshold)
{
  struct times start;
  struct times end;

  switch (method)
  {
    /* Standard Methods ----------------------------------------------------- */
    case QSORT:
      start = get_times();
      qsort(data, n, sizeof(int64_t), int64_t_compare);
      break;
    /* Hybrid Methods ------------------------------------------------------- */
    case MSORT_HYBRID:
      break;
    default:
      // How tf did you end up here?
      break;
  }

  end = get_times();
  return elapsed(&start, &end);
}
