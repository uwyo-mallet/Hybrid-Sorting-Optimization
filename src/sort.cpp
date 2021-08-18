
#include "sort.hpp"

#include <algorithm>
#include <boost/multiprecision/cpp_int.hpp>
#include <climits>
#include <cstdint>
#include <iostream>

namespace bmp = boost::multiprecision;

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

template <typename T>
struct stack_node
{
  T *lo;
  T *hi;
};
#define STACK_SIZE (CHAR_BIT * sizeof(size_t))
#define PUSH(low, high) (((top->lo = (low)), (top->hi = (high)), ++top))
#define POP(low, high) ((--top, (low = top->lo), (high = top->hi)))
#define STACK_NOT_EMPTY (stack < top)
#define min(x, y) ((x) < (y) ? (x) : (y))

template <typename T>
void qsort_cpp(T arr[], const size_t &n, const size_t &thresh)
{
  stack_node<T> stack[STACK_SIZE];
  stack_node<T> *top = stack;

  PUSH(arr, arr + n - 1);

  T *hi;
  T *lo;

  while (STACK_NOT_EMPTY)
  {
    POP(lo, hi);

    T *mid = lo + ((hi - lo) >> 1);

    if (*mid < *lo)
    {
      swap(mid, lo);
    }
    if (*hi < *mid)
    {
      swap(mid, hi);
    }
    else
    {
      goto jump_over;
    }
    if (*mid < *lo)
    {
      swap(mid, lo);
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
        swap(left_ptr, right_ptr);
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
    swap(tmp_ptr, arr);
  }

  insertion_sort(arr, n);
}

/* Comparator for std::sort */
template <typename T>
bool compare_std(const T &a, const T &b)
{
  return (a < b);
}

// Template forward decleration to fix linker issues
template bool is_sorted<int>(int input[], const size_t &len);
template bool is_sorted<uint64_t>(uint64_t input[], const size_t &len);
template bool is_sorted<bmp::cpp_int>(bmp::cpp_int input[], const size_t &len);

template void swap<int>(int *a, int *b);
template void swap<uint64_t>(uint64_t *a, uint64_t *b);
template void swap<bmp::cpp_int>(bmp::cpp_int *a, bmp::cpp_int *b);

template void insertion_sort<int>(int input[], const size_t &len);
template void insertion_sort<uint64_t>(uint64_t input[], const size_t &len);
template void insertion_sort<bmp::cpp_int>(bmp::cpp_int input[],
                                           const size_t &len);

template bool compare_std<int>(const int &a, const int &b);
template bool compare_std<uint64_t>(const uint64_t &a, const uint64_t &b);
template bool compare_std<bmp::cpp_int>(const bmp::cpp_int &a,
                                        const bmp::cpp_int &b);

template void qsort_cpp<int>(int arr[], const size_t &n, const size_t &thresh);
template void qsort_cpp<uint64_t>(uint64_t arr[], const size_t &n,
                                  const size_t &thresh);
template void qsort_cpp<bmp::cpp_int>(bmp::cpp_int arr[], const size_t &len,
                                      const size_t &thresh);

template struct stack_node<int>;
template struct stack_node<uint64_t>;
template struct stack_node<bmp::cpp_int>;
