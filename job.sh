#!/bin/bash

#SBATCH --job-name=Quicksort-Tuning
#SBATCH --account=mallet
#SBATCH --time=12:00:00
#SBATCH --nodes=1
#SBATCH --cpus-per-task=32
#SBATCH --mem=4G
#SBATCH --mail-type=ALL
#SBATCH --mail-user=jarulsam@uwyo.edu

module load python/3.7.6
module load miniconda3/4.3.30

WORKING_DIR="/project/mallet/jarulsam/quicksort-tuning/"
cd "$WORKING_DIR" || exit 1
source activate /project/mallet/jarulsam/job_py

python --version

echo "- - - - - - - - - - - - - - - - - - - - -"
srun python src/jobs.py data -j32 -e ./build/QST
echo "- - - - - - - - - - - - - - - - - - - - -"

echo "Deactivate conda"
source deactivate

echo "DONE"
