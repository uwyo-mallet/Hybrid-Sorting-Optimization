#ifndef UTILS_HPP
#define UTILS_HPP

#include <iostream>
#include <string>
#include <vector>

/**
 * Print a vector to stdout
 *
 * @param arr: Vector to print
 */
template <typename T>
void print(const std::vector<T> &arr)
{
  for (const T &i : arr)
  {
    std::cout << i << " ";
  }
  std::cout << std::endl;
}

/**
 * Print an array to stdout
 *
 * @param arr: Array to print
 * @param n: Length of array
 */
template <typename T>
void print(const T *arr, const size_t &n)
{
  for (size_t i = 0; i < n; i++)
  {
    std::cout << arr[i] << " ";
  }
  std::cout << std::endl;
}

/**
 * Check if two arrays are equal.
 *
 * @param a: First array
 * @param b: Second array
 * @param n: Number of elements in each array.
 */
template <typename T>
bool array_equal(T *a, T *b, size_t n)
{
  for (size_t i = 0; i < n; i++)
  {
    if (a[i] != b[i])
    {
      return false;
    }
  }

  return true;
}

std::string &trim(std::string &s);
std::vector<std::string> parse_comma_sep_args(const std::string &args);

#endif  // UTILS_HPP
