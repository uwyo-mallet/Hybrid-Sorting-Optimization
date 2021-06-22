# Quicksort Tuning

Most standard library sorting implementations use some version of quicksort,
which is usually combined with insertion sort for small lists to avoid some of
the overhead of repeated partitioning and recursive calls. This improves
performance in practice, but the threshold size of when to switch from quicksort
to insertion sort is hard-coded. Optimizing the threshold value could offer
better performance. This project evaluates quicksort on a variety of
architectures with varying threshold values, and tunes this value with machine learning.

## Road Map

A broad overview of what has to be done.
Very work in progress...

- [x] Implement iterative insertion sort.
- [x] Implement vanilla, recursive quicksort.
- [x] Implement glibc [qsort](https://github.com/lattera/glibc/blob/master/stdlib/qsort.c).
  - Only **some** standard libraries use quicksort in conjunction with insertion
    sort. One of which is glibc's qsort. To ensure consistency between tests on
    varying systems, architectures, and compilers, this _specific implementation_
    is included.
- [X] Design an evaluation method:
  1. Generate large arrays of varying distributions.
     - Completely sorted
     - Completely unsorted
     - Partially sorted
  2. Feed each array into each method, and time execution.
     - Insertion sort (baseline)
     - Glibc qsort C version
     - std::sort (varies based on platform)
  3. Tweak the threshold value from 0 - 100 and run qsort again.
  4. Graphs and evaluation.


### TODO

A more specific list of some niche things that I need to finish.

- [ ] Add a CLI to CPP implementations where 3 things can be specified:
  - Input file with a bunch of numbers to sort.
  - Which sorting algorithm to use:
    - If quicksort, threshold to switch to insertion sort.
- [ ] Automate timing and data collection (Output to csv?).
- [ ] Collect the baseline data.

## Sources

- [General Iterative Quicksort](https://www.geeksforgeeks.org/iterative-quick-sort/)
- [Glibc Qsort](https://github.com/lattera/glibc/blob/master/stdlib/qsort.c)
- [Quicksort Overview](https://www.youtube.com/watch?v=7h1s2SojIRw)
- [The Analysis of Quicksort Programs](https://link.springer.com/content/pdf/10.1007/BF00289467.pdf)
