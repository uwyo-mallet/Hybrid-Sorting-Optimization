/** Contains various sorting functions in pure C */

#define _GNU_SOURCE

#include "sort.h"

#include <alloca.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/**
 * Swap any two identically typed variables in-place.
 *
 * @param a: Pointer to first item
 * @param b: Pointer to second item
 * @param size: Size of item.
 */
static void swap_any(void *a, void *b, size_t size)
{
  char tmp[size];
  memcpy(tmp, a, size);
  memcpy(a, b, size);
  memcpy(b, tmp, size);
}

/**
 * Swap two doubles in-place.
 *
 * @param a: Pointer to first item
 * @param b: Pointer to second item
 */
static void swap_double(double *a, double *b)
{
  double tmp = *a;
  *a = *b;
  *b = tmp;
}

/**
 * Swap two floats in-place.
 *
 * @param a: Pointer to first item
 * @param b: Pointer to second item
 */
static void swap_float(float *a, float *b)
{
  float tmp = *a;
  *a = *b;
  *b = tmp;
}

/**
 * In-place Insertion sort with a parameterized compare function.
 *
 * @param arr: Array to be sorted.
 * @param total_elems: Length of array.
 * @param size: Size of an individual element in arr in bytes.
 * @param cmp: Comparator function, follows STL qsort convention.
 * @see compare()
 */
void insertion_sort_c(void *const arr, size_t total_elems, size_t size,
                      compar_d_fn_t cmp)
{
  if (total_elems <= 1)
  {
    return;
  }

  char *const base_ptr = (char *)arr;
  char *const end = &base_ptr[size * (total_elems - 1)];

  for (char *i = base_ptr + size; i <= end; i += size)
  {
    char *j = i;
    while (j > base_ptr && (*cmp)((void *)j, (void *)(j - size)) < 0)
    {
      swap_any(j, j - size, size);
      j -= size;
    }
  }
}

/**
 * In-place Insertion sort with parameterized swap and compare functions.
 *
 * @param arr: Array to be sorted.
 * @param total_elems: Length of array.
 * @param size: Size of an individual element in arr.
 * @param swp: Swap function.
 * @param cmp: Comparator function, follows STL qsort convention.
 * @see compare()
 */
void insertion_sort_c_swp(void *const arr, size_t total_elems, size_t size,
                          swp_fn_t swp, compar_d_fn_t cmp)
{
  if (total_elems <= 1)
  {
    return;
  }

  char *const base_ptr = (char *)arr;
  char *const end = &base_ptr[size * (total_elems - 1)];

  for (char *i = base_ptr + size; i <= end; i += size)
  {
    char *j = i;
    while (j > base_ptr && (*cmp)((void *)j, (void *)(j - size)) < 0)
    {
      (*swp)((void *)j, (void *)(j - size));
      j -= size;
    }
  }
}

/**
 * STL Qsort with parameterized swap and compare functions.
 *
 * @param pbase: Array to be sorted.
 * @param total_elems: Length of array.
 * @param size: Size of an individual element in arr.
 * @param swp: Swap function.
 * @param cmp: Comparator function, follows STL qsort convention.
 * @param thresh: Threshold to switch to insertion_sort_swp.
 * @see compare()
 */
