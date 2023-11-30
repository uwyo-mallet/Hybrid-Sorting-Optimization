#include "sort_cxx.h"

#include <algorithm>
#include <iostream>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void cxx_std_sort(void* b, size_t n, size_t s, compar_d_fn_t cmp)
{
  auto my_comparator = [=](sort_t a, sort_t b)
  { return (*cmp)((void*)&a, (void*)&b) < 0; };
  sort_t* start = (sort_t*)b;
  sort_t* end = start + n;
  std::sort(start, end, my_comparator);
}
#pragma GCC diagnostic pop
