#pragma once
#ifndef SORT_HPP_
#define SORT_HPP_

#include <cstddef>
#include <cstdint>

typedef int (*compar_d_fn_t)(const void *, const void *);

template <typename T>
bool is_sorted(T input[], const size_t &len);

template <typename T>
void insertion_sort(T input[], const size_t &len);

template <typename T>
void qsort_recursive(T input[], const size_t &len, const size_t &thresh);

template <typename T>
void qsort_c(T input[], const size_t &len, const size_t &thresh);

template <typename T>
int compare(const T *a, const T *b);

template <typename T>
bool compare_std(const T &a, const T &b);

extern "C" void insertion_sort_asm(uint64_t input[], const uint64_t size);

#endif /* SORT_HPP_ */