void qsort_c_swp(void *const pbase, const size_t total_elems, const size_t size,
                 swp_fn_t swp, compar_d_fn_t cmp, const size_t thresh)
{
  /* Avoid lossage with unsigned arithmetic below. */
  if (total_elems == 0)
  {
    return;
  }

  char *base_ptr = (char *)pbase;
  const size_t max_thresh = thresh * size;

  if (total_elems > thresh)
  {
    char *lo = base_ptr;
    char *hi = &lo[size * (total_elems - 1)];

    stack_node stack[STACK_SIZE];
    stack_node *top = stack;

    PUSH(NULL, NULL);

    while (STACK_NOT_EMPTY)
    {
      char *left_ptr;
      char *right_ptr;

      /*
        Select median value from among LO, MID, and HI. Rearrange
        LO and HI so the three values are sorted. This lowers the
        probability of picking a pathological pivot value and
        skips a comparison for both the LEFT_PTR and RIGHT_PTR in
        the while loops.
      */
      char *mid = lo + size * ((hi - lo) / size >> 1);

      if ((*cmp)((void *)mid, (void *)lo) < 0)
      {
        (*swp)((void *)mid, (void *)lo);
      }
      if ((*cmp)((void *)hi, (void *)mid) < 0)
      {
        (*swp)((void *)mid, (void *)hi);
      }
      else
      {
        goto jump_over;
      }
      if ((*cmp)((void *)mid, (void *)lo) < 0)
      {
        (*swp)((void *)mid, (void *)lo);
      }

    jump_over:

      left_ptr = lo + size;
      right_ptr = hi - size;

      /*
        Here's the famous ``collapse the walls'' section of quicksort.
        Gotta like those tight inner loops!  They are the main reason
        that this algorithm runs much faster than others.
      */
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
          /* SWAP(left_ptr, right_ptr, size); */
          (*swp)((void *)left_ptr, (void *)right_ptr);
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

      /*
        Set up pointers for next iteration.  First determine whether
        left and right partitions are below the threshold size.  If so,
        ignore one or both.  Otherwise, push the larger partition's
        bounds on the stack and continue sorting the smaller one.
      */
      if ((size_t)(right_ptr - lo) <= max_thresh)
      {
        if ((size_t)(hi - left_ptr) <= max_thresh)
        {
          /* Ignore both small partitions. */
          POP(lo, hi);
        }
        else
        {
          /* Ignore small left partition. */
          lo = left_ptr;
        }
      }
      else if ((size_t)(hi - left_ptr) <= max_thresh)
      {
        /* Ignore small right partition. */
        hi = right_ptr;
      }
      else if ((right_ptr - lo) > (hi - left_ptr))
      {
        /* Push larger left partition indices. */
        PUSH(lo, right_ptr);
        lo = left_ptr;
      }
      else
      {
        /* Push larger right partition indices. */
        PUSH(left_ptr, hi);
        hi = right_ptr;
      }
    }
  }

  /*
    Once the BASE_PTR array is partially sorted by quicksort the rest
    is completely sorted using insertion sort, since this is efficient
    for partitions below MAX_THRESH size. BASE_PTR points to the beginning
    of the array to sort, and END_PTR points at the very last element in
    the array (*not* one beyond it!).
  */
  {
    char *const end_ptr = &base_ptr[size * (total_elems - 1)];
    char *tmp_ptr = base_ptr;
    char *thresh = MIN(end_ptr, base_ptr + max_thresh);
    char *run_ptr;

    /*
      Find smallest element in first threshold and place it at the
      array's beginning.  This is the smallest array element,
      and the operation speeds up insertion sort's inner loop.
    */
    for (run_ptr = tmp_ptr + size; run_ptr <= thresh; run_ptr += size)
    {
      if ((*cmp)((void *)run_ptr, (void *)tmp_ptr) < 0)
      {
        tmp_ptr = run_ptr;
      }
    }
    if (tmp_ptr != base_ptr)
    {
      (*swp)((void *)tmp_ptr, (void *)base_ptr);
    }

    /* Insertion sort, running from left-hand-side up to right-hand-side.  */
    run_ptr = base_ptr + size;
    while ((run_ptr += size) <= end_ptr)
    {
      tmp_ptr = run_ptr - size;
      while ((*cmp)((void *)run_ptr, (void *)tmp_ptr) < 0)
      {
        tmp_ptr -= size;
      }

      tmp_ptr += size;
      if (tmp_ptr != run_ptr)
      {
        char *trav;

        trav = run_ptr + size;
        while (--trav >= run_ptr)
        {
          char c = *trav;
          char *hi, *lo;

          for (hi = lo = trav; (lo -= size) >= tmp_ptr; hi = lo)
          {
            *hi = *lo;
          }
          *hi = c;
        }
      }
    }
  }
}

