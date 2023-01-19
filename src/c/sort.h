#ifndef SORT_H_
#define SORT_H_

#include <stdio.h>
#include <stdlib.h>

extern const char* METHODS[];
enum METHOD_TOK
{
  QSORT = 0,
  MSORT_HEAP,
  EMPTY,
  MSORT_HEAP_WITH_OLD_INS,
  MSORT_HEAP_WITH_BASIC_INS,
  MSORT_HEAP_WITH_SHELL,
};

typedef int (*compar_d_fn_t)(const void*, const void*);
int int64_t_compare(const void* a, const void* b);

void msort_heap(void* b, size_t n, size_t s, compar_d_fn_t cmp);
void msort_heap_with_old_ins(void* b, size_t n, size_t s, compar_d_fn_t cmp,
                             const size_t threshold);
void msort_heap_with_basic_ins(void* b, size_t n, size_t s, compar_d_fn_t cmp,
                               const size_t threshold);
void msort_heap_with_shell(void* b, size_t n, size_t s, compar_d_fn_t cmp,
                           const size_t threshold);
#endif  // SORT_H_
