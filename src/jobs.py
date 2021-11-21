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
    --callgrind              Enable callgrind data collection for each job.
    --massif                 Enable massif data collection for each job.
"""
import itertools
import multiprocessing
import os
import random
import shutil
import signal
import subprocess
import sys
import threading
import time
from collections import deque
from dataclasses import dataclass
from datetime import datetime
from pathlib import Path
from typing import Union

from docopt import docopt
from tqdm import tqdm

from info import write_info

VERSION = "1.0.5"

VALID_METHODS = {
    "insertion_sort",
    "insertion_sort_asm",
    "insertion_sort_c",
    "qsort_asm",
    "qsort_c",
    "qsort_c_swp",
    "qsort_cpp",
    "qsort_cpp_no_comp",
    "qsort_sanity",
    "std",
}
THRESHOLD_METHODS = {
    "qsort_asm",
    "qsort_c",
    "qsort_c_swp",
    "qsort_cpp",
    "qsort_cpp_no_comp",
}
DATA_TYPES = {"ascending", "descending", "random", "single_num"}


# Maximum array index supported by slurm
# https://slurm.schedmd.com/job_array.html
MAX_BATCH = 4_500


@dataclass
class Job:
    job_id: int
    exec_path: Path
    infile_path: Path
    description: str
    method: str
    runs: int
    output: Path
    threshold: int

    callgrind: Union[Path, None]
    massif: Union[Path, None]

    @property
    def commands(self):
        """Return all possible commands given the parameters from init."""
        all_commands = []
        base_command = [
            str(self.exec_path.absolute()),
            str(self.infile_path.absolute()),
            "--id",
            str(self.job_id),
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
        base_valgrind_opts = [
            "valgrind",
            "--time-stamp=yes",
            "--quiet",
        ]

        # Not using valgrind, just pass the base command
        if not any([self.callgrind, self.massif]):
            return [base_command]

        # Parse valgrind specific stuff
        if self.callgrind:
            out = self.callgrind / f"{self.job_id}_callgrind.out"
            out = str(out)
            opts = [
                "--tool=callgrind",
                f"--callgrind-out-file={out}",
                "--dump-line=yes",
                "--dump-instr=yes",
                "--instr-atstart=yes",
                "--collect-atstart=yes",
                "--collect-jumps=yes",
                "--collect-systime=yes",
                "--collect-bus=yes",
                "--cache-sim=yes",
                "--branch-sim=yes",
                "--",
            ]
            all_commands.append(
                list(itertools.chain(base_valgrind_opts, opts, base_command))
            )
        if self.massif:
            out = self.massif / f"{self.job_id}_massif.out"
            out = str(out)
            opts = [
                "--tool=massif",
                f"--massif-out-file={out}",
                "--stacks=yes",
                "--",
            ]
            all_commands.append(
                list(itertools.chain(base_valgrind_opts, opts, base_command))
            )

        return all_commands

    @property
    def size(self):
        return len(self.commands)

    @property
    def cli(self):
        # """Return the raw CLI equivalent of the subprocess.Popen command."""
        return [" ".join([str(i) for i in command]) for command in self.commands]

    def run(self, quiet=False):
        """Call the subprocess and run the job."""
        for i in self.commands:
            if not quiet:
                print(i)
            subprocess.run(i, capture_output=True)


def parse_args(args):
    """Parse CLI args from docopt."""
    parsed = {}

    # Data dir
    parsed["data_dir"] = Path(args.get("DATA_DIR"))
    if not parsed["data_dir"].is_dir():
        raise NotADirectoryError("Invalid data directory")

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

    # Valgrind options
    for i in ("callgrind", "massif"):
        arg = args.get(f"--{i}")
        if arg:
            arg = parsed["output"].parent / "valgrind"
            if parsed["slurm"] is None:
                arg.mkdir(exist_ok=True)
        parsed[i] = arg

    return parsed


class Scheduler:
    """Utility class allowing for easy job generation and scheduling across many threads."""

    def __init__(
        self,
        data_dir: Path,
        exec: Path,
        jobs: int,
        methods: list[str],
        output: Path,
        runs: int,
        slurm: Path,
        threshold: range,
        progress: bool,
        callgrind: Path,
        massif: Path,
    ):
        """
        Define the base parameters

        @param data_dir: Path to input data.
        @param exec: Path to QST executable.
        @param jobs: Number of jobs to run concurrently.
        @param methods: List of all the methods to test.
        @param output: Path to output CSV file with all test results.
        @param runs: Number of times to repeat sorts on the given data.
        @param slurm: Optional path to output slurm.d/ folder. Only provide if
                      not running jobs on local machine.
        @param threshold: Range of thresholds to test with each method.
        @param progress: Optionally enable a progress bar.
        @param callgrind: Optionally run all experiments with callgrind.
        @param massif: Optionally run all experiments with massif.
        """
        self.data_dir = data_dir
        self.exec = exec
        self.jobs = jobs
        self.methods = methods
        self.output = output
        self.runs = runs
        self.slurm = slurm
        self.threshold = threshold
        self.progress = progress

        self.callgrind = callgrind
        self.massif = massif

        self.job_queue: "deque[Job]" = deque()
        self.active_queue: "deque[Job]" = deque()

        self._gen_jobs()

    def _get_exec_version(self) -> str:
        """Call the QST process and parse the version output."""
        cmd = [self.exec, "--version"]
        p = subprocess.Popen(cmd, stdout=subprocess.PIPE)
        stdout, _ = p.communicate()
        return stdout.decode()

    def _gen_subjobs(self, job_id: int, params) -> tuple[list[Job], int]:
        """
        Generate any possible valgrind subjobs from this specfic job.

        Since the job object is responsible for keeping track of its subprocess
        command and its valgrind specific subprocess commands, we do a somewhat
        goofy method of creating an invididual job for each sub type of
        valgrind. This can probably eventually be refactored into the Job
        object itself. But for now, this is the easiet way to keep the job_id
        and valgrind outputs synced and unique.

        @param job_id: Current largest job_id to increment from.
        @param params: Tuple containing all the other positional arguments
                       required for for the creation of the job.

        @return Tuple containing a list of all the new jobs, and an integer
                representing the next job_id.
        """
        subjobs = []
        if not any([self.callgrind, self.massif]):
            job = Job(job_id, *params, callgrind=None, massif=None)
            return [job], job_id + 1
        if self.callgrind:
            job = Job(
                job_id,
                *params,
                callgrind=self.callgrind,
                massif=None,
            )
            subjobs.append(job)
            job_id += 1
        if self.massif:
            job = Job(
                job_id,
                *params,
                massif=self.massif,
                callgrind=None,
            )
            subjobs.append(job)
            job_id += 1

        return subjobs, job_id

    def _gen_jobs(self):
        """Populate the queue with jobs."""
        files = self.data_dir.glob(r"**/*.gz")
        self.job_queue.clear()

        job_id = 0
        for f in files:
            # Get the type of data from one of the subdirectory names.
            desc = "N/A"
            for t in DATA_TYPES:
                if f"/{t}" in str(f):
                    desc = t
                    break

            for method in self.methods:
                # Only methods in THRESHOLD_METHODS care about threshold value.
                # For the others, we can use just one dummy value.
                # QST (> V1.0) corrects this to 0 on the CSV output.
                base_params = (self.exec, f, desc, method, self.runs, self.output)
                if method in THRESHOLD_METHODS:
                    for thresh in self.threshold:
                        params = base_params + (thresh,)
                        subjobs, job_id = self._gen_subjobs(job_id, params)
                        self.job_queue += subjobs
                else:
                    params = base_params + (1,)
                    subjobs, job_id = self._gen_subjobs(job_id, params)
                    self.job_queue += subjobs

        self.active_queue = self.job_queue
        random.shuffle(self.active_queue)

    def _restore_jobs(self):
        """Bring all the jbos from the last _gen_jobs() back into the active queue."""
        self.active_queue = self.job_queue

    def _worker(self):
        """Worker function for each thread."""
        while self.active_queue:
            job = self.active_queue.pop()
            job.run(quiet=self.progress)
            self.pbar.update()

    def run_jobs(self):
        """Run all the jobs on the local machine."""
        print("===========================", file=sys.stderr)
        print(f"About to run {len(self.active_queue)} jobs", file=sys.stderr)
        print("===========================", file=sys.stderr)
        time.sleep(3)
        print("Okay, lets do it!", file=sys.stderr)

        self.pbar = tqdm(total=len(self.active_queue), disable=not self.progress)

        # Log system info
        write_info(
            self.output.parent,
            command=" ".join(sys.argv),
            data_details_path=Path(self.data_dir, "details.json"),
            concurrent=self.jobs,
            qst_vers=self._get_exec_version(),
            runs=self.runs,
            total_num_jobs=len(self.active_queue),
            total_num_sorts=len(self.active_queue) * self.runs,
        )
        # Create my own process group
        os.setpgrp()
        threads = []
        try:
            for _ in range(self.jobs):
                threads.append(threading.Thread(target=self._worker, daemon=True))
            for i in threads:
                i.start()
            for i in threads:
                i.join()
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
            total_num_jobs=len(self.active_queue),
            total_num_sorts=len(self.active_queue) * self.runs,
        )
        index = 0
        while self.active_queue:
            current_file = Path(self.slurm, f"{index}.dat")
            with open(current_file, "w") as slurm_file:
                size = 0
                while self.active_queue and size < MAX_BATCH:
                    job = self.active_queue.pop()
                    for i in job.cli:
                        slurm_file.write(i + "\n")
                    size += job.size
            print(f"{current_file}: {size}")

            index += 1


if __name__ == "__main__":
    args = parse_args(docopt(__doc__, version=VERSION))
    s = Scheduler(**args)
    if args["slurm"]:
        s.gen_slurm()
    else:
        s.run_jobs()
