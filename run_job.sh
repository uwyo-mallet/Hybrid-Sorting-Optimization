#!/bin/bash

echoerr() { printf "%s\n" "$*" >&2; }

CWD="/project/mallet/jarulsam/quicksort-tuning"
INPUT="${CWD}/slurm.dat"
RESULTS_DIR="/project/mallet/jarulsam/results/$(date +"%Y-%m-%d_%H-%M-%S")"

if ! [ -f "$INPUT" ]; then
  echoerr "Cannot open $INPUT"
  exit 1
fi

NUM_JOBS="$(wc -l <$INPUT)"
mkdir -p "$RESULTS_DIR"

cd "$RESULTS_DIR" || exit 1

sbatch --array "0-$NUM_JOBS" --output="${RESULTS_DIR}/output.%A_%a.out" --error="${RESULTS_DIR}/error.%A_%a.err" "${CWD}/job.sbatch"
