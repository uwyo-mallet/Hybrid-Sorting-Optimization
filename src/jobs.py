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
    -j, --jobs=N             Do N jobs in parallel.
    -m, --methods=METHODS    Comma seperated list of methods to use for sorters.
    -o, --output=FILE        Output CSV to save results from QST.
    -p, --progress           Enable a progress bar.
    -r, --runs=N             Number of times to run the same input data.
    -s, --slurm=DIR          Generate a batch of slurm data files in this dir.
    -t, --threshold=THRESH   Comma seperated range for threshold (min,max,[step])
                             including both endpoints, or a single value.
"""
import multiprocessing
import os
import shutil
import signal
import subprocess
import sys
import threading
import time
from dataclasses import dataclass
from datetime import datetime
from pathlib import Path
from queue import Queue

from docopt import docopt
from tqdm import tqdm

from info import write_info

VALID_METHODS = (
    "insertion_sort",
    "insertion_sort_asm",
    "qsort_asm",
    "qsort_c",
    "qsort_cpp",
    "std",
)
THRESHOLD_METHODS = ("qsort_asm", "qsort_c", "qsort_cpp")
DATA_TYPES = ("ascending", "descending", "random", "single_num")

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
    runs: int
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
            "--runs",
            str(self.runs),
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


def parse_args(args):
    """Parse CLI args from docopt."""
    parsed = {}

    # Data dir
    parsed["data_dir"] = Path(args.get("DATA_DIR"))
    if not parsed["data_dir"].is_dir():
        raise Exception("Invalid data directory")

    # Executable location
    parsed["exec"] = args.get("--exec") or "./build/QST"
    parsed["exec"] = Path(parsed["exec"])
    if not parsed["exec"].is_file():
        raise FileNotFoundError("Can't find QST executable")

    # Num jobs
    if args.get("--jobs") == "CPU":
        parsed["jobs"] = multiprocessing.cpu_count() - 1
        parsed["jobs"] = parsed["jobs"] if parsed["jobs"] > 0 else 1
    else:
        parsed["jobs"] = args.get("--jobs") or 1
        parsed["jobs"] = int(parsed["jobs"])
        if parsed["jobs"] <= 0:
            raise ValueError("Jobs must be >= 1")

    # Methods
    try:
        methods = args.get("--methods").rsplit(",")
        for i in methods:
            if i not in VALID_METHODS:
                raise ValueError(f"Invalid method: {i}")
    except AttributeError:
        methods = VALID_METHODS
    parsed["methods"] = methods

    # Progress bar
    parsed["progress"] = args.get("--progress")

    # Num runs
    parsed["runs"] = args.get("--runs") or 1
    parsed["runs"] = int(parsed["runs"])
    if parsed["runs"] <= 0:
        raise ValueError("Runs must be >= 1")

    # Slurm
    parsed["slurm"] = (
        Path(args.get("--slurm")) if args.get("--slurm") is not None else None
    )

    # Output
    now = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
    if args.get("--output") is None:
        if parsed["slurm"] is not None:
            # Don't create an output folder if using slurm,
            # that is the shell script's responsibility.
            # Assume that the output should be in the folder I'm in.
            parsed["output"] = Path(f"./output_{now}.csv")
        else:
            parent = Path(f"./results/{now}/")
            parent.mkdir(exist_ok=True, parents=True)
            parsed["output"] = Path(parent, f"./output_{now}.csv")
    else:
        parsed["output"] = Path(args.get("--output"))

    # Threshold
    if args.get("--threshold") is not None:
        # Try if the threshold is a single value
        try:
            buf = int(args.get("--threshold"))
            thresh_range = [buf, buf]
        # Not a single value, try to parse into range (min,max)
        except ValueError:
            try:
                buf = args.get("--threshold").rsplit(",")
                if len(buf) < 2 or len(buf) > 3:
                    raise ValueError
                thresh_range = [int(i) for i in buf]
                if len(thresh_range) == 2:
                    thresh_range.append(1)
            except ValueError as e:
                raise ValueError(f"Invalid threshold: {buf}") from e
    else:
        thresh_range = [4, 4, 1]

    # Ensure thresholds are within sensible range
    if (
        thresh_range[1] < thresh_range[0]
        or thresh_range[0] <= 0
        or thresh_range[1] <= 0
        or thresh_range[2] <= 0
    ):
        raise ValueError(f"Invalid threshold range: {thresh_range}")
    parsed["threshold"] = range(thresh_range[0], thresh_range[1] + 1, thresh_range[2])

    return parsed


class Scheduler:
    """Utility class allowing for easy job generation and scheduling across many threads."""

    def __init__(self, **kwargs):
        self.data_dir = kwargs["data_dir"]
        self.exec = kwargs["exec"]
        self.jobs = kwargs["jobs"]
        self.methods = kwargs["methods"]
        self.output = kwargs["output"]
        self.runs = kwargs["runs"]
        self.slurm = kwargs["slurm"]
        self.threshold = kwargs["threshold"]
        self.progress = kwargs["progress"]

        self.job_queue: "Queue[Job]" = Queue()
        self.active_queue: "Queue[Job]" = Queue()

        self._gen_jobs()

    def _get_exec_version(self):
        cmd = [self.exec, "--version"]
        p = subprocess.Popen(cmd, stdout=subprocess.PIPE)
        stdout, _ = p.communicate()
        return stdout.decode()

    def _gen_jobs(self):
        """Populate the queue with jobs."""
        files = self.data_dir.glob(r"**/*.gz")
        self.job_queue: "Queue[Job]" = Queue()

        for file in files:
            # Get the type of data from one of the subdirectory names.
            desc = "N/A"
            for t in DATA_TYPES:
                if f"/{t}" in str(file):
                    desc = t
                    break

            for method in self.methods:
                # Only methods in THRESHOLD_METHODS care about threshold value.
                # For the others, we can use just one dummy value.
                # QST (> V1.0) corrects this to 0 on the CSV output.
                if method in THRESHOLD_METHODS:
                    for thresh in self.threshold:
                        self.job_queue.put(
                            Job(
                                self.exec,
                                file,
                                desc,
                                method,
                                self.runs,
                                thresh,
                                self.output,
                            )
                        )
                else:
                    self.job_queue.put(
                        Job(
                            self.exec,
                            file,
                            desc,
                            method,
                            self.runs,
                            1,
                            self.output,
                        )
                    )
        self.active_queue = self.job_queue

    def _restore_jobs(self):
        self.active_queue = self.job_queue

    def _worker(self):
        """Worker function for each thread."""
        while True:
            job = self.active_queue.get()
            job.run(quiet=self.progress)
            self.active_queue.task_done()

            self.pbar.update()

    def run_jobs(self):
        """Run all the jobs on the local machine."""
        print("===========================", file=sys.stderr)
        print(f"About to run {self.active_queue.qsize()} jobs", file=sys.stderr)
        print("===========================", file=sys.stderr)
        time.sleep(3)
        print("Okay, lets do it!", file=sys.stderr)

        self.pbar = tqdm(total=self.active_queue.qsize(), disable=not self.progress)

        # Log system info
        write_info(
            self.output.parent,
            command=" ".join(sys.argv),
            data_details_path=Path(self.data_dir, "details.json"),
            concurrent=self.jobs,
            qst_vers=self._get_exec_version(),
            runs=self.runs,
            total_num_jobs=self.active_queue.qsize(),
            total_num_sorts=self.active_queue.qsize() * self.runs,
        )
        # Create my own process group
        os.setpgrp()
        try:
            for _ in range(self.jobs):
                threading.Thread(target=self._worker, daemon=True).start()
            self.active_queue.join()
        except KeyboardInterrupt:
            # Kill myself and all my processes if told to
            os.killpg(0, signal.SIGKILL)

    def gen_slurm(self):
        """Create the slurm.d/ directory with all necessary parameters."""
        if self.slurm.exists() and self.slurm.is_dir():
            shutil.rmtree(self.slurm)
        elif self.slurm.is_file():
            raise FileExistsError("Slurm output cannot be a file")

        self.slurm.mkdir()
        write_info(
            self.slurm,
            command=" ".join(sys.argv),
            concurrent="slurm",
            data_details_path=Path(self.data_dir, "details.json"),
            qst_vers=self._get_exec_version(),
            runs=self.runs,
            total_num_jobs=self.active_queue.qsize(),
            total_num_sorts=self.active_queue.qsize() * self.runs,
        )
        index = 0
        while not self.active_queue.empty():
            with open(Path(self.slurm, f"{index}.dat"), "w") as slurm_file:
                size = 0
                while not self.active_queue.empty() and size < MAX_BATCH:
                    job = self.active_queue.get()
                    slurm_file.write(job.cli + "\n")
                    self.active_queue.task_done()
                    size += 1

            index += 1


if __name__ == "__main__":
    args = parse_args(docopt(__doc__))

    s = Scheduler(**args)
    if args["slurm"]:
        s.gen_slurm()
    else:
        s.run_jobs()
