#ifndef SORT_CXX_H
#define SORT_CXX_H

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

#include <stdint.h>

#include "sort.h"

EXTERNC void cxx_std_sort(void* b, size_t n, size_t s, compar_d_fn_t cmp);

#endif  // SORT_CXX_H
