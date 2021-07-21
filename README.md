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

Instructions for compiling QST and setting up the required python environment
on the cluster.

1. Load the required modules
   ```
   module load gcc
   module load swset
   module load boost/1.67.0
   module load cmake/3.16.5
   module load miniconda3
   ```
2. Compile QST

   ```
   mkdir build
   cd build
   cmake .. -DCMAKE_BUILD_TYPE=RELEASE
   make

   $ ./QST -V
   1.0.4
   Compiled with: GNU 7.3.0
   ```

3. Setup the python environment

   ```
   conda create -n qst_py python=3.8
   conda activate qst_py

   pip install -r requirements.txt
   ```

## Running Tests

Assuming that everything is compiled and the miniconda environment is active,
this procedure is used to generate and run the tests.

1. Generating the input data.

   Generate a large amount of random data of the following categories:

   - Ascending
   - Descending
   - Entirely random
   - Repeated single number

   Edit `src/data.py` to specify the threshold and generate the data with:

   ```
   python src/data.py generate
   ```

   This will create a `data/` directory with a subfolder for each category of data.

   > This will take a considerable amount of time for larger input sizes.

   By default, the resulting data will be compressed. This has no impact other
   than to speedup the process moving data to and from the cluster.

2. Create the `slurm.dat` file.

   The `slurm.dat` contains all the parameters for the slurm batch array jobs.
   This file cannot exceed 5000 lines or slurm will fail to initialize the batch
   array job.

   The following command generates a `slurm.dat` file which specifies the
   sorting methods (`qsort_c` and `std::sort`), threshold values (1 - 20), and
   input data (`data/`) to test.

   ```
   python src/job.py -s slurm.dat -m qsort_c,std -t 1,20 data/
   ```

3. Tune job parameters and submit the jobs.

   Once the `slurm.dat` file is created, we are ready to dispatch all the jobs.
   This is handled by the `run_job.sh` script. This script will create a
   `results` directory and store all the outputs in a timestamped directory.

   Edit the variables at the top of `run_job.sh` to fit your environment.

   Then, start the jobs by running the script:

   ```
   ./run_job.sh
   ```

   > Remember, if `slurm.dat` exceeds 5000 lines the slurm job will fail.

4. Post-processing and Evaluation.

   Once all the slurm jobs are complete, you can use the `evaluate.ipynb`
   jupyter notebook to view the results.

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
  - [ ] Increase the number of inputs and run more tests.
  - [ ] Investigate introsort, more specifically implementations of `std::sort`.

## Sources

- [General Iterative Quicksort](https://www.geeksforgeeks.org/iterative-quick-sort/)
- [Glibc Qsort](https://github.com/lattera/glibc/blob/master/stdlib/qsort.c)
- [Quicksort Overview](https://www.youtube.com/watch?v=7h1s2SojIRw)
- [The Analysis of Quicksort Programs](https://link.springer.com/content/pdf/10.1007/BF00289467.pdf)
