#!/bin/bash

# Determine the correct number of jobs to submit, create a results directory,
# then submit all the jobs as an sbatch job array.

# DO NOT EDIT job.sbatch!
# Instead, edit the following variables, then run with ./run_job.sh
CWD="/project/mallet/jarulsam/quicksort-tuning"           # Project directory.
EXE="${CWD}/build/QST"                                    # QST executable.
INPUT="${CWD}/slurm.dat"                                  # File with all commands to run (slurm.dat).
RESULTS_DIR="${CWD}/results/$(date +"%Y-%m-%d_%H-%M-%S")" # Place to store all the results.
JOB_DETAILS="${RESULTS_DIR}/job_details.txt"              # File to save all the details of this job.
NUM_REPEATS=10                                            # Number of times to repeat the same list of commands.

echoerr() { printf "%s\n" "$*" >&2; }

if ! [ -f "$INPUT" ]; then
  echoerr "Cannot open $INPUT"
  exit 1
fi

num_jobs="$(wc -l <$INPUT)"

mkdir -p "$RESULTS_DIR"
cd "$RESULTS_DIR" || exit 1

echo "Number of jobs per run: $num_jobs"

total_num_jobs=0
for ((i = 0; i < NUM_REPEATS; i++)); do
  sbatch --array "0-${num_jobs}" --output="${RESULTS_DIR}/output.%A_%a.out" "${CWD}/job.sbatch" "$INPUT"
  ((total_num_jobs += num_jobs))
  sleep 1
done

echo "Total number of jobs: ${total_num_jobs}"

# Write to job details
{
  echo "RESULTS_DIR: $RESULTS_DIR"
  echo "NUM_REPEATS: $NUM_REPEATS"
  echo "NUM_JOBS_PER_RUN: $num_jobs"
  echo "TOTAL_NUM_JOBS: $total_num_jobs"
  echo "QST VERSION: $(${EXE} --version)"
} >>"$JOB_DETAILS"

# Ensure the slurm.dat file is preserved
cp "$INPUT" "${RESULTS_DIR}/."
