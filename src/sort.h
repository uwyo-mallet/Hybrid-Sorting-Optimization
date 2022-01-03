#ifndef SORT_H_
#define SORT_H_

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

typedef struct
{
  char *lo;
  char *hi;
} stack_node;

/* The next 4 #defines implement a very fast in-line stack abstraction. */
/* The stack needs log(total_elements) entries (we could even subtract
   log(THRESHOLD)). Since total_elements has type size_t, we get as
   upper bound for log(total_elements): bits per byte (CHAR_BIT) *
   sizeof(size_t).
 */
#define STACK_SIZE (CHAR_BIT * sizeof(size_t))
#define PUSH(low, high) (((top->lo = (low)), (top->hi = (high)), ++top))
#define POP(low, high) ((--top, (low = top->lo), (high = top->hi)))
#define STACK_NOT_EMPTY (stack < top)
#define MIN(x, y) ((x) < (y) ? (x) : (y))

#define SWAP(a, b, size)         \
  do                             \
  {                              \
    size_t __size = (size);      \
    char *__a = (a), *__b = (b); \
    do                           \
    {                            \
      char __tmp = *__a;         \
      *__a++ = *__b;             \
      *__b++ = __tmp;            \
    } while (--__size > 0);      \
  } while (0)

#if defined(__cplusplus)
extern "C"
{
#endif

  // Legacy fully C compatible sort functions.
  typedef int (*compar_d_fn_t)(const void *, const void *);
  typedef void (*swp_fn_t)(void *, void *);

  // No Threshold --------------------------------------------------------------
  void insertion_sort_c(void *const arr, size_t n, size_t size,
                        compar_d_fn_t cmp);
  void insertion_sort_c_swp(void *const arr, size_t n, size_t size,
                            swp_fn_t swp, compar_d_fn_t cmp);

  // Threshold -----------------------------------------------------------------
  void qsort_c(void *const pbase, const size_t total_elems, const size_t size,
               compar_d_fn_t cmp, const size_t thresh);
  void qsort_c_sep_ins(void *const pbase, const size_t total_elems,
                       const size_t size, compar_d_fn_t cmp,
                       const size_t thresh);
  void qsort_c_swp(void *const arr, const size_t n, const size_t size,
                   swp_fn_t swp, compar_d_fn_t cmp, const size_t thresh);

  void qsort_sanity(void *const pbase, const size_t total_elems,
                    const size_t size, compar_d_fn_t cmp);
#if defined(__cplusplus)
}
#endif

#endif  // SORT_H_
