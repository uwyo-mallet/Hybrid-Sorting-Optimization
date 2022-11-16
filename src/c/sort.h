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
  MSORT_HEAP_HYBRID_INS,
  MSORT_HEAP_HYBRID_INS_ITER,
};

typedef int (*compar_d_fn_t)(const void*, const void*);
int int64_t_compare(const void* a, const void* b);

void msort_heap(void* b, size_t n, size_t s, compar_d_fn_t cmp);
void msort_heap_hybrid_ins(void* b, size_t n, size_t s, compar_d_fn_t cmp,
                           const size_t threshold);
void msort_heap_hybrid_ins_iter(void* b, size_t n, size_t s, compar_d_fn_t cmp,
                                const size_t threshold);

#endif  // SORT_H_