/**
 * In-place Insertion sort ripped from qsort with a parameterized compare
 * function.
 *
 * @param arr: Array to be sorted.
 * @param total_elems: Length of array.
 * @param size: Size of an individual element in arr.
 * @param cmp: Comparator function, follows STL qsort convention.
 * @see compare()
 */
void ins_sort(void const *arr, size_t total_elems, size_t size,
              compar_d_fn_t cmp)
{
  char *base_ptr = (char *)arr;
  char *const end_ptr = &base_ptr[size * (total_elems - 1)];
  char *tmp_ptr;

  /* Insertion sort, running from left-hand-side up to right-hand-side.  */
  char *run_ptr = base_ptr + size;
  while ((run_ptr += size) <= end_ptr)
  {
    tmp_ptr = run_ptr - size;
    while ((*cmp)((void *)run_ptr, (void *)tmp_ptr) < 0)
    {
      tmp_ptr -= size;
    }

    tmp_ptr += size;
    if (tmp_ptr != run_ptr)
    {
      char *trav;

      trav = run_ptr + size;
      while (--trav >= run_ptr)
      {
        char c = *trav;
        char *hi, *lo;

        for (hi = lo = trav; (lo -= size) >= tmp_ptr; hi = lo)
        {
          *hi = *lo;
        }
        *hi = c;
      }
    }
  }
}

/**
 * STL Qsort which uses a modified insertion sort routine.
 *
 * @param pbase: Array to be sorted.
 * @param total_elems: Length of array.
 * @param size: Size of an individual element in arr.
 * @param cmp: Comparator function, follows STL qsort convention.
 * @param thresh: Threshold to switch to insertion_sort_swp.
 */
void qsort_c_sep_ins(void *const pbase, const size_t total_elems,
                     const size_t size, compar_d_fn_t cmp, const size_t thresh)
{
  char *base_ptr = (char *)pbase;

  const size_t max_thresh = thresh * size;

  if (total_elems == 0)
  {
    /* Avoid lossage with unsigned arithmetic below. */
    return;
  }

  if (total_elems > thresh)
  {
    char *lo = base_ptr;
    char *hi = &lo[size * (total_elems - 1)];

    stack_node stack[STACK_SIZE];
    stack_node *top = stack;

    PUSH(NULL, NULL);

    while (STACK_NOT_EMPTY)
    {
      char *left_ptr;
      char *right_ptr;

      /*
        Select median value from among LO, MID, and HI. Rearrange
        LO and HI so the three values are sorted. This lowers the
        probability of picking a pathological pivot value and
        skips a comparison for both the LEFT_PTR and RIGHT_PTR in
        the while loops.
      */

      char *mid = lo + size * ((hi - lo) / size >> 1);

      if ((*cmp)((void *)mid, (void *)lo) < 0)
      {
        SWAP(mid, lo, size);
      }
      if ((*cmp)((void *)hi, (void *)mid) < 0)
      {
        SWAP(mid, hi, size);
      }
      else
      {
        goto jump_over;
      }
      if ((*cmp)((void *)mid, (void *)lo) < 0)
      {
        SWAP(mid, lo, size);
      }
    jump_over:

      left_ptr = lo + size;
      right_ptr = hi - size;

      /*
        Here's the famous ``collapse the walls'' section of quicksort.
        Gotta like those tight inner loops!  They are the main reason
        that this algorithm runs much faster than others.
      */
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
          SWAP(left_ptr, right_ptr, size);
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

      /*
        Set up pointers for next iteration.  First determine whether
        left and right partitions are below the threshold size.  If so,
        ignore one or both.  Otherwise, push the larger partition's
        bounds on the stack and continue sorting the smaller one.
      */
      if ((size_t)(right_ptr - lo) <= max_thresh)
      {
        if ((size_t)(hi - left_ptr) <= max_thresh)
        {
          /* Ignore both small partitions. */
          POP(lo, hi);
        }
        else
        {
          /* Ignore small left partition. */
          lo = left_ptr;
        }
      }
      else if ((size_t)(hi - left_ptr) <= max_thresh)
      {
        /* Ignore small right partition. */
        hi = right_ptr;
      }
      else if ((right_ptr - lo) > (hi - left_ptr))
      {
        /* Push larger left partition indices. */
        PUSH(lo, right_ptr);
        lo = left_ptr;
      }
      else
      {
        /* Push larger right partition indices. */
        PUSH(left_ptr, hi);
        hi = right_ptr;
      }
    }
  }

  /*
    Once the BASE_PTR array is partially sorted by quicksort the rest
    is completely sorted using insertion sort, since this is efficient
    for partitions below MAX_THRESH size. BASE_PTR points to the beginning
    of the array to sort, and END_PTR points at the very last element in
    the array (*not* one beyond it!).
  */

  {
    char *const end_ptr = &base_ptr[size * (total_elems - 1)];
    char *tmp_ptr = base_ptr;
    char *thresh = MIN(end_ptr, base_ptr + max_thresh);
    char *run_ptr;

    /*
      Find smallest element in first threshold and place it at the
      array's beginning.  This is the smallest array element,
      and the operation speeds up insertion sort's inner loop.
    */

    for (run_ptr = tmp_ptr + size; run_ptr <= thresh; run_ptr += size)
      if ((*cmp)((void *)run_ptr, (void *)tmp_ptr) < 0)
      {
        tmp_ptr = run_ptr;
      }

    if (tmp_ptr != base_ptr)
    {
      SWAP(tmp_ptr, base_ptr, size);
    }

    /* ins_sort(pbase, total_elems, size, cmp); */
    insertion_sort_c(pbase, total_elems, size, cmp);
  }
}

