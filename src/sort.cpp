#include "sort.hpp"

#include <iostream>

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

template <typename T>
void swap(T *a, T *b)
{
  T tmp = *a;
  *a = *b;
  *b = tmp;
}

// Given 3 values, sort them in place (a < b < c), and return the median (b)
// value.
template <typename T>
T median_of_three(T *a, T *b, T *c)
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

  return *b;
}

template <typename T>
size_t partition(T input[], size_t low, size_t high)
{
  const size_t mid = (high - 1) / 2;
  std::cout << input[low] << std::endl;
  std::cout << input[mid] << std::endl;
  std::cout << input[high] << std::endl;

  const size_t pivot = median_of_three(&input[low], &input[mid], &input[high]);

  const T pivot_val = input[pivot];

  while (low < high)
  {
    while (input[low] <= pivot_val)
    {
      low++;
    }
    while (input[high] > pivot_val)
    {
      high--;
    }
    if (low < high)
    {
      swap(&input[low], &input[high]);
    }
  }
  swap(&input[pivot], &input[high]);

  return high;
}

/* Quicksort recursive call */
template <typename T>
void quick_sort(T input[], size_t low, size_t high)
{
  if (low >= high) return;

  size_t split_pos = partition(input, low, high);
  quick_sort(input, low, split_pos - 1);
  quick_sort(input, split_pos + 1, high);
}

template <typename T>
void quick_sort(T input[], const size_t &len)
{
  quick_sort(input, 0, len);
}

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

// Template forward decleration to fix linker issues
template bool is_sorted<int>(int input[], const size_t &len);
template bool is_sorted<float>(float input[], const size_t &len);

template void swap<int>(int *a, int *b);
template void swap<float>(float *a, float *b);

template int median_of_three<int>(int *a, int *b, int *c);
template float median_of_three<float>(float *a, float *b, float *c);

template size_t partition<int>(int input[], size_t low, size_t high);
template size_t partition<float>(float input[], size_t low, size_t high);

template void quick_sort<int>(int input[], size_t low, size_t high);
template void quick_sort<float>(float input[], size_t low, size_t high);

template void quick_sort<int>(int input[], const size_t &len);
template void quick_sort<float>(float input[], const size_t &len);

template void insertion_sort<int>(int input[], const size_t &len);
template void insertion_sort<float>(float input[], const size_t &len);
