#!/usr/bin/env python3
"""
Write info about the system to a FOLDER/system_details.pickle.

Usage:
    info.py DIR [options]

Options:
    -h, --help               Show this help.
    -c, --concurrent=N       Specify number of simultaenous runs jobs.
    -q, --qst=QST            Specify QST version.
    -r, --runs=N             Specify number of runs
    -t, --total=N            Specify total number of jobs
"""
import json
import multiprocessing
import platform
from pathlib import Path

from docopt import docopt


def write_info(
    output_folder,
    concurrent="slurm",
    qst_vers="",
    runs=0,
    total_num_jobs=0,
):
    """Write system information to output_folder/job_details.json."""
    info = {
        "Architecture": platform.architecture(),
        "Machine": platform.machine(),
        "Node": platform.node(),
        "Number of CPUs": multiprocessing.cpu_count(),
        "Number of concurrent jobs": concurrent,
        "Platform": platform.platform(),
        "Processor": platform.processor(),
        "QST Version": qst_vers,
        "Release": platform.release(),
        "Runs": runs,
        "System": platform.system(),
        "Total number of jobs": total_num_jobs,
        "Version": platform.version(),
    }

    with open(Path(output_folder, "job_details.json"), "w") as f:
        json.dump(info, f, indent=4, sort_keys=True)


if __name__ == "__main__":
    args = docopt(__doc__)
    OUT_DIR = Path(args.get("DIR"))
    write_info(
        OUT_DIR,
        concurrent=args.get("--concurrent"),
        qst_vers=args.get("--qst"),
        runs=args.get("--runs"),
        total_num_jobs=args.get("--total"),
    )