void qsort_sanity(void *const pbase, const size_t total_elems,
                  const size_t size, compar_d_fn_t cmp)
{
  qsort(pbase, total_elems, size, cmp);
}

struct msort_param
{
  size_t s;          /* Size of an individual element. */
  size_t var;        /* Variance */
  compar_d_fn_t cmp; /* Comparator function. */
  char *t;           /* Temporary storage space. */
};

static void msort_with_tmp(const struct msort_param *p, void *b, size_t n)
{
  char *b1, *b2;
  size_t n1, n2;

  if (n <= 1) return;

  n1 = n / 2;
  n2 = n - n1;
  b1 = b;
  b2 = (char *)b + (n1 * p->s);

  msort_with_tmp(p, b1, n1);
  msort_with_tmp(p, b2, n2);

  char *tmp = p->t;
  const size_t s = p->s;
  compar_d_fn_t cmp = p->cmp;
  switch (p->var)
  {
    case 0:
      while (n1 > 0 && n2 > 0)
      {
        if ((*cmp)(b1, b2) <= 0)
        {
          *(uint32_t *)tmp = *(uint32_t *)b1;
          b1 += sizeof(uint32_t);
          --n1;
        }
        else
        {
          *(uint32_t *)tmp = *(uint32_t *)b2;
          b2 += sizeof(uint32_t);
          --n2;
        }
        tmp += sizeof(uint32_t);
      }
      break;
    case 1:
      while (n1 > 0 && n2 > 0)
      {
        if ((*cmp)(b1, b2) <= 0)
        {
          *(uint64_t *)tmp = *(uint64_t *)b1;
          b1 += sizeof(uint64_t);
          --n1;
        }
        else
        {
          *(uint64_t *)tmp = *(uint64_t *)b2;
          b2 += sizeof(uint64_t);
          --n2;
        }
        tmp += sizeof(uint64_t);
      }
      break;
    case 2:
      while (n1 > 0 && n2 > 0)
      {
        unsigned long *tmpl = (unsigned long *)tmp;
        unsigned long *bl;

        tmp += s;
        if ((*cmp)(b1, b2) <= 0)
        {
          bl = (unsigned long *)b1;
          b1 += s;
          --n1;
        }
        else
        {
          bl = (unsigned long *)b2;
          b2 += s;
          --n2;
        }
        while (tmpl < (unsigned long *)tmp) *tmpl++ = *bl++;
      }
      break;
    case 3:
      while (n1 > 0 && n2 > 0)
      {
        if ((*cmp)(*(const void **)b1, *(const void **)b2) <= 0)
        {
          *(void **)tmp = *(void **)b1;
          b1 += sizeof(void *);
          --n1;
        }
        else
        {
          *(void **)tmp = *(void **)b2;
          b2 += sizeof(void *);
          --n2;
        }
        tmp += sizeof(void *);
      }
      break;
    default:
      while (n1 > 0 && n2 > 0)
      {
        if ((*cmp)(b1, b2) <= 0)
        {
          tmp = (char *)mempcpy(tmp, b1, s);
          b1 += s;
          --n1;
        }
        else
        {
          tmp = (char *)mempcpy(tmp, b2, s);
          b2 += s;
          --n2;
        }
      }
      break;
  }

  if (n1 > 0) memcpy(tmp, b1, n1 * s);
  memcpy(b, p->t, (n - n2) * s);
}

