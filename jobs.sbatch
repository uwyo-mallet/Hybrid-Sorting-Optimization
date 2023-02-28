#!/bin/bash
#
#SBATCH --job-name=Quicksort-Tuning
#SBATCH --account=mallet
#SBATCH --time=23:00:00
#SBATCH --nodes=1
#SBATCH --mem=4G
#SBATCH --signal=TERM@300
#SBATCH --profile=all
#SBATCH --output=stdout_%A.out

# Load required modules
module load gcc/12.2.0
module load python/3.10.6
source /project/mallet/jarulsam/venv/bin/activate

cd /project/mallet/jarulsam/quicksort-tuning

# Mimic the src/jobs.py script
printf "%s\n" "$@"

python src/jobs.py "$@"
