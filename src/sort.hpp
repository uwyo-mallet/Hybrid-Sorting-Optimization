#pragma once
#ifndef _SORT_HPP
#define _SORT_HPP

#include <cstddef>

/* Discontinue quicksort when the partition gets below this size */
#define MAX_QSORT_THRESH 4

template <typename T>
bool is_sorted(T input[], const size_t &len);

template <typename T>
void quick_sort(T input[], const size_t &len);

template <typename T>
void insertion_sort(T input[], const size_t &len);

#endif /* _SORT_HPP */
