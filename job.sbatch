#!/bin/bash

#SBATCH --job-name=Quicksort-Tuning
#SBATCH --account=mallet
#SBATCH --time=01:00:00
#SBATCH --nodes=1
#SBATCH --mem=1G

echo "SLURM_JOB_ID:" $SLURM_JOB_ID
echo "SLURM_JOB_NAME:" $SLURM_JOB_NAME
echo "SLURM_JOB_NODELIST:" $SLURM_JOB_NODELIST
echo "SLURM_ARRAY_TASK_ID:" $SLURM_ARRAY_TASK_ID

INPUT_DATA="/project/mallet/jarulsam/quicksort-tuning/slurm.dat"
mapfile -t commands < "$INPUT_DATA"

echo "- - - - - - - - - - - - - - - - - - - - -"
${commands[$SLURM_ARRAY_TASK_ID]}
echo "- - - - - - - - - - - - - - - - - - - - -"

echo "DONE"
