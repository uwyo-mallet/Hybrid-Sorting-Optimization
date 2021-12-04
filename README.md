# Quicksort Tuning

Most standard library sorting implementations use some version of quicksort,
which is usually combined with insertion sort for small lists to avoid some of
the overhead of repeated partitioning and recursive calls. This improves
performance in practice, but the threshold size of when to switch from quicksort
to insertion sort is hard-coded. Optimizing the threshold value could offer
better performance. This project evaluates various implementations of quicksort
on a variety of architectures with varying threshold values, and tunes this value
with machine learning.

## Setup

Compiling QST and setting up the required python environment on ARCC.

1. Load the required modules

   This project requires both a C and C++ compiler, each supporting version 11
   of their respective standards.

   ```
   $ module load gcc
   $ module load swset
   $ module load boost/1.72.0
   $ module load cmake/3.16.5
   $ module load miniconda3
   ```

2. Compile QST

   ```
   $ mkdir build
   $ cd build

   # Optionally enable boost CPP int with: -DUSE_BOOST_CPP_INT=ON
   # Optionally disable assembly sorting methods with: -DDISABLE_ASM=ON
   $ cmake .. -DCMAKE_BUILD_TYPE=RELEASE
   $ make

   $ ./QST --version
   1.6.0
   	C COMPILER : GNU 11.2.1
   	CXX COMPILER : GNU 11.2.1
   	Type: RELEASE
   	BOOST CPP INT: [-]
   	ASM Methods: [+]
   ```

3. Setup the python environment

   ```
   $ conda create -n qst_py python=3.8
   $ conda activate qst_py

   $ pip install -r requirements.txt
   ```

## Running Tests

Assuming that everything is compiled and the miniconda environment is active,
this procedure is used to generate and run the tests.

1. Generating the input data.

   Generate a large amount of input data of the following categories:

   - Ascending
   - Descending
   - Entirely random
   - Repeated single number (42)

   Use `src/data.py` to specify the threshold and generate the data with:

   ```
   $ python src/data.py generate -f -t 1_000_000,50_000_000
   ```

   This will create a `data/` directory with a subfolder for each category of
   data. All of the data will be compatible with `int64_t`.

   > Beware: `QST` currently only supports unsigned integers.

   By default, the resulting data will be compressed. This has no impact other
   than to speedup the process moving data to and from various machines.
   You can manually inspect the data using `zcat`. `QST` will automatically
   try to decompress files with the `.gz` extension. If the input does not have
   this extension it is assumed to be plain text. All initialization
   (including IO) is not included in the runtime output.

