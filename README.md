# Quicksort Tuning

Most standard library sorting implementations use some version of quicksort,
which is usually combined with insertion sort for small lists to avoid some of
the overhead of repeated partitioning and recursive calls. This improves
performance in practice, but the threshold size of when to switch from quicksort
to insertion sort is hard-coded. Optimizing the threshold value could offer
better performance. This project evaluates quicksort on a variety of
architectures with varying threshold values, and tunes this value with machine
learning.

## Setup

Compiling QST and setting up the required python environment on ARCC.

1. Load the required modules

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
   $ cmake .. -DCMAKE_BUILD_TYPE=RELEASE
   $ make

   $ ./QST --version
   1.3.2
       Compiled with: GNU 11.1.1
       Type: RELEASE
       USE_BOOST_CPP_INT: False
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
   - Repeated single number

   Use `src/data.py` to specify the threshold and generate the data with:

   ```
   $ python src/data.py generate -f -t 1_000_000,50_000_000
   ```

   This will create a `data/` directory with a subfolder for each category of
   data.

   By default, the resulting data will be compressed. This has no impact other
   than to speedup the process moving data to and from ARCC. You can manually
   inspect the data using `zcat`. `QST` will automatically try to decompress
   files with the `.gz` extension. If the input does not have this extension it
   is assumed to be plain text.

2. Create the `slurm.d/` directory.

   The `slurm.d/` contains several files each detailing the parameters for the
   slurm batch array jobs. Each file cannot exceed 5000 lines otherwise slurm
   will fail to submit the job, so this script auto truncates each file to 4,500
   lines by default, then creates another file with the remaining jobs.

   The following command generates `slurm.d/` which specifies the sorting
   methods (`qsort_c` and `std::sort`), threshold values (1 - 20), number of
   times to run the same input data (20) and, input data (`data/`) to test.

   ```
   $ python src/job.py -s ./slurm.d/ -m qsort_c,std -t 1,20 -r 20 data/
   ```

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
   CPU architectures. More info
   [here](https://arccwiki.atlassian.net/wiki/spaces/DOCUMENTAT/pages/82247690/Hardware+-+Teton).

5. Post-processing and Evaluation.

   Once all the slurm jobs are complete, you can use the `evaluate.ipynb`
   jupyter notebook to view the results.

   ```
   $ jupyter notebook evaluate.ipynb
   ```

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
- [x] Design an evaluation method:
- [x] Support arbitrary precision integers (`boost::multiprecision::cpp_int`)
  1. Generate large arrays of varying distributions.
     - Ascending
     - Descending
     - Random
     - Repeated single number
  2. Feed each array into each method, and time execution.
     - Glibc qsort C version
     - std::sort (varies based on platform) (baseline)
  3. Tweak the threshold value from 0 - N (20?) and run qsort again.
- [ ] Graphs and evaluation.
  - [x] Implement aforementioned evaluation method.
  - [x] Generate data.
  - [x] Build evaluation tools.
  - [x] Increase the number of inputs and run more tests.
  - [ ] Investigate introsort, more specifically implementations of `std::sort`.

## Sources

- [General Iterative Quicksort](https://www.geeksforgeeks.org/iterative-quick-sort/)
- [Glibc Qsort](https://github.com/lattera/glibc/blob/master/stdlib/qsort.c)
- [Quicksort Overview](https://www.youtube.com/watch?v=7h1s2SojIRw)
- [The Analysis of Quicksort Programs](https://link.springer.com/content/pdf/10.1007/BF00289467.pdf)
