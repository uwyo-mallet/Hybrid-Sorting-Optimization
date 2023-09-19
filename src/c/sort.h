#ifndef SORT_H_
#define SORT_H_

#include <stdio.h>
#include <stdlib.h>

#if defined(SORT_LARGE_STRUCTS)
#define PAD_SIZE 4 * 1024
typedef struct
{
  // Volatile to prevent optimizer from removing this altogether.
  volatile char c[PAD_SIZE];
  int64_t val;
} sort_t;
#elif defined(SORT_INTS)
typedef int sort_t;
#else
typedef int64_t sort_t;
#endif  // SORT_LARGE_STRUCTS

extern const char* METHODS[];
enum METHOD_TOK
{
  QSORT = 0,
  MSORT_HEAP,
  BASIC_INS,
  FAST_INS,
  SHELL,
  DUMMY1, /* Methods from this point support a threshold value. */
  MSORT_HEAP_WITH_OLD_INS,
  MSORT_HEAP_WITH_BASIC_INS,
  MSORT_HEAP_WITH_SHELL,
  MSORT_HEAP_WITH_FAST_INS,
  MSORT_HEAP_WITH_NETWORK,
  MSORT_WITH_NETWORK,
  QUICKSORT_WITH_INS,
  QUICKSORT_WITH_FAST_INS,
  DUMMY2, /* Methods from this point forward are AlphaDev related. */
  ALPHA_DEV_SORT3,
  ALPHA_DEV_SORT4,
  ALPHA_DEV_SORT5,
  ALPHA_DEV_SORT6,
  ALPHA_DEV_SORT7,
  ALPHA_DEV_SORT8,
  ALPHA_DEV_SORT3_VAR,
  ALPHA_DEV_SORT4_VAR,
  ALPHA_DEV_SORT5_VAR,
};

#define UINT32 0
#define UINT64 1
#define ULONG 2
#define PTR 3
#define DEFAULT 4

typedef int (*compar_d_fn_t)(const void*, const void*);
struct msort_param
{
  size_t s;          /* Size of an individual element. */
  unsigned var;      /* Variance */
  compar_d_fn_t cmp; /* Comparator function. */
  char* t;           /* Temporary storage space. */
};

int sort_t_compare(const void* a, const void* b);

void msort_heap(void* b, size_t n, size_t s, compar_d_fn_t cmp);
void basic_ins_sort(void* b, size_t n, size_t s, compar_d_fn_t cmp);
void fast_ins_sort(void* b, size_t n, size_t s, compar_d_fn_t cmp);
void ins_sort(const struct msort_param* const p, void* b, const size_t n);

void msort_heap_with_old_ins(void* b, size_t n, size_t s, compar_d_fn_t cmp,
                             const size_t threshold);
void msort_heap_with_basic_ins(void* b, size_t n, size_t s, compar_d_fn_t cmp,
                               const size_t threshold);
void shell_sort(void* b, size_t n, size_t s, compar_d_fn_t cmp);
void msort_heap_with_shell(void* b, size_t n, size_t s, compar_d_fn_t cmp,
                           const size_t threshold);
void msort_heap_with_fast_ins(void* b, size_t n, size_t s, compar_d_fn_t cmp,
                              const size_t threshold);
void msort_heap_with_network(void* b, size_t n, size_t s, compar_d_fn_t cmp,
                             const size_t threshold);
void msort_with_network(void* b, size_t n, size_t s, compar_d_fn_t cmp,
                        const size_t threshold);

void quicksort_with_ins(void* b, size_t n, size_t s, compar_d_fn_t cmp,
                        const size_t threshold);
void quicksort_with_fast_ins(void* b, size_t n, size_t s, compar_d_fn_t cmp,
                             const size_t threshold);

void alphadev_sort3(int* b);
void alphadev_sort4(int* b);
void alphadev_sort5(int* b);
void alphadev_sort6(int* b);
void alphadev_sort7(int* b);
void alphadev_sort8(int* b);

void alphadev_sort3_var(int* b);
void alphadev_sort4_var(int* b);
void alphadev_sort5_var(int* b);

#endif  // SORT_H_