2. Create the `slurm.d/` directory.

   The `slurm.d/` directory contains several files each detailing the parameters
   for the slurm batch array jobs. Each file cannot exceed 5,000 lines
   (this is specific to UW) otherwise slurm will fail to submit the job
   ([info](https://slurm.schedmd.com/job_array.html)), so this script auto
   truncates each file to 4,500 lines by default, then creates another file with
   the remaining jobs.

   For example, the following command generates `slurm.d/` which specifies the
   sorting methods (`qsort_c` and `std::sort`), threshold values (1 - 20),
   number of times to run the same input data (20) and, input data (`data/`) to
   test.

   ```
   $ python src/job.py --slurm ./slurm.d/ -m qsort_c,std -t 1,20 -r 20 data/
   ```

   > Note: When running subsequent jobs, even if the input data and all
   > parameters remain the same, **always** regenerate `slurm.d`. Otherwise,
   > runtime specific information and timestamps will not be accurate.

3. Submit the jobs.

   Once the `slurm.d/` directory is created, we are ready to dispatch all the
   jobs. This is handled by the `run_job.sh` script. This script will create a
   results directory and store all the outputs in a timestamped directory.

   Edit the variables at the top of `run_job.sh` to fit your environment.

   Then, start the jobs by running the `run_job.sh` script, substituting
   `teton` with whatever partition you would like to run on:

   ```
   $ ./run_job.sh teton
   ```

4. Optionally, repeat all the jobs on several different partitions with:

   ```
   $ ./all_partitions.sh
   ```

   This will dispatch jobs across many partitions simultaneously, creating a
   separate results directory for each, suffixed by `_partition_name`.

   List of partitions:

   - teton
   - teton-cascade
   - teton-hugemem
   - teton-massmem
   - teton-knl
   - moran

   These particular partitions were chosen because they all have differing
   CPU architectures. More info is available
   [here](https://arccwiki.atlassian.net/wiki/spaces/DOCUMENTAT/pages/82247690/Hardware+-+Teton).

5. Post-processing and Evaluation.

   Once all the slurm jobs are complete, you can use the `evaluator` python
   module to view the results. Ensure you copy all the data back to your local
   machine.

   For the sake of compatibility with older Python versions (mostly for HPCs),
   dependencies for the `evaluator` are kept separate from the core testing
   module. Install them with:

   ```
   $ pip install -r eval_requirements.txt
   ```

   Then run the module with:

   ```
   $ python -m evaluator
   ```

   Open a web browser to [http://localhost:8050](http://localhost:8050) and
   you should be able to pick various parameters to visualize your results.

### QST Supported Sorting Methods

Supported sorting methods for QST. Essentially the possible options for
`--method/-m`:

#### No Threshold

- `insertion_sort` - C++ iterative insertion sort using templates and a
  parameterized compare function.
- `insertion_sort_c` - Pure C version of `insertion_sort` using `void` pointers.
- `insertion_sort_c_swp` - Same as `insertion_sort_c` but uses a parameterized
  swap function instead of preprocessor based bitwise swap (from `qsort` in
  glibc).
- `insertion_sort_asm` - Pure x86 assembly version of insertion sort. Only
  supports `uint64_t`.
- `qsort_sanity` - Literally just calls `qsort`. May be different methods
  internally depending on the compiler implementation.
- `std::sort` - Literally just calls `std::sort`. May be different methods
  internally depending on the compiler implementation.

#### Threshold

- `qsort_asm` - Pure x86 assembly version of glibc quicksort. Only supports
  `uint64_t`. (_Currently broken and runs very slowly._)
- `qsort_c` - Pure C version of quicksort ripped directly from glibc for the
  sake of consistency between compiler implementations. Also makes the threshold
  a parameter instead of a compile time constant.
- `qsort_c_sep_ins` - Same as `qsort_c` but uses a different insertion sort
  routine for subarrays.
- `qsort_c_swp` - Same as `qsort_c` but uses a parameterized swap function,
  similar to `insertion_sort_c_swp`.
- `qsort_cpp` - C++ version of quicksort. Contains all the same optimizations
  as quicksort from glibc, but uses modern language features such as
  templates.
- `qsort_cpp_no_comp` - Same as `qsort_cpp` but has a hardcoded comparison
  function for integers.

More detailed information about the methods and the specific optimizations can
be found in the generated doxygen docs, and directly in the source code.

## Docs

All source code docs are auto-built with Doxygen. Install
[doxygen](https://www.doxygen.nl/index.html) (> v1.9) and build QST. The
docs should be auto built as well, as long as all dependencies are met.
Open `docs/doxygen/html/index.html` to browse source code documentation.

## Road Map

A broad overview of what has to be done.
Very work in progress...

- [x] Implement iterative insertion sort.
- [x] Implement vanilla, recursive quicksort.
- [x] Implement glibc [`qsort`](https://github.com/lattera/glibc/blob/master/stdlib/qsort.c).
  - Only **some** standard libraries use quicksort in conjunction with insertion
    sort. One of which is glibc's qsort. To ensure consistency between tests on
    varying systems, architectures, and compilers, this _specific implementation_
    is included.
- [x] Implement my own rendition of qsort using modern C++ features.
- [x] Support arbitrary precision integers (`boost::multiprecision::cpp_int`),
      although for now just use `uint64_t` to avoid the extra overhead.
- [x] Design an evaluation method:
  1. Generate large arrays of varying distributions.
     - Ascending
     - Descending
     - Random
     - Repeated single number
  2. Feed each array into each method, and time execution.
  3. Tweak the threshold value from 0 - N (20?) and run qsort again.
- [x] Implement aforementioned evaluation method.
- [x] Generate data scripts.
- [x] Build evaluation tools.
- [x] Increase the number of inputs and run more tests.
- [x] Run tests across many partitions of ARCC.
- [x] Add unmodified `qsort` from glibc (`qsort_sanity`) as a sanity check.
- [x] Run some small tests to discern the impact of comparator functions.
- [x] Run some small tests to measure the performance gain of compiler
      optimization (in-lining of the aforementioned comparator functions).
- [x] Decouple C code entirely from C++ code. See
      [this](https://gitlab.com/uwyo-mallet/quicksort-tuning/-/commit/27b37eeae9cb1d4912f33d6847035f08a27288db)
      commit for more info.
- [x] Run tests on smaller inputs.
- [x] Better plotting scripts (Dash + Plotly)
- [x] Evaluate how swapping optimization affects runtime.
- [x] Investigate other hybridized sort value thresholds and why:

  - Python uses TimSort, an adaptation of mergesort. If array is less than 32
    elements, uses binary insertion sort.

        If N < 64, minrun is N.  IOW, binary insertion sort is used for the whole
        array then; it's hard to beat that given the overheads of trying something
        fancier (see note BINSORT).

        When N is a power of 2, testing on random data showed that minrun values of
        16, 32, 64 and 128 worked about equally well.  At 256 the data-movement cost
        in binary insertion sort clearly hurt, and at 8 the increase in the number
        of function calls clearly hurt.  Picking *some* power of 2 is important
        here, so that the merges end up perfectly balanced (see next section).  We
        pick 32 as a good value in the sweet range; picking a value at the low end
        allows the adaptive gimmicks more opportunity to exploit shorter natural
        runs. - Tim Peters, 2002.

  - NodeJS and Spider-monkey use C++ `std::sort`.
  - WebKit uses `qsort`.
  - V8 used to use `std::sort` and `qsort` / insertion sort, but now uses
    TimSort.
  - Glibc (libstdc++) `std::sort` uses introsort. Quicksort till depth > 3, then
    heapsort.
  - Clang (llvm libc++) `std::sort` hardcodes sorts for arrays smaller than 5, then
    uses heapsort.
  - Glibc (gcc) qsort uses threshold 4. "_Discontinue quicksort algorithm when
    partition gets below this size [4].
    This particular magic number was chosen to work best on a Sun 4/260._" -
    Douglas C. Schmidt, 1995.
  - musl `qsort` uses smoothsort, an adaptation of heapsort. No threshold.
  - Clang (llvm libc++) `qsort` uses a simple, standard, _recursive_ implementation using
    the Hoare partition scheme (needs further investigation).

- [ ] Investigate introsort (worst case O(nlog(n)), more specifically
      implementations of `std::sort`.
- [ ] Potentially analyze cache hits and misses, but generally quicksort does
      pretty well here, so this may not be necessary.
- [ ] Investigate branch prediction error performance impact. It's possible
      branch prediction errors are common, but performance may still be
      generally good. In which case, it is a waste to analyze this since it is
      _very_ challenging to effectively measure.
- [ ] Investigate block quicksort implementation in this
      [paper](https://arxiv.org/abs/1604.06697).
- [ ] Determine why `qsort_c` is running so slow. ~~Looks like a missing brace
      in the insertion sort implementation.~~ I have absolutely no idea why
      this runs so slow.
- [x] Try modifying `qsort_c` to use my insertion sort routine.

### Side Goals - Mostly For Experience and Fun

- [x] Implement insertion sort in x86 assembly. Can we beat an optimizing
      compiler?
- [ ] Implement quicksort in x86 assembly (this is currently broken and very
      slow) This remains near the lowest priority item for this entire project.
- [x] Increase threshold significantly thousands to tens of thousands.
      Interval of like 500? This doesn't make sense practically, as most inputs
      for standard sorting algorithms are under 100 million inputs, and
      using insertion sort for everything is not particularly efficient.
- [ ] Test the runtime of worst-case inputs;
      [paper](https://arxiv.org/abs/1604.06697).

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
