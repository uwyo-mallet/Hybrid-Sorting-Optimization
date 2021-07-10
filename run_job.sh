#!/bin/bash

# Determine the correct number of jobs to submit, create a results directory,
# then submit all the jobs as an sbatch job array.

# DO NOT EDIT job.sbatch!
# Instead, edit the following variables, then run with ./run_job.sh

CWD="/project/mallet/jarulsam/quicksort-tuning"           # Project directory
INPUT="${CWD}/slurm.dat"                                  # File with all commands to run
RESULTS_DIR="${CWD}/results/$(date +"%Y-%m-%d_%H-%M-%S")" # Place to store all the results
NUM_REPEATS=10                                            # Number of times to repeat the same list of commands

echoerr() { printf "%s\n" "$*" >&2; }

if ! [ -f "$INPUT" ]; then
  echoerr "Cannot open $INPUT"
  exit 1
fi

NUM_JOBS="$(wc -l <$INPUT)"

mkdir -p "$RESULTS_DIR"
cd "$RESULTS_DIR" || exit 1

echo "NUMBER OF JOBS: $NUM_JOBS"

for ((i = 0 ; i <= $NUM_REPEATS ; i++)); do
  sbatch --array "0-$NUM_JOBS" --output="${RESULTS_DIR}/output.%A_%a.out" "${CWD}/job.sbatch" "$INPUT"
  sleep 1
done
