#!/usr/bin/env python3
"""
Automatically dispatch sort jobs of varying inputs, methods, and threshold values.

This essentially serves as a job scheduler, with optional features to easily
dispatch slurm job array batches. Only use the native scheduling features if
running on a local multi-core machine.

Usage:
    jobs.py DATA_DIR [options]
    jobs.py -h | --help

Options:
    -h, --help               Show this help.
    -e, --exec=EXEC          Path to QST executable.
    -f, --force              Overwrite existing if applicable.
    -j, --jobs=N             Do N jobs in parallel.
    -m, --methods=METHODS    Comma seperated list of methods to use for sorters.
    -o, --output=FILE        Output to save results from QST.
    -r, --runs=N             Number of times to run the same input data.
                             This option and -s are mutually exclusive.
    -s, --slurm=SLURM        Generate a slurm job data file and save to SLURM.
    -t, --threshold=THRESH   Comma seperated range for threshold (min,max)
                             including both endpoints, or a single value.
"""
import os
import shutil
import signal
import subprocess
import threading
from dataclasses import dataclass
from datetime import datetime
from pathlib import Path
from queue import Queue

from docopt import docopt

from info import write_info

# Maximum array index supported by slurm
# https://slurm.schedmd.com/job_array.html
MAX_BATCH = 4_500


@dataclass
class Job:
    """Dataclass storing the parameters of a single call to the QST subprocess."""

    exec_path: Path
    infile_path: Path
    description: str
    method: str
    threshold: int
    output: Path

    @property
    def command(self):
        """Return the appropiate format for subprocess.Popen."""
        return [
            str(self.exec_path.absolute()),
            str(self.infile_path.absolute()),
            "--description",
            str(self.description),
            "--method",
            str(self.method),
            "--output",
            str(self.output),
            "--threshold",
            str(self.threshold),
        ]

    @property
    def cli(self):
        """Return the raw CLI equivalent of the subprocess.Popen command."""
        return " ".join(self.command)

    def run(self, quiet=False):
        """Call the subprocess and run the job."""
        to_run = self.command
        if not quiet:
            print(to_run)

        p = subprocess.Popen(to_run)
        p.wait()


def worker():
    """Run jobs in queue until queue is empty, used for threads."""
    while not queue.empty():
        job = queue.get()
        job.run()


if __name__ == "__main__":
    args = docopt(__doc__)
    now = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")

    # Cleanup CLI inputs
    args["--exec"] = args.get("--exec") or "./build/QST"

    # Setup constants
    VALID_METHODS = ("vanilla_quicksort", "qsort_c", "insertion_sort", "std")
    DATA_TYPES = ("ascending", "descending", "random", "single_num")

    DATA_DIR = Path(args.get("DATA_DIR"))
    OUTPUT_PATH = args.get("--output")
    QST_PATH = Path(args.get("--exec"))
    NUM_JOBS = args.get("--jobs") or 1
    NUM_JOBS = int(NUM_JOBS)

    NUM_REPEATS = args.get("--repeats") or 1
    NUM_REPEATS = int(NUM_REPEATS)

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
            except ValueError as e:
                raise ValueError(f"Invalid threshold: {buf}") from e
    if (
        THRESH_RANGE[1] < THRESH_RANGE[0]
        or THRESH_RANGE[0] <= 0
        or THRESH_RANGE[1] <= 0
    ):
        raise ValueError(f"Invalid threshold range: {THRESH_RANGE}")
    THRESHOLDS = list(range(THRESH_RANGE[0], THRESH_RANGE[1] + 1))

    # Create default output path if no override
    if OUTPUT_PATH is None:
        if args.get("--slurm") is not None:
            # Don't create an output folder if using slurm,
            # that is the shell script's responsibility.
            # Assume that the output should be in the folder I'm in.
            OUTPUT_PATH = Path(f"./output_{now}.csv")
        else:
            output_folder = Path(f"./results/{now}/")
            output_folder.mkdir(exist_ok=True, parents=True)
            OUTPUT_PATH = Path(output_folder, f"./output_{now}.csv")

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
    files = list(DATA_DIR.glob("**/*.gz"))
    queue: "Queue[Job]" = Queue()

    # Generate the jobs
    for file in files:
        # Determine the type of each input data file
        desc = "N/A"
        for t in DATA_TYPES:
            if f"/{t}" in str(file):
                desc = t
                break

        for method in METHODS:
            # Only qsort_c cares about thresh (for now).
            # For the others, we can use just one dummy value.
            # QST corrects this to 0 on the CSV output.
            if method == "qsort_c":
                for thresh in THRESHOLDS:
                    queue.put(Job(QST_PATH, file, desc, method, thresh, OUTPUT_PATH))
            else:
                queue.put(Job(QST_PATH, file, desc, method, 1, OUTPUT_PATH))

    if args.get("--slurm") is None:
        # Save some deatils about what platform we are running on.
        write_info(OUTPUT_PATH.parent, NUM_JOBS)

        for i in range(NUM_REPEATS):
            # Create my own process group and start all the jobs in parrallel
            os.setpgrp()
            try:
                threads = [threading.Thread(target=worker) for _ in range(NUM_JOBS)]
                # Start each thread and wait till they all complete
                for thread in threads:
                    thread.start()
                # Wait till all are started before waiting
                for thread in threads:
                    thread.join()
            except KeyboardInterrupt:
                # Kill all processes in my group if stopped early.
                os.killpg(0, signal.SIGKILL)
    else:
        SLURM_DIR = Path(args.get("--slurm"))

        if SLURM_DIR.exists() and SLURM_DIR.is_dir():
            shutil.rmtree(SLURM_DIR)
        SLURM_DIR.mkdir()

        index = 0
        while not queue.empty():
            with open(Path(SLURM_DIR, f"{index}.dat"), "w") as slurm_file:
                size = 0
                while not queue.empty() and size < MAX_BATCH:
                    job = queue.get()
                    slurm_file.write(job.cli + "\n")
                    size += 1
            index += 1
