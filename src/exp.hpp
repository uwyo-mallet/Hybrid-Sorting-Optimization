#pragma once
#ifndef EXP_HPP_
#define EXP_HPP_

#include <algorithm>
#include <boost/multiprecision/cpp_int.hpp>
#include <chrono>
#include <stdexcept>
#include <string>
#include <vector>

#include "platform.hpp"
#include "sort.h"
#include "sort.hpp"

template <typename T>
size_t time(const std::string& method, const size_t& threshold,
            std::vector<T>& to_sort, const std::vector<T>& sorted)
{
  std::chrono::time_point<std::chrono::steady_clock> start_time;

  if (method == "insertion_sort")
  {
    start_time = std::chrono::steady_clock::now();
    insertion_sort(to_sort.data(), to_sort.size(), compare<T>);
  }
// These methods require x86 and only support uint64_t.
#ifdef ARCH_X86
  else if (method == "insertion_sort_asm")
  {
    start_time = std::chrono::steady_clock::now();
    insertion_sort_asm(to_sort.data(), to_sort.size());
  }
  else if (method == "qsort_asm")
  {
    start_time = std::chrono::steady_clock::now();
    qsort_asm(to_sort.data(), to_sort.size(), threshold);
  }
#endif  // ARCH_X86
  else if (method == "qsort_c")
  {
    start_time = std::chrono::steady_clock::now();
    // qsort_c(vec.data(), vec.size(), threshold);
    qsort_c(to_sort.data(), to_sort.size(), sizeof(T), compare<T>, threshold);
  }
  else if (method == "qsort_cpp")
  {
    start_time = std::chrono::steady_clock::now();
    qsort_cpp(to_sort.data(), to_sort.size(), threshold, compare<T>);
  }
  else if (method == "qsort_cpp_no_comp")
  {
    start_time = std::chrono::steady_clock::now();
    qsort_cpp_no_comp(to_sort.data(), to_sort.size(), threshold);
  }
  else if (method == "qsort_c_improved")
  {
    start_time = std::chrono::steady_clock::now();
    qsort_c_improved(to_sort.data(), to_sort.size(), sizeof(T), compare<T>,
                     threshold);
  }
  else if (method == "qsort_sanity")
  {
    start_time = std::chrono::steady_clock::now();
    qsort(to_sort.data(), to_sort.size(), sizeof(T), compare<T>);
  }
  else if (method == "std")
  {
    start_time = std::chrono::steady_clock::now();
#ifdef USE_BOOST_CPP_INT
    std::sort(to_sort.begin(), to_sort.end(),
              compare_std<boost::multiprecision::cpp_int>);
#else
    std::sort(to_sort.begin(), to_sort.end(), compare_std<uint64_t>);
#endif
  }
  else
  {
    throw std::invalid_argument("Invalid method selected");
  }

  std::chrono::time_point<std::chrono::steady_clock> end_time =
      std::chrono::steady_clock::now();

  auto runtime = std::chrono::duration_cast<std::chrono::microseconds>(
      end_time - start_time);

  // Ensure the data is actually sorted
  if (to_sort != sorted)
  {
    throw std::runtime_error("Post sort array was not properly sorted");
  }

  return (size_t)runtime.count();
}
#endif
