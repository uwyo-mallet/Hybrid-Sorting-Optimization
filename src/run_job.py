#!/usr/bin/env python3
"""
Submit jobs to a HPC.

Usage:
    run_job.py PARTITION SLURM_DIRS... [options]
    run_job.py -h | --help

Options:
    -e, --exclusive          Slurm exclusive flag.
    -h, --help               Show this help.
    -n, --dry-run            Do everything except actually submit the jobs to slurm.
    -w, --wait=N             Time to wait in seconds between slurm submissions
                             [default: 30]
"""
import os
import shutil
import subprocess
import sys
import time
from collections import defaultdict, namedtuple
from datetime import datetime
from itertools import repeat, takewhile
from pathlib import Path

from docopt import docopt

VERSION = "1.1.0"


class cd:
    """
    Change directories safely, go back to the previous CWD when exiting.

    @see https://stackoverflow.com/a/13197763/8846676
    """

    def __init__(self, new_path, dry_run=False):
        self.new_path = os.path.expanduser(new_path)
        self._dry_run = dry_run

    def __enter__(self):
        if not self._dry_run:
            self.saved_path = os.getcwd()
            os.chdir(self.new_path)

    def __exit__(self, etype, value, traceback):
        if not self._dry_run:
            os.chdir(self.saved_path)


def fast_line_count(filename: Path) -> int:
    """
    Quickly count the number of lines in a text file.

    @param filename: Input file to count lines of
    @returns Number of lines in input file.
    @see https://stackoverflow.com/a/27517681
    """
    with open(filename, "rb") as f:
        bufgen = takewhile(lambda x: x, (f.raw.read(1024 * 1024) for _ in repeat(None)))
        num_lines = sum(buf.count(b"\n") for buf in bufgen if buf)
    return num_lines


# All valid partition names for job submission.
VALID_PARTITIONS = {
    "teton",
    "teton-cascade",
    "teton-hugemem",
    "teton-massmem",
    "teton-knl",
    "moran",
}


Args = namedtuple(
    "Args",
    [
        "partition",
        "slurm_dirs",
        "cwd",
        "job_sbatch",
        "exclusive",
        "dry_run",
        "wait",
    ],
)


def validate(args):
    # Validate user inputs
    if not args.job_sbatch.is_file():
        raise FileNotFoundError("Can't find job.sbatch")
    if args.partition not in VALID_PARTITIONS:
        raise ValueError(f"Invalid partition selection: {args.partition}")

    for d in args.slurm_dirs:
        if not d.is_dir():
            raise NotADirectoryError(f"{d} must be a directory")
        input_files = tuple(d.glob("*.dat"))
        if not input_files:
            raise FileNotFoundError(f"{d} doesn't contain any input .dat files")


def submit(args):
    num_jobs = defaultdict(int)
    for slurm_dir in args.slurm_dirs:
        # Sort numerically, fallback to current order if we can't parse the
        # filenames properly
        input_files = tuple(slurm_dir.glob("*.dat"))
        results_dir = (
            args.cwd
            / ("gb_results" if "gb" in str(slurm_dir) else "results")
            / f"{datetime.now().strftime('%Y-%m-%d_%H-%M-%S')}_{args.partition}_{slurm_dir.name}"
        )
        valgrind_dir = results_dir / "valgrind"
        try:
            input_files = sorted(input_files, key=lambda x: int(x.stem))
        except ValueError:
            pass

        # All user inputs are valid, prep for job submission.
        if not args.dry_run:
            results_dir.mkdir(parents=True, exist_ok=True)
            valgrind_dir.mkdir(parents=True, exist_ok=True)

            shutil.copy(Path(slurm_dir, "job_details.json"), results_dir)
            shutil.copytree(slurm_dir, Path(results_dir, slurm_dir.name))
            Path(results_dir, "partition").write_text(args.partition + "\n")

        print(f"{slurm_dir}: ")
        with cd(results_dir, args.dry_run):
            for batch in input_files:
                num_lines = fast_line_count(batch)
                num_jobs[slurm_dir.name] += num_lines

                if num_lines == 0:
                    print(
                        f"\t[Warning]: Skipping empty data file: {batch}",
                        file=sys.stderr,
                    )
                    continue

                command = [
                    "sbatch",
                    "--array",
                    f"0-{num_lines}",
                    "--partition",
                    args.partition,
                    str(args.job_sbatch),
                    str(batch),
                ]
                if args.exclusive:
                    command.insert(1, "--exclusive")

                print(f"\t{batch.name}: {num_lines}")
                print(f"\t\t{' '.join(command)}")
                if not args.dry_run:
                    subprocess.run(command)

                # Wait between submissions, otherwise slurm fails many jobs.
                if batch != input_files[-1]:
                    time.sleep(args.wait)

        if slurm_dir != args.slurm_dirs[-1]:
            time.sleep(args.wait)

    print("\nTotal number of jobs sumitted:")
    for k, v in num_jobs.items():
        print(f"\t{k}: {v}")


if __name__ == "__main__":
    raw_args = docopt(__doc__, version=VERSION)

    CWD = Path.cwd()
    JOB_SBATCH = Path(CWD, "job.sbatch")
    SLURM_DIRS = [Path(i) for i in raw_args["SLURM_DIRS"]]
    PARTITION = raw_args["PARTITION"].lower()
    DRY_RUN = raw_args["--dry-run"]
    EXCLUSIVE = raw_args["--exclusive"]
    WAIT = float(raw_args["--wait"])

    args = Args(
        PARTITION,
        SLURM_DIRS,
        CWD,
        JOB_SBATCH,
        raw_args["--exclusive"],
        DRY_RUN,
        WAIT,
    )

    validate(args)
    submit(args)
