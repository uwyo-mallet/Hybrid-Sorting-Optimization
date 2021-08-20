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
#include "sort.hpp"

template <typename T>
size_t time(const std::string& method, const size_t& threshold,
            std::vector<T>& vec)
{
  std::chrono::time_point<std::chrono::steady_clock> start_time;

  if (method == "insertion_sort")
  {
    start_time = std::chrono::steady_clock::now();
    insertion_sort(vec.data(), vec.size(), compare<T>);
  }
#ifdef ARCH_X86
  // insertion sort asm only works with uint64_t
  else if (method == "insertion_sort_asm")
  {
    start_time = std::chrono::steady_clock::now();
    insertion_sort_asm(vec.data(), vec.size());
  }
  else if (method == "qsort_asm")
  {
    start_time = std::chrono::steady_clock::now();
    qsort_asm(vec.data(), vec.size(), threshold);
  }
#endif  // ARCH_X86
  else if (method == "qsort_c")
  {
    start_time = std::chrono::steady_clock::now();
    // qsort_c(vec.data(), vec.size(), threshold);
    qsort_c(vec.data(), vec.size(), sizeof(T), (compar_d_fn_t)compare<T>,
            threshold);
  }
  else if (method == "qsort_cpp")
  {
    start_time = std::chrono::steady_clock::now();
    qsort_cpp(vec.data(), vec.size(), threshold, compare<T>);
  }
  else if (method == "qsort_sanity")
  {
    start_time = std::chrono::steady_clock::now();
    qsort(vec.data(), vec.size(), sizeof(T), (compar_d_fn_t)compare<T>);
  }
  else if (method == "std")
  {
    start_time = std::chrono::steady_clock::now();
#ifdef USE_BOOST_CPP_INT
    std::sort(vec.begin(), vec.end(),
              compare_std<boost::multiprecision::cpp_int>);
#else
    std::sort(vec.begin(), vec.end(), compare_std<uint64_t>);
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
  if (!is_sorted(vec.data(), vec.size()))
  {
    throw std::runtime_error("Not valid sort");
  }

  // return std::to_string((size_t)runtime.count());
  return (size_t)runtime.count();
}
#endif