void msort_c(void *b, size_t n, size_t s, compar_d_fn_t cmp)
{
  size_t size = n * s;
  char *tmp = NULL;

  struct msort_param p;

  // For large object sizes use indirect sorting.
  if (s > 32)
  {
    size = 2 * n * sizeof(void *) + s;
  }

  // Size is relatively small, just stack allocate it.
  if (size < 1024)
  {
    p.t = alloca(size);
  }
  else
  {
    // Heap allocate otherwise.
    tmp = malloc(size);
    if (tmp == NULL)
    {
      // Don't try to sort if malloc fails.
      // Kill the program with an error.
      exit(1);
    }
    p.t = tmp;
  }

  p.s = s;
  p.var = 4;
  p.cmp = cmp;

  if (s > 32)
  {
    // Indirect sorting

    char *ip = (char *)b;
    void **tp = (void **)(p.t + n * sizeof(void *));
    void **t = tp;
    void *tmp_storage = (void *)(tp + n);

    while ((void *)t < tmp_storage)
    {
      *t++ = ip;
      ip += s;
    }
    p.s = sizeof(void *);
    p.var = 3;
    msort_with_tmp(&p, p.t + n * sizeof(void *), n);

    /* tp[0] .. tp[n - 1] is now sorted, copy around entries of
       the original array.  Knuth vol. 3 (2nd ed.) exercise 5.2-10.  */
    char *kp;
    size_t i;
    for (i = 0, ip = (char *)b; i < n; i++, ip += s)
      if ((kp = tp[i]) != ip)
      {
        size_t j = i;
        char *jp = ip;
        memcpy(tmp_storage, ip, s);

        do
        {
          size_t k = (kp - (char *)b) / s;
          tp[j] = jp;
          memcpy(jp, kp, s);
          j = k;
          jp = kp;
          kp = tp[k];
        } while (kp != ip);

        tp[j] = jp;
        memcpy(jp, tmp_storage, s);
      }
  }
  else
  {
    if ((s & (sizeof(uint32_t) - 1)) == 0 &&
        ((char *)b - (char *)0) % __alignof__(uint32_t) == 0)
    {
      if (s == sizeof(uint32_t))
        p.var = 0;
      else if (s == sizeof(uint64_t) &&
               ((char *)b - (char *)0) % __alignof__(uint64_t) == 0)
        p.var = 1;
      else if ((s & (sizeof(unsigned long) - 1)) == 0 &&
               ((char *)b - (char *)0) % __alignof__(unsigned long) == 0)
        p.var = 2;
    }
    msort_with_tmp(&p, b, n);
  }
  free(tmp);
}
