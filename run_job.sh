#!/bin/bash

# Determine the correct number of jobs to submit, create a results directory,
# then submit all the jobs as an sbatch job array.

# DO NOT EDIT job.sbatch!
# Instead, edit the following variables, then run with ./run_job.sh
CWD="/project/mallet/jarulsam/quicksort-tuning"           # Project directory.
EXE="${CWD}/build/QST"                                    # QST executable.
INPUT_DIR="${CWD}/slurm.d/"                               # Dir with all commands to run (slurm.d/).
RESULTS_DIR="${CWD}/results/$(date +"%Y-%m-%d_%H-%M-%S")" # Place to store all the results.
JOB_DETAILS="${RESULTS_DIR}/job_details.txt"              # File to save all the details of this job.
NUM_RUNS=2                                                # Number of times to run the same data set.

echoerr() { printf "%s\n" "$*" >&2; }

if ! [ -d "$INPUT_DIR" ]; then
  echoerr "Cannot open $INPUT_DIR"
  exit 1
fi

mkdir -p "$RESULTS_DIR"

# Write system details to results
src/info.py "${RESULTS_DIR}"

cd "$RESULTS_DIR" || exit 1

NUM_JOBS_PER_RUN=$(cat ${INPUT_DIR}/*.dat | wc -l)
echo "Number of jobs per run: $NUM_JOBS_PER_RUN"

total_num_jobs=0
for ((i = 0; i < NUM_RUNS; i++)); do
  printf "\n--------------------\n"

  for f in "${INPUT_DIR}"/*.dat; do
    num_lines=$(cat "$f" | wc -l)
    printf "${num_lines}: "
    sbatch --array "0-${num_lines}" "${CWD}/job.sbatch" "$f"
    ((total_num_jobs += num_lines))

    sleep 1
  done

done

printf "\nTotal number of jobs: ${total_num_jobs}\n"

# Write to job details
{
  echo "RESULTS_DIR: $RESULTS_DIR"
  echo "NUM_RUNS: $NUM_RUNS"
  echo "NUM_JOBS_PER_RUN: $NUM_JOBS_PER_RUN"
  echo "TOTAL_NUM_JOBS: $total_num_jobs"
  echo "QST VERSION: $(${EXE} --version)"
} >>"$JOB_DETAILS"

# Ensure the slurm.dat file is preserved
cp -r "$INPUT_DIR" "${RESULTS_DIR}/."
