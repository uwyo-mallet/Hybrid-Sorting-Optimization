#pragma once
#ifndef EXP_HPP_
#define EXP_HPP_

#include <algorithm>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/timer/timer.hpp>
#include <stdexcept>
#include <string>
#include <vector>

#include "platform.hpp"
#include "sort.h"
#include "sort.hpp"

/**
 * Time the runtime of a sort algorithm.
 *
 * @param method: Name of method to sort with.
 * @param threshold: Threshold if applicable, otherwise ignored.
 * @param to_sort: Data to sort.
 * @param sorted: Sorted version of to_sort to use to validate sorting function.
 */
template <typename T>
boost::timer::cpu_times time(const std::string& method, const size_t& threshold,
                             std::vector<T>& to_sort,
                             const std::vector<T>& sorted)
{
  // std::chrono::time_point<std::chrono::steady_clock> start_time;

  boost::timer::cpu_timer timer;

  // No Threshold --------------------------------------------------------------
  if (method == "insertion_sort")
  {
    timer.start();
    insertion_sort(to_sort.data(), to_sort.size(), compare<T>);
  }
  else if (method == "insertion_sort_c")
  {
    timer.start();
    insertion_sort_c(to_sort.data(), to_sort.size(), sizeof(T), compare<T>);
  }
  else if (method == "insertion_sort_c_swp")
  {
    timer.start();
    insertion_sort_c_swp(to_sort.data(), to_sort.size(), sizeof(T), swap<T>,
                         compare<T>);
  }
  else if (method == "qsort_sanity")
  {
    timer.start();
    qsort(to_sort.data(), to_sort.size(), sizeof(T), compare<T>);
  }
  else if (method == "std")
  {
    timer.start();
#ifdef USE_BOOST_CPP_INT
    std::sort(to_sort.begin(), to_sort.end(),
              compare_std<boost::multiprecision::cpp_int>);
#else
    std::sort(to_sort.begin(), to_sort.end(), compare_std<uint64_t>);
#endif
  }
// These methods require x86 and only support uint64_t.
#ifdef ARCH_X86
  else if (method == "insertion_sort_asm")
  {
    timer.start();
    insertion_sort_asm(to_sort.data(), to_sort.size());
  }
  // Threshold -----------------------------------------------------------------
  else if (method == "qsort_asm")
  {
    timer.start();
    qsort_asm(to_sort.data(), to_sort.size(), threshold);
  }
#endif  // ARCH_X86
  else if (method == "qsort_c")
  {
    timer.start();
    qsort_c(to_sort.data(), to_sort.size(), sizeof(T), compare<T>, threshold);
  }
  else if (method == "qsort_c_sep_ins")
  {
    timer.start();
    qsort_c_sep_ins(to_sort.data(), to_sort.size(), sizeof(T), compare<T>,
                    threshold);
  }
  else if (method == "qsort_c_swp")
  {
    timer.start();
    qsort_c_swp(to_sort.data(), to_sort.size(), sizeof(T), swap<T>, compare<T>,
                threshold);
  }
  else if (method == "qsort_cpp")
  {
    timer.start();
    qsort_cpp(to_sort.data(), to_sort.size(), threshold, compare<T>);
  }
  else if (method == "qsort_cpp_no_comp")
  {
    timer.start();
    qsort_cpp_no_comp(to_sort.data(), to_sort.size(), threshold);
  }
  else
  {
    throw std::invalid_argument("Invalid method selected");
  }
  timer.stop();

  // Ensure the data is actually sorted
  if (to_sort != sorted)
  {
    throw std::runtime_error("Post sort array was not properly sorted");
  }

  return timer.elapsed();
}
#endif
