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

echoerr() { printf "%s\n" "$*" >&2; }

if ! [ -d "$INPUT_DIR" ]; then
  echoerr "Cannot open $INPUT_DIR"
  exit 1
fi

mkdir -p "$RESULTS_DIR"

# Write system details to results
src/info.py "${RESULTS_DIR}"

cd "$RESULTS_DIR" || exit 1

total_num_jobs=0
for f in "${INPUT_DIR}"/*.dat; do
  num_lines=$(wc -l <"$f")
  printf "%s" "${num_lines}: "
  echo sbatch --array "0-${num_lines}" "${CWD}/job.sbatch" "$f"
  ((total_num_jobs += num_lines))

  sleep 1
done

printf "%s" "\nTotal number of jobs: ${total_num_jobs}\n"

# Write to job details
{
  echo "RESULTS_DIR: $RESULTS_DIR"
  echo "TOTAL_NUM_JOBS: $total_num_jobs"
  echo "QST VERSION: $(${EXE} --version)"
} >>"$JOB_DETAILS"

# Ensure the slurm.dat file is preserved
cp -r "$INPUT_DIR" "${RESULTS_DIR}/."
