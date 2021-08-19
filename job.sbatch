#!/bin/bash

# DO NOT CALL OR EDIT THIS DIRECTLY.
# This file is only used by run_job.sh
# Contains the definition of a SINGLE job in a batch array. If you must edit
# this file, only edit the slurm parameters.

#SBATCH --job-name=Quicksort-Tuning
#SBATCH --account=mallet
#SBATCH --time=12:00:00
#SBATCH --nodes=1
#SBATCH --mem=4G
#SBATCH --signal=TERM@300
#SBATCH --profile=all
#SBATCH --output=stdout_%A.out

INPUT_DATA="$1"
mapfile -t commands <"$INPUT_DATA"

${commands[$SLURM_ARRAY_TASK_ID]}
