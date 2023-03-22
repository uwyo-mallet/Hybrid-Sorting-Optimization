#!/bin/bash
#
#SBATCH --job-name=Hybrid-Sort-Optimization
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
module load singularity
source /project/mallet/jarulsam/venv/bin/activate

cd /project/mallet/jarulsam/Hybrid-Sorting-Optimization
mkdir -p ./experiment

# Mimic the src/jobs.py script
printf "%s " "$@"
printf "\n"

mkdir -p ./experiment
singularity run --bind ./experiment:/HSO/experiment "$@"
