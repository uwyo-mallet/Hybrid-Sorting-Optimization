#!/usr/bin/env python3
"""
Automatically dispatch sort jobs of varying inputs, methods, and threshold values.

Usage:
    jobs.py DATA_DIR [options]
    jobs.py -h | --help

Options:
    -h, --help                      Show this help.
    -e EXEC, --exec=EXEC            Path to executable QST.
    -f, --force                     Overwrite existing.
    -j N, --jobs=N                  Do N jobs in parallel.
    -m METHODS, --methods=METHODS   Comma seperated list of methods to use for sorters.
    -o FILE, --output=FILE          Output to save data.
    -t THRESH, --threshold THRESH   Comma seperated range for threshold (min,max) including both endpoints, or a single value.
"""

import os
import signal
import subprocess
import threading
from dataclasses import dataclass
from datetime import datetime
from pathlib import Path, PosixPath
from pprint import pprint
from queue import Queue

from docopt import docopt


@dataclass
class Job:
    exec_path: Path
    infile_path: Path
    method: str
    threshold: int
    output: Path


def worker():
    while not queue.empty():
        job = queue.get()
        print(job)
        to_run = [
            str(job.exec_path.absolute()),
            str(job.infile_path.absolute()),
            "--method",
            str(job.method),
            "--output",
            str(job.output.absolute()),
            "--threshold",
            str(job.threshold),
        ]
        p = subprocess.Popen(to_run)
        p.wait()


if __name__ == "__main__":
    args = docopt(__doc__, version="1.0.0")
    now = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")

    # Cleanup CLI inputs
    args["--exec"] = args.get("--exec") or "./build/QST"
    args["--output"] = args.get("--output") or f"./output_{now}.csv"

    VALID_METHODS = ("vanilla_quicksort", "qsort_c", "insertion_sort")

    DATA_DIR = Path(args.get("DATA_DIR"))
    OUTPUT_PATH = Path(args.get("--output"))
    QST_PATH = Path(args.get("--exec"))
    NUM_JOBS = args.get("--jobs") or 1
    NUM_JOBS = int(NUM_JOBS)
    THRESH_RANGE = [4, 4]

    # Parse methods
    if args.get("--methods") is not None:
        METHODS = args.get("--methods").rsplit(",")
        for i in METHODS:
            if i not in VALID_METHODS:
                raise ValueError(f"Invalid method: {i}")
    else:
        METHODS = VALID_METHODS

    # Parse threshold and validate
    if args.get("--threshold") is not None:
        try:
            buf = int(args.get("--threshold"))
            THRESH_RANGE = [buf, buf]
        except ValueError:
            # Poor man's goto equivalent for error check
            try:
                buf = args.get("--threshold").rsplit(",")
                if len(buf) != 2:
                    raise ValueError
                THRESH_RANGE = [int(i) for i in buf]
            except ValueError:
                raise ValueError(f"Invalid threshold: {buf}")
    if (
        THRESH_RANGE[1] < THRESH_RANGE[0]
        or THRESH_RANGE[0] <= 0
        or THRESH_RANGE[1] <= 0
    ):
        raise ValueError(f"Invalid threshold: {THRESH_RANGE}")
    THRESHOLDS = list(range(THRESH_RANGE[0], THRESH_RANGE[1] + 1))

    # Error check CLI args
    if not QST_PATH.is_file():
        raise FileNotFoundError("Can't find QST executable")
    if OUTPUT_PATH.exists() and not args.get("--force"):
        raise FileExistsError("Output already exists")
    if NUM_JOBS < 0:
        raise ValueError("Jobs must be >= 1")
    if not DATA_DIR.exists() or not DATA_DIR.is_dir():
        raise Exception("Invalid data directory")

    if args.get("--force"):
        OUTPUT_PATH.unlink(missing_ok=True)

    # Create all the jobs to run
    files = list(DATA_DIR.glob("**/*.dat"))
    queue = Queue()
    for file in files:
        for method in METHODS:
            # Only qsort_c cares about thresh. For the others, we can use a dummy value.
            if method == "qsort_c":
                for thresh in THRESHOLDS:
                    queue.put(Job(QST_PATH, file, method, thresh, OUTPUT_PATH))
            else:
                queue.put(Job(QST_PATH, file, method, 1, OUTPUT_PATH))

    # Create my own process group and start all the jobs in parrallel
    os.setpgrp()
    try:
        threads = [threading.Thread(target=worker) for _ in range(NUM_JOBS)]
        # Start each thread and wait till they all complete
        for thread in threads:
            thread.start()
        for thread in threads:
            thread.join()
    except KeyboardInterrupt:
        # Kill all processes in my group
        os.killpg(0, signal.SIGKILL)
