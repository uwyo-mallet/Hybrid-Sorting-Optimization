#pragma once
#ifndef SORT_HPP_
#define SORT_HPP_

#include <algorithm>
#include <boost/multiprecision/cpp_int.hpp>
#include <climits>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <set>
#include <string>

#include "platform.hpp"
#include "sort.h"

// Hybrid methods which support a threshold value.
// clang-format off
static const std::set<std::string> THRESHOLD_METHODS{
    "qsort_asm",
    "qsort_c",
    "qsort_c_no_comp",
    "qsort_c_sep_ins",
    "qsort_c_swp",
    "qsort_cpp",
    "qsort_cpp_no_comp",
};
// clang-format on

// x86 assembly methods
#ifdef ARCH_X86
extern "C" void insertion_sort_asm(uint64_t *input, const uint64_t size);
extern "C" void qsort_asm(uint64_t *arr, const uint64_t n,
                          const size_t threshold);
#endif  // ARCH_X86

/** Comparator for any qsort_*
@see https://en.cppreference.com/w/c/algorithm/qsort
*/
template <typename T>
int __attribute__((noinline)) compare(const void *ap, const void *bp)
{
  T *a = (T *)ap;
  T *b = (T *)bp;

  if (*a < *b) return -1;
  if (*a > *b) return 1;
  return 0;
}

/** Comparator for std::sort */
template <typename T>
bool __attribute__((noinline)) compare_std(const T &a, const T &b)
{
  return (a < b);
}

/**
  Determine if an array is sorted by checking every value.

  @param input: Input array to validate.
  @param len: Length of input array.
  @return Whether or not the array is sorted.
*/
template <typename T>
bool is_sorted(const T *input, const size_t &len)
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

/**
  Swap two values in-place
*/
template <typename T>
void swap(void *ap, void *bp)
{
  T *a = (T *)ap;
  T *b = (T *)bp;

  T tmp = *a;
  *a = *b;
  *b = tmp;
}

/**
  Iterative implementation of insertion sort

  @param input: Input array to sort.
  @param n: Length of input array.
  @param comp: Comparator function, follows STL qsort convention.
*/
template <typename T, typename Comparator>
void insertion_sort(T *input, const size_t &n, Comparator comp)
{
  if (n <= 1)
  {
    return;
  }

  T const *end = input + n;

  for (T *i = input + 1; i < end; i++)
  {
    T *j = i;
    while (j > input && comp(j, (j - 1)) < 0)
    {
      swap<T>(j, j - 1);
      j--;
    }
  }
}

/**
 * Iterative implementation of insertion sort
 * without a comparator function.
 *
 * @param input: Input array to sort.
 * @param n: Length of input array.
 */
template <typename T>
void insertion_sort(T *input, const size_t &n)
{
  if (n <= 1)
  {
    return;
  }

  T const *end = input + n;

  for (T *i = input + 1; i < end; i++)
  {
    T *j = i;
    while (*j < *(j - 1))
    {
      swap<T>(j, j - 1);
      j--;
    }
  }
}

/**
 * A node to 'push' onto stack for iterative quicksort.
 * Lo and hi pointers for subarrays.
 */
template <typename T>
struct node
{
  T *lo; /** Lower bound of subarray */
  T *hi; /** Upper bound of subarray */
};

/**
 * Hybrid quicksort-insertion sort using modern C++ features.
 *
 * @param arr: Input array to sort.
 * @param n: Length of arr.
 * @param thresh: Threshold at which to switch to insertion sort.
 * @param comp: Comparator function, follows STL qsort conventions.
 * @see compare()
 */
template <typename T, typename Comparator>
void qsort_cpp(T *arr, const size_t &n, const size_t &thresh, Comparator comp)
{
  if (n < thresh)
  {
    insertion_sort(arr, n, comp);
    return;
  }

  node<T> stack[STACK_SIZE];
  node<T> *top = stack;

  PUSH(arr, arr + n - 1);

  T *hi;
  T *lo;

  while (STACK_NOT_EMPTY)
  {
    POP(lo, hi);

    T *mid = lo + ((hi - lo) >> 1);

    if (comp(mid, lo) < 0)
    {
      swap<T>(mid, lo);
    }
    if (comp(hi, mid) < 0)
    {
      swap<T>(mid, hi);
    }
    else
    {
      goto jump_over;
    }
    if (comp(mid, lo) < 0)
    {
      swap<T>(mid, lo);
    }

  jump_over:

    T *left_ptr = lo + 1;
    T *right_ptr = hi - 1;

    do
    {
      while (comp(left_ptr, mid) < 0)
      {
        left_ptr++;
      }
      while (comp(mid, right_ptr) < 0)
      {
        right_ptr--;
      }
      if (left_ptr < right_ptr)
      {
        swap<T>(left_ptr, right_ptr);
        if (mid == left_ptr)
        {
          mid = right_ptr;
        }
        else if (mid == right_ptr)
        {
          mid = left_ptr;
        }

        left_ptr++;
        right_ptr--;
      }
      else if (left_ptr == right_ptr)
      {
        left_ptr++;
        right_ptr--;
        break;
      }
    } while (left_ptr <= right_ptr);

    if ((right_ptr - lo) > thresh && right_ptr > lo)
    {
      PUSH(lo, right_ptr);
    }
    if ((hi - left_ptr) > thresh && left_ptr < hi)
    {
      PUSH(left_ptr, hi);
    }
  }

  T *const end = arr + n - 1;
  T *tmp_ptr = arr;
  T *run_ptr;
  T *first_thresh = min(end, arr + thresh);
  /*
    Find smallest element in first threshold and place it at the
    array's beginning.  This is the smallest array element,
    and the operation speeds up insertion sort's inner loop.
  */
  for (run_ptr = tmp_ptr + 1; run_ptr <= first_thresh; run_ptr++)
  {
    if (comp(run_ptr, tmp_ptr) < 0)
    {
      tmp_ptr = run_ptr;
    }
  }
  if (tmp_ptr != arr)
  {
    swap<T>(tmp_ptr, arr);
  }

  // One pass of insertion sort.
  insertion_sort(arr, n, comp);
}

