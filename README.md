# Hybrid Sorting Optimization

Most standard libraries include some sort of standard sorting implementation.
Usually, either Quicksort or Mergesort is used, as they generally perform very
well in most scenarios. However, combining these algorithms with some other
secondary algorithm, such as insertion sort, for small lists can offer a
sizeable performance gain as the repeated overhead of partitioning arrays or
recursive calls is avoided. The threshold at which to switch between these
algorithms is often hard-coded by some developer.

This project evaluates various candidate implementations for standard libraries
at many different threshold values. Specifically, `qsort()` from GNU's libc is
evaluated extensively. Along with this evaluation, this research has found a
new, faster sorting algorithm by intelligently combining Mergesort with sorting
networks and insertion sort.

## Setup

Refer to the instructions provided for each language in their corresponding
subdirectory within [`src/`](./src/).

## Methods

The following methods are evaluated:

- `qsort` (from the C standard library)
- `std::sort` (from the C++ standard library)
- Mergesort
- A basic insertion sort.
- A highly optimized insertion sort.
- Shell Sort
- Mergesort w/ basic insertion sort.
- Mergesort w/ optimized insertion sort.
- Mergesort w/ shell sort.
- Mergesort w/ sorting network.
- Mergesort w/ optimized insertion sort & sorting network.
- Quicksort w/ basic insertion sort.
- Quicksort w/ optimized insertion sort.

## Sources

- [A Killer Advisory for Quicksort](https://algs4.cs.princeton.edu/references/papers/mcilroy.pdf)
- [Block Quicksort](https://arxiv.org/abs/1604.06697)
- [CPython Listsort Explanation](https://github.com/python/cpython/blob/main/Objects/listsort.txt)
- [CPython Listsort Implementation](https://github.com/python/cpython/blob/main/Objects/listobject.c#L1058)
- [General Iterative Quicksort](https://www.geeksforgeeks.org/iterative-quick-sort/)
- [Quicksort Basic Overview](https://www.youtube.com/watch?v=7h1s2SojIRw)
- [Quicksort is Optimal](https://www.cs.princeton.edu/~rs/talks/QuicksortIsOptimal.pdf)
- [Sedgewick Quicksort](https://algs4.cs.princeton.edu/23quicksort/)
- [The Analysis of Quicksort Programs](https://link.springer.com/content/pdf/10.1007/BF00289467.pdf)
- [`qsort` (glibc)](https://sourceware.org/git/?p=glibc.git;a=blob;f=stdlib/qsort.c;h=23f2d283147073ac5bcb6e4bf2c9d6ea994d629c;hb=HEAD)
- [`qsort` (llvm C++)](https://github.com/llvm/llvm-project/blob/main/libc/src/stdlib/qsort.cpp)
- [`qsort` (musl)](https://git.musl-libc.org/cgit/musl/tree/src/stdlib/qsort.c)
- [`std::sort` (libc++)](https://github.com/llvm-mirror/libcxx/blob/a12cb9d211019d99b5875b6d8034617cbc24c2cc/include/algorithm#L3901)
- [`std::sort` (libstdc++)](https://github.com/gcc-mirror/gcc/blob/d9375e490072d1aae73a93949aa158fcd2a27018/libstdc%2B%2B-v3/include/bits/stl_algo.h#L1950)
