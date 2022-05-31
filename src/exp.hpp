#pragma once
#ifndef EXP_HPP_
#define EXP_HPP_

#include <algorithm>
#include <boost/timer/timer.hpp>
#include <stdexcept>
#include <string>

#include "platform.hpp"
#include "sort.h"
#include "sort.hpp"

/**
 * Time the runtime of a sort algorithm.
 *
 * @param method: Name of method to sort with.
 * @param threshold: Threshold if applicable, otherwise ignored.
 * @param to_sort: Data to sort.
 * @param n: Number of elements in array.
 * @see https://www.boost.org/doc/libs/1_67_0/libs/timer/doc/cpu_timers.html
 */
template <typename T>
boost::timer::cpu_times time(const std::string& method, const size_t& threshold,
                             T* to_sort, const size_t n)
{
  boost::timer::cpu_timer timer;

  // No Threshold --------------------------------------------------------------
  if (method == "insertion_sort")
  {
    timer.start();
    insertion_sort(to_sort, n, compare<T>);
  }
  else if (method == "insertion_sort_c")
  {
    timer.start();
    insertion_sort_c(to_sort, n, sizeof(T), compare<T>);
  }
  else if (method == "insertion_sort_c_swp")
  {
    timer.start();
    insertion_sort_c_swp(to_sort, n, sizeof(T), swap<T>, compare<T>);
  }
  else if (method == "qsort_sanity")
  {
    timer.start();
    qsort_sanity(to_sort, n, sizeof(T), compare<T>);
  }
  else if (method == "std::sort")
  {
    timer.start();
    std::sort(to_sort, to_sort + n, compare_std<T>);
  }
  else if (method == "msort_c")
  {
    timer.start();
    msort_c(to_sort, n, sizeof(T), compare<T>);
  }
#ifdef ARCH_X86
  /* These methods require x86 and only support uint64_t. */
  else if (method == "insertion_sort_asm")
  {
    timer.start();
    insertion_sort_asm(to_sort, n);
  }
  // Threshold -----------------------------------------------------------------
  else if (method == "qsort_asm")
  {
    timer.start();
    qsort_asm(to_sort, n, threshold);
  }
#endif  // ARCH_X86
  else if (method == "qsort_c")
  {
    timer.start();
    qsort_c(to_sort, n, sizeof(T), compare<T>, threshold);
  }
  else if (method == "qsort_c_sep_ins")
  {
    timer.start();
    qsort_c_sep_ins(to_sort, n, sizeof(T), compare<T>, threshold);
  }
  else if (method == "qsort_c_swp")
  {
    timer.start();
    qsort_c_swp(to_sort, n, sizeof(T), swap<T>, compare<T>, threshold);
  }
  else if (method == "qsort_cpp")
  {
    timer.start();
    qsort_cpp(to_sort, n, threshold, compare<T>);
  }
  else if (method == "qsort_cpp_no_comp")
  {
    timer.start();
    qsort_cpp_no_comp(to_sort, n, threshold);
  }
  else if (method == "qsort_vanilla")
  {
    timer.start();
    qsort_vanilla(to_sort, n, compare<T>);
  }
  else if (method == "fail")
  {
    timer.start();
  }
  else
  {
    throw std::invalid_argument("Invalid method selected");
  }
  timer.stop();

  return timer.elapsed();
}
#endif
