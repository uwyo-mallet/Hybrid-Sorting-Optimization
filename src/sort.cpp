#include "sort.hpp"

#include <algorithm>
#include <climits>
#include <iostream>

/*
  Determine if an array is sorted by checking every value.

  @param input: Input array to validate.
  @param len: Length of input array.
  @return: Whether or not the array is sorted.
*/
template <typename T>
bool is_sorted(T input[], const size_t &len)
{
  for (size_t i = 0; i < len - 1; i++)
  {
    if (input[i] > input[i + 1])
    {
      return false;
    }
  }

  return true;
}

/*
        Swap two values in-place
*/
template <typename T>
void swap(T *a, T *b)
{
  T tmp = *a;
  *a = *b;
  *b = tmp;
}

/*
  Given 3 values, sort them in place, and find the median.
  The values are always sorted as, a <= b <= c.

  @return: Pointer to median value (b)
*/
template <typename T>
T *median_of_three(T *a, T *b, T *c)
{
  if (*a > *c)
  {
    swap(a, c);
  }
  if (*a > *b)
  {
    swap(a, b);
  }
  if (*b > *c)
  {
    swap(b, c);
  }

  return b;
}

/*
  Iterative implementation of insertion sort

  @param input: Input array to sort.
  @param len: Length of input array.
*/
template <typename T>
void insertion_sort(T input[], const size_t &len)
{
  for (size_t i = 1; i < len; i++)
  {
    size_t j = i;
    while (j > 0 && input[j - 1] > input[j])
    {
      swap(&input[j], &input[j - 1]);
      j--;
    }
  }
}

/*
  Partition the array for swapping.

  @param input: Current subarray.
  @param lo: Low index of current subarray.
  @param hi: High index of current subarray.
  @return: Index of new pivot value.
*/
template <typename T>
size_t vanilla_partition(T input[], size_t lo, size_t hi)
{
  size_t p = lo;
  T pivot_val = input[p];

  for (size_t i = lo + 1; i <= hi; i++)
  {
    if (input[i] <= pivot_val)
    {
      p++;
      swap(&input[i], &input[p]);
    }
  }
  swap(&input[p], &input[lo]);

  return p;
}

/*
  Recursive implementation of vanilla quicksort.
  Pivot is the first of each subarray.

  @param input: Input array to sort.
  @param lo: Low index of current subarray.
  @param hi: High index of current subarray.
  @return: Number of recursive calls
*/
template <typename T>
void vanilla_quicksort(T input[], int lo, int hi)
{
  if (lo < hi)
  {
    size_t pivot_index = vanilla_partition(input, lo, hi);
    vanilla_quicksort(input, lo, pivot_index - 1);
    vanilla_quicksort(input, pivot_index + 1, hi);
  }
}

/*
  Recursive implementation of vanilla quicksort.
  Pivot is the first of each subarray.

  @param input: Input array to sort.
  @param len: Length of input array.
  @return: Number of recursive calls.
*/
template <typename T>
void vanilla_quicksort(T input[], const size_t &len)
{
  vanilla_quicksort(input, 0, len - 1);
}

/* Discontinue optimized quicksort when the partition gets below this size */
#define QSORT_MAX_THRESH 4

/* Stack node declarations used to store unfulfilled partition obligations. */
typedef struct
{
  char *lo;
  char *hi;
} stack_node;

/* The next 4 #defines implement a very fast in-line stack abstraction. */
/* The stack needs log(total_elements) entries (we could even subtract
   log(QSORT_MAX_THRESH)). Since total_elements has type size_t, we get as
   upper bound for log(total_elements): bits per byte (CHAR_BIT) *
   sizeof(size_t).  */
#define STACK_SIZE (CHAR_BIT * sizeof(size_t))
#define PUSH(low, high) ((void)((top->lo = (low)), (top->hi = (high)), ++top))
#define POP(low, high) ((void)(--top, (low = top->lo), (high = top->hi)))
#define STACK_NOT_EMPTY (stack < top)

