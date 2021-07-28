#!/usr/bin/env python3
"""
Write info about the system to a FOLDER/system_details.pickle.

Usage:
    info.py DIR [options]

Options:
    -h, --help               Show this help.
    -q, --qst=QST            Specify QST version.
    -t, --total=N            Specify total number of jobs
"""
import json
import platform
from pathlib import Path

from docopt import docopt


def write_info(output_folder, total_num_jobs=None, concurrent="slurm", qst_vers=None):
    """Write system information to output_folder/job_details.json."""
    info = {
        "Architecture": platform.architecture(),
        "Machine": platform.machine(),
        "Node": platform.node(),
        "Number of concurrent jobs": concurrent,
        "Platform": platform.platform(),
        "Processor": platform.processor(),
        "QST Version": qst_vers,
        "Release": platform.release(),
        "System": platform.system(),
        "Total number of jobs": total_num_jobs,
        "Version": platform.version(),
    }

    with open(Path(output_folder, "job_details.json"), "w") as f:
        json.dump(info, f, sort_keys=True)


if __name__ == "__main__":
    args = docopt(__doc__)
    OUT_DIR = Path(args.get("DIR"))
    write_info(OUT_DIR, args.get("--total"), qst_vers=args.get("--qst"))
