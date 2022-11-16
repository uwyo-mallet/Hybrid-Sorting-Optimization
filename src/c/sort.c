#include "sort.h"

#include <inttypes.h>

int int64_t_compare(const void* a, const void* b)
{
  int64_t* A = (int64_t*)a;
  int64_t* B = (int64_t*)b;
  return (*A < *B);
}