/* Byte-wise swap two items of size SIZE. */
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

/*
  Iterative, extremely optimized implementation of quicksort.
  Pivot is the median of 3 (first, mid, last) values of array.

  This implementation incorporates four optimizations discussed in
   Sedgewick:

  1. Non-recursive, using an explicit stack of pointer that store the
     next array partition to sort.  To save time, this maximum amount
     of space required to store an array of SIZE_MAX is allocated on the
     stack.  Assuming a 32-bit (64 bit) integer for size_t, this needs
     only 32 * sizeof(stack_node) == 256 bytes (for 64 bit: 1024 bytes).
     Pretty cheap, actually.

  2. Choose the pivot element using a median-of-three decision tree.
     This reduces the probability of selecting a bad pivot value and
     eliminates certain extraneous comparisons.

  3. Only quicksorts TOTAL_ELEMS / QSORT_MAX_THRESH partitions, leaving
     insertion sort to order the QSORT_MAX_THRESH items within each
     partition. This is a big win, since insertion sort is faster for small,
    mostly sorted array segments.

  4. The larger of the two sub-partitions is always pushed onto the
     stack first, with the algorithm then concentrating on the
     smaller partition.  This *guarantees* no more than log (total_elems)
     stack size is needed (actually O(1) in this case)!

  @param pbase: Pointer to start of array to sort.
  @param total_elems: Total number of elements in array.
  @param size: Size of element in array.
  @param cmp: Comparator function
  (https://www.cplusplus.com/reference/cstdlib/qsort/)
*/
void qsort_c(void *const pbase, size_t total_elems, size_t size,
             compar_d_fn_t cmp)
{
  char *base_ptr = (char *)pbase;

  const size_t max_thresh = QSORT_MAX_THRESH * size;

  if (total_elems == 0)
  {
    /* Avoid lossage with unsigned arithmetic below. */
    return;
  }

  if (total_elems > QSORT_MAX_THRESH)
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

#define min(x, y) ((x) < (y) ? (x) : (y))

  {
    char *const end_ptr = &base_ptr[size * (total_elems - 1)];
    char *tmp_ptr = base_ptr;
    char *thresh = min(end_ptr, base_ptr + max_thresh);
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

/* Comparator for qsort_c */
// TODO: this may have a performance impact in the real world, investigate.
int compare(const void *a, const void *b) { return (*(int *)a - *(int *)b); }
template <typename T>
void qsort_c(T input[], const size_t &len)
{
  qsort_c(input, len, sizeof(T), compare);
}

/* void qsort_c(void *const pbase, size_t total_elems, size_t size, */
/*     compar_d_fn_t cmp) */

// Template forward decleration to fix linker issues
template bool is_sorted<int>(int input[], const size_t &len);
template bool is_sorted<float>(float input[], const size_t &len);

template void swap<int>(int *a, int *b);
template void swap<float>(float *a, float *b);
template void swap<size_t>(size_t *a, size_t *b);

template int *median_of_three<int>(int *a, int *b, int *c);
template float *median_of_three<float>(float *a, float *b, float *c);

template void insertion_sort<int>(int input[], const size_t &len);
template void insertion_sort<float>(float input[], const size_t &len);

// Partition
template size_t vanilla_partition<int>(int input[], size_t low, size_t high);
template size_t vanilla_partition<float>(float input[], size_t low,
                                         size_t high);

// Recursive calls
template void vanilla_quicksort<int>(int input[], int low, int high);
template void vanilla_quicksort<float>(float input[], int low, int high);

// User calls
template void vanilla_quicksort<int>(int input[], const size_t &len);
template void vanilla_quicksort<float>(float input[], const size_t &len);

template void qsort_c<int>(int input[], const size_t &len);
template void qsort_c<float>(float input[], const size_t &len);
