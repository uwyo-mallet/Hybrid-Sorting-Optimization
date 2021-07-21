#!/bin/bash

# DO NOT CALL OR EDIT THIS DIRECTLY.
# This file is only used by run_job.sh

#SBATCH --job-name=Quicksort-Tuning
#SBATCH --account=mallet
#SBATCH --partition=teton
#SBATCH --time=05:00:00
#SBATCH --nodes=1
#SBATCH --mem=2G
#SBATCH --signal=TERM@300

echo "- - - - - - - - - - - - - - - - - - - - -"
echo "SLURM_JOB_ID: $SLURM_JOB_ID"
echo "SLURM_JOB_NAME: $SLURM_JOB_NAME"
echo "SLURM_JOB_NODELIST: $SLURM_JOB_NODELIST"
echo "SLURM_ARRAY_TASK_ID: $SLURM_ARRAY_TASK_ID"
echo "- - - - - - - - - - - - - - - - - - - - -"

INPUT_DATA="$1"
mapfile -t commands <"$INPUT_DATA"

echo "- - - - - - - - - - - - - - - - - - - - -"
${commands[$SLURM_ARRAY_TASK_ID]}
echo "- - - - - - - - - - - - - - - - - - - - -"

echo "DONE"
