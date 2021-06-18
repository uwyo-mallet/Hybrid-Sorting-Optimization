#pragma once
#ifndef SORT_HPP_
#define SORT_HPP_

#include <cstddef>

typedef int (*compar_d_fn_t) (const void*, const void*);

template <typename T>
bool is_sorted(T input[], const size_t &len);

template <typename T>
size_t vanilla_quicksort(T input[], const size_t &len);

template <typename T>
void insertion_sort(T input[], const size_t &len);

void optimized_quicksort(void* const pbase, size_t total_elems, size_t size, compar_d_fn_t cmp);

#endif /* _SORT_HPP */
