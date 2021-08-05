#!/bin/bash

# Determine the correct number of jobs to submit, create a results directory,
# then submit all the jobs as an sbatch job array.

# DO NOT EDIT job.sbatch!
# Instead, edit the following variables, then run with ./run_job.sh
CWD=$(realpath .)                                         # Project directory.
INPUT_DIR="${CWD}/slurm.d/"                               # Dir with all commands to run (slurm.d/).
RESULTS_DIR="${CWD}/results/$(date +"%Y-%m-%d_%H-%M-%S")" # Place to store all the results.

echoerr() { printf "%s\n" "$*" >&2; }

partitions=("teton" "teton-cascade" "teton-hugemem" "teton-massmem" "teton-knl" "moran")

# Ensure the inputs exist
if ! [ -d "$INPUT_DIR" ]; then
  echoerr "Cannot open $INPUT_DIR"
  exit 1
fi

# Ensure a valid partitition is selected
if [[ ! " ${partitions[@]} " =~ " ${1} " ]]; then
  echoerr "Invalid partition selection"
  exit 2
fi

# Ensure slurm.d/ has at least 1 input file
if ! test -n "$(
  shopt -s nullglob
  echo "${INPUT_DIR}"/*.dat
)"; then
  echoerr "${INPUT_DIR} doesn't contain at .dat files!"
  exit 3
fi

# Everything looks good, proceeed to actual job submission.
mkdir -p "$RESULTS_DIR"
cd "$RESULTS_DIR" || exit 1

total_num_jobs=0

files=("${INPUT_DIR}"/*.dat)
sorted_files=($(printf "%s\n" "${files[@]}" | sort -V))
pos=$((${#files[*]} - 1))
last="${files[$pos]}"

for f in "${sorted_files[@]}"; do
  num_lines=$(wc -l <"$f")
  printf "%s" "$(basename "$f") ${num_lines}: "
  sbatch --array "0-${num_lines}" --partition="$1" "${CWD}/job.sbatch" "$f"
  ((total_num_jobs += num_lines))

  # Sleep per batch of jobs. Otherwise, this causes slurm to fail MANY jobs.
  # Skip the sleep if it is the last, since we aren't submitting any more jobs.
  if [[ "$f" != "$last" ]]; then
    sleep 300
  fi
done

printf "\n%s\n" "Total number of jobs: ${total_num_jobs}"

# Copy job details generated by jobs.py
cp "${INPUT_DIR}/job_details.json" "${RESULTS_DIR}/."

# Ensure the slurm.d/ dir is preserved
cp -r "$INPUT_DIR" "${RESULTS_DIR}/."

# Save which partition we ran on
echo "$1" >"${RESULTS_DIR}/partition"
