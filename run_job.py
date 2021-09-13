#!/usr/bin/env python3
"""
Submit jobs to a HPC.

Usage:
    run_job.py SLURM_DIR PARTITION [options]
    run_job.py -h | --help

Options:
    -h, --help               Show this help.
    -n, --dry-run            Do everything except actually submit the jobs to slurm.
    -w, --wait=N             Time to wait in seconds between slurm submissions
                             [default: 30]
"""
import shutil
import subprocess
import time
from datetime import datetime
from itertools import repeat, takewhile
from pathlib import Path

from docopt import docopt


def fast_line_count(filename):
    """Quickly count the number of lines in a text file."""
    with open(filename, "rb") as f:
        bufgen = takewhile(lambda x: x, (f.raw.read(1024 * 1024) for _ in repeat(None)))
        num_lines = sum(buf.count(b"\n") for buf in bufgen if buf)
    return num_lines


PARTITIONS = (
    "teton",
    "teton-cascade",
    "teton-hugemem",
    "teton-massmem",
    "teton-knl",
    "moran",
)

args = docopt(__doc__)

CWD = Path.cwd()
JOB_SBATCH = Path(CWD, "job.sbatch")
SLURM_DIR = Path(args["SLURM_DIR"])
PARTITION = args["PARTITION"].lower()
DRY_RUN = args["--dry-run"]
WAIT = float(args["--wait"])

RESULTS_DIR = Path(
    CWD, "results", datetime.now().strftime("%Y-%m-%d_%H-%M-%S") + "_" + PARTITION
)

# Validate user inputs
if not JOB_SBATCH.is_file():
    raise FileNotFoundError("Can't find job.sbatch")
if not SLURM_DIR.is_dir():
    raise NotADirectoryError("SLURM_DIR must be a directory")
if PARTITION not in PARTITIONS:
    raise ValueError(f"Invalid partition selection: {PARTITION}")

input_files = tuple(SLURM_DIR.glob("*.dat"))
if not input_files:
    raise FileNotFoundError(f"{SLURM_DIR} doesn't contain any input .dat files")

# Sort numerically, fallback to current order if we can't parse the filename properly
try:
    input_files = tuple(sorted(input_files, key=lambda x: int(x.stem)))
except ValueError:
    pass

# All user inputs are valid, prep for job submission.
if not DRY_RUN:
    RESULTS_DIR.mkdir(parents=True, exist_ok=True)
    shutil.copy(Path(SLURM_DIR, "job_details.json"), RESULTS_DIR)
    shutil.copytree(SLURM_DIR, Path(RESULTS_DIR, SLURM_DIR.name))
    Path(RESULTS_DIR, "partition").write_text(PARTITION + "\n")


total_num_jobs = 0
for batch in input_files:
    num_lines = fast_line_count(batch)

    if num_lines == 0:
        print(f"[Warning]: Skipping empty data file: {batch}")
        continue

    print(f"{batch.name}: {num_lines}")
    command = [
        "sbatch",
        "--array",
        f"0-{num_lines}",
        "--partition",
        PARTITION,
        str(JOB_SBATCH),
        str(batch),
    ]

    print("\t" + " ".join(command))
    if not DRY_RUN:
        subprocess.run(command)
    total_num_jobs += num_lines

    # Wait between submissions, otherwise slurm fails many jobs.
    if batch != input_files[-1]:
        time.sleep(WAIT)

print(f"Total number of jobs: {total_num_jobs}")
