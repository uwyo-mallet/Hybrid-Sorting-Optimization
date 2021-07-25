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
NUM_REPEATS=15                                            # Number of times to repeat the same list of commands.
MAX_BATCH=4500                                            # The maximum number of batch jobs supported by slurm. (~5000 for UW)

echoerr() { printf "%s\n" "$*" >&2; }

if ! [ -f "$INPUT" ]; then
  echoerr "Cannot open $INPUT"
  exit 1
fi

NUM_JOBS_PER_RUN="$(wc -l <$INPUT)"

mkdir -p "$RESULTS_DIR"
cd "$RESULTS_DIR" || exit 1

echo "Number of jobs per run: $NUM_JOBS_PER_RUN"

total_num_jobs=0
for ((i = 0; i < NUM_REPEATS; i++)); do
  num_remaining="$NUM_JOBS_PER_RUN"
  current_batch_min=0
  current_batch_max=0

  if [ "$num_remaining" -gt "$MAX_BATCH" ]; then
    current_batch_max="$MAX_BATCH"
    ((num_remaining -= "$MAX_BATCH"))
  else
    current_batch_min=0
    current_batch_max="$num_remaining"
  fi

  printf "\n--------------------\n"
  while [ "$current_batch_min" -ne "$current_batch_max" ]; do
    printf "${current_batch_min}-${current_batch_max}: "
    out=$(sbatch --array "${current_batch_min}-${current_batch_max}" "${CWD}/job.sbatch" "$INPUT")
    printf "${out}\n"

    # Keep track of the total number of jobs actually submitted
    ((total_num_jobs += (current_batch_max - current_batch_min)))

    # Adjust the indexes for the next batch within this run.
    current_batch_min="$current_batch_max"
    if [ "$num_remaining" -gt "$MAX_BATCH" ]; then
      ((current_batch_max += MAX_BATCH))
      ((num_remaining -= MAX_BATCH))
    else
      ((current_batch_max += num_remaining))
      num_remaining=0
    fi

    sleep 1
  done

  sleep 1
done

echo "Total number of jobs: ${total_num_jobs}"

# Write to job details
{
  echo "RESULTS_DIR: $RESULTS_DIR"
  echo "NUM_REPEATS: $NUM_REPEATS"
  echo "NUM_JOBS_PER_RUN: $NUM_JOBS_PER_RUN"
  echo "TOTAL_NUM_JOBS: $total_num_jobs"
  echo "QST VERSION: $(${EXE} --version)"
} >>"$JOB_DETAILS"

# Write system details to results
src/plat.py "./${RESULTS_DIR}/"

# Ensure the slurm.dat file is preserved
cp "$INPUT" "${RESULTS_DIR}/."
