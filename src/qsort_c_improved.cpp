#include "sort.hpp"

// Stack allocation
#include <alloca.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

/*
 * Swap any two same sized value in-place by using stack allocation.
 */
void swap_any(void *a, void *b, size_t size)
{
  void *tmp = alloca(size);
  memcpy(tmp, a, size);
  memcpy(a, b, size);
  memcpy(b, tmp, size);
}

/*
 * Pure C implementation of insertion sort
 */
void insertion_sort_c(void *const arr, size_t n, size_t size, compar_d_fn_t cmp)
{
  if (n <= 1)
  {
    return;
  }

  char *const base_ptr = (char *)arr;
  char *const end = &base_ptr[size * (n - 1)];

  for (char *i = base_ptr + size; i <= end; i += size)
  {
    char *j = i;
    while (j > arr && (*cmp)((void *)j, (void *)(j - size)) < 0)
    {
      swap_any(j, j - size, size);
      j -= size;
    }
  }
}

/*
 * Attempted optimization of qsort_c while maintaining pure C compatibility.
 */
void qsort_c_improved(void *const arr, const size_t n, const size_t size,
                      compar_d_fn_t cmp, const size_t thresh)
{
  char *base_ptr = (char *)arr;

  if (n < thresh)
  {
    insertion_sort_c(arr, n, size, cmp);
    return;
  }

  stack_node<char> stack[STACK_SIZE];
  stack_node<char> *top = stack;

  char *lo;
  char *hi;

  const size_t max_thresh = thresh * size;

  PUSH(base_ptr, &base_ptr[size * (n - 1)]);
  while (STACK_NOT_EMPTY)
  {
    POP(lo, hi);

    char *mid = lo + size * ((hi - lo) / size >> 1);

    if ((*cmp)((void *)mid, (void *)lo) < 0)
    {
      swap_any((void *)mid, (void *)lo, size);
    }
    if ((*cmp)((void *)hi, (void *)mid) < 0)
    {
      swap_any((void *)mid, (void *)hi, size);
    }
    else
    {
      goto jump_over;
    }
    if ((*cmp)((void *)mid, (void *)lo) < 0)
    {
      swap_any((void *)mid, (void *)lo, size);
    }

  jump_over:
    char *left_ptr = lo + size;
    char *right_ptr = hi - size;

    do
    {
      while ((*cmp)((void *)left_ptr, (void *)mid) < 0)
      {
        left_ptr += size;
      }
      while ((*cmp)((void *)mid, (void *)right_ptr) < 0)
      {
        right_ptr -= size;
      }
      if (left_ptr < right_ptr)
      {
        swap_any((void *)left_ptr, (void *)right_ptr, size);
        if (mid == left_ptr)
        {
          mid = right_ptr;
        }
        else if (mid == right_ptr)
        {
          mid = left_ptr;
        }

        left_ptr += size;
        right_ptr -= size;
      }
      else if (left_ptr == right_ptr)
      {
        left_ptr += size;
        right_ptr -= size;
        break;
      }
    } while (left_ptr <= right_ptr);

    if ((right_ptr - lo) > max_thresh && right_ptr > lo)
    {
      PUSH(lo, right_ptr);
    }
    if ((hi - left_ptr) > max_thresh && left_ptr < hi)
    {
      PUSH(left_ptr, hi);
    }
  }

  char *const end = &base_ptr[size * (n - 1)];
  char *tmp_ptr = base_ptr;
  char *run_ptr;
  char *first_thresh = min(end, base_ptr + (size * thresh));

  for (run_ptr = tmp_ptr + size; run_ptr <= first_thresh; run_ptr += size)
  {
    if ((*cmp)((void *)run_ptr, (void *)tmp_ptr) < 0)
    {
      tmp_ptr = run_ptr;
    }
  }

  if (tmp_ptr != base_ptr)
  {
    swap_any((void *)tmp_ptr, (void *)base_ptr, size);
  }

  // One pass of insertion sort.
  insertion_sort_c(arr, n, size, cmp);
}