/**
 * Hard code the comparison function (int < int).
 * In theory, this is already done for the simple test case through compiler
 * optimization, __attribute__ ((noinline)) on the comparison functions should
 * ensure this doesn't happen. Thus, we manually add a guaranteed inline'ed
 * test.
 *
 * @param arr: Input array to sort.
 * @param n: Length of arr.
 * @param thresh: Threshold at which to switch to insertion sort.
 */
template <typename T>
void qsort_cpp_no_comp(T *arr, const size_t &n, const size_t &thresh)
{
  if (n < thresh)
  {
    insertion_sort(arr, n);
    return;
  }

  node<T> stack[STACK_SIZE];
  node<T> *top = stack;

  PUSH(arr, arr + n - 1);

  T *hi;
  T *lo;

  while (STACK_NOT_EMPTY)
  {
    POP(lo, hi);

    T *mid = lo + ((hi - lo) >> 1);

    if (*mid < *lo)
    {
      swap<T>(mid, lo);
    }
    if (*hi < *mid)
    {
      swap<T>(mid, hi);
    }
    else
    {
      goto jump_over;
    }
    if (*mid < *lo)
    {
      swap<T>(mid, lo);
    }

  jump_over:

    T *left_ptr = lo + 1;
    T *right_ptr = hi - 1;

    do
    {
      while (*left_ptr < *mid)
      {
        left_ptr++;
      }
      while (*mid < *right_ptr)
      {
        right_ptr--;
      }
      if (left_ptr < right_ptr)
      {
        swap<T>(left_ptr, right_ptr);
        if (mid == left_ptr)
        {
          mid = right_ptr;
        }
        else if (mid == right_ptr)
        {
          mid = left_ptr;
        }

        left_ptr++;
        right_ptr--;
      }
      else if (left_ptr == right_ptr)
      {
        left_ptr++;
        right_ptr--;
        break;
      }
    } while (left_ptr <= right_ptr);

    if ((right_ptr - lo) > thresh && right_ptr > lo)
    {
      PUSH(lo, right_ptr);
    }
    if ((hi - left_ptr) > thresh && left_ptr < hi)
    {
      PUSH(left_ptr, hi);
    }
  }

  T *const end = arr + n - 1;
  T *tmp_ptr = arr;
  T *run_ptr;
  T *first_thresh = min(end, arr + thresh);
  /*
    Find smallest element in first threshold and place it at the
    array's beginning.  This is the smallest array element,
    and the operation speeds up insertion sort's inner loop.
  */
  for (run_ptr = tmp_ptr + 1; run_ptr <= first_thresh; run_ptr++)
  {
    if (*run_ptr < *tmp_ptr)
    {
      tmp_ptr = run_ptr;
    }
  }
  if (tmp_ptr != arr)
  {
    swap<T>(tmp_ptr, arr);
  }

  // One pass of insertion sort.
  insertion_sort(arr, n);
}

/**
 * Vanilla quicksort with no hybridization.
 *
 * @param arr: Input array to sort.
 * @param n: Length of arr.
 * @param comp: Comparator function, follows STL qsort conventions.
 * @see compare()
 */
template <typename T, typename Comparator>
void qsort_vanilla(T *arr, const size_t &n, Comparator comp)
{
  node<T> stack[STACK_SIZE];
  node<T> *top = stack;

  PUSH(arr, arr + n - 1);

  T *hi;
  T *lo;

  while (STACK_NOT_EMPTY)
  {
    POP(lo, hi);

    T *mid = lo + ((hi - lo) >> 1);

    if (comp(mid, lo) < 0)
    {
      swap<T>(mid, lo);
    }
    if (comp(hi, mid) < 0)
    {
      swap<T>(mid, hi);
    }
    else
    {
      goto jump_over;
    }
    if (comp(mid, lo) < 0)
    {
      swap<T>(mid, lo);
    }

  jump_over:

    T *left_ptr = lo + 1;
    T *right_ptr = hi - 1;

    do
    {
      while (comp(left_ptr, mid) < 0)
      {
        left_ptr++;
      }
      while (comp(mid, right_ptr) < 0)
      {
        right_ptr--;
      }
      if (left_ptr < right_ptr)
      {
        swap<T>(left_ptr, right_ptr);
        if (mid == left_ptr)
        {
          mid = right_ptr;
        }
        else if (mid == right_ptr)
        {
          mid = left_ptr;
        }

        left_ptr++;
        right_ptr--;
      }
      else if (left_ptr == right_ptr)
      {
        left_ptr++;
        right_ptr--;
        break;
      }
    } while (left_ptr <= right_ptr);

    if (right_ptr > lo)
    {
      PUSH(lo, right_ptr);
    }
    if (left_ptr < hi)
    {
      PUSH(left_ptr, hi);
    }
  }
}

#endif /* SORT_HPP_ */
