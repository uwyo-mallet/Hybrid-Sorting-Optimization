# Quicksort Tuning

Most standard library sorting implementations use some version of quicksort,
which is usually combined with insertion sort for small lists to avoid some of
the overhead of repeated partitioning and recursive calls. This improves
performance in practice, but the threshold size of when to switch from quicksort
to insertion sort is hard-coded. Optimizing the threshold value could offer
better performance. This project evaluates quicksort on a variety of
architectures with varying threshold values, and tunes this value with machine learning.

## Road Map

Very work in progress...

- [x] Implement iterative insertion sort.
- [x] Implement vanilla, recursive quicksort.
- [x] Implement glibc [qsort](https://github.com/lattera/glibc/blob/master/stdlib/qsort.c).
  - Only **some** standard libraries use quicksort in conjunction with insertion
    sort. One of which is glibc's qsort. To ensure consistency between tests on
    varying systems, architectures, and compilers, this _specific implementation_
    is included.
- [ ] Improve the glibc version for C++ with templates.
  - While the glibc qsort algorithm is very well performing, the code is very
    old and uses many unsafe memory constructs, such as void pointers, in the
    name of portability. Updating this code to use C++ templates is preferable.
- [ ] Design an evaluation method:
  1. Generate large arrays of varying distributions.
     - Completely unsorted
     - Partially sorted
     - Completely sorted
  2. Feed each array into each method, and time execution.
     - Insertion sort
     - Glibc qsort C version
     - Glibc qsort C++ version
     - std::sort (varies based on platform)
  3. Tweak the threshold value from 0 - 100 and time again.
  4. Graphs and evaluation.

## Sources

- [Glibc Qsort](https://github.com/lattera/glibc/blob/master/stdlib/qsort.c)
- [General Iterative Quicksort](https://www.geeksforgeeks.org/iterative-quick-sort/)
- [The Analysis of Quicksort Programs](https://link.springer.com/content/pdf/10.1007/BF00289467.pdf)
