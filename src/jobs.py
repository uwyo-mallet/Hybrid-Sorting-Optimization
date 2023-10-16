#!/usr/bin/env python3
"""
Automatically dispatch sort jobs of varying inputs, methods, and threshold
values.

This essentially serves as a job scheduler, with optional features to easily
dispatch slurm job array batches. Only use the native scheduling features if
running on a local multi-core machine.

Usage:
    jobs.py <EXEC> <DATA_DIR> [options]
    jobs.py <EXEC> <DATA_DIR> [options] (--threshold=THRESH ...)
    jobs.py <EXEC> <DATA_DIR> [options] (--valgrind-opt=OPT ...)
    jobs.py <EXEC> <DATA_DIR> [options] (--threshold=THRESH ...) (--valgrind-opt=OPT ...)
    jobs.py -h | --help

Options:
    -h, --help               Show this help.
    -j, --jobs=N             Do N jobs in parallel.
    -c, --output-chunks=N    Preaverage N chunks within HSO itself.
    -m, --methods=METHODS    Comma seperated list of methods to use for sorters.
    -o, --output=FILE        Output CSV to save results.
    -p, --progress           Enable a progress bar.
    -r, --runs=N             Number of times to run the same input data.
    -s, --slurm=DIR          Generate a batch of slurm data files in this dir.
    -t, --threshold=THRESH   Comma seperated range for threshold (min,max,[step])
                             including both endpoints, or a single value.

    --valgrind-opt=OPT       Any additional options to pass through to valgrind.
                             Can be specified multiple times. Each and every
                             CLI argument must be paired with this flag,
                             otherwise a CalledProcess exception will be raised.

    --base                   Collect a normal sample along with any valgrind options.
                             Ignored if not accompanied by any of the following options.
    --callgrind              Enable callgrind data collection for each job.
    --cachegrind             Enable cachegrind data collection for each job.
    --massif                 Enable massif data collection for each job.
    --arcc-partition=PART    ARCC Partition, Must be parseable JSON.

"""
import itertools
import json
import multiprocessing
import os
import platform
import shutil
import signal
import subprocess
import sys
import threading
from collections import deque
from copy import deepcopy
from dataclasses import dataclass
from datetime import datetime
from pathlib import Path
from typing import Optional

from docopt import docopt
from tqdm import tqdm

from info import get_supported_methods, write_info

VERSION = "1.1.7"


DATA_TYPES = {"ascending", "descending", "random", "single_num", "pipe_organ"}

# Maximum array index supported by slurm
# https://slurm.schedmd.com/job_array.html
MAX_BATCH = 4_500


@dataclass
class Job:
    """Represent a single call to executable."""

    job_id: int
    exec_path: Path
    infile_path: Path
    description: str
    method: str
    runs: int
    output: Path
    threshold: int

    base: bool
    callgrind: bool
    cachegrind: bool
    massif: bool

    valgrind_opts: Optional[list[str]]

    def __init__(
        self,
        job_id,
        exec_path,
        infile_path,
        description,
        method,
        runs,
        output,
        threshold,
        output_chunks=0,
        base=False,
        callgrind=False,
        cachegrind=False,
        massif=False,
        valgrind_opts=None,
    ):
        """
        Define the base parameters.

        @param job_id: Unique identifier for this 'run'.
        @param exec_path: Path to executable.
        @param infile_path: Path to input data.
        @param description: Input data description.
        @param method: Methods to use to sort.
        @param runs: Number of times to sort the same data.
        @param output: CSV to write resulting time data.
        @param threshold: Value at which to switch to insertion sort if supported
                          for this method.

        @param base: Run without valgrind.
        @param callgrind: Run with callgrind.
        @param cachegrind: Run with cachegrind.
        @param massif: Run with massif.

        Any of the last 4 options can be True at the same time. All of them will
        run depending on their status.

        @param valgrind_opts: Other arguments to pass to valgrind. Only used
                              when callgrind, cachegrind, or massif are True.
        """
        self.job_id = job_id
        self.exec_path = exec_path
        self.infile_path = infile_path
        self.description = description
        self.method = method
        self.runs = runs
        self.output = output
        self.threshold = threshold
        self.output_chunks = output_chunks
        self.base = base

        self.callgrind = None
        self.cachegrind = None
        self.massif = None
        self.valgrind_opts = valgrind_opts

        if callgrind:
            self.callgrind = (
                self.output.parent / "valgrind" / f"{self.job_id}_callgrind.out"
            )
        if cachegrind:
            self.cachegrind = (
                self.output.parent / "valgrind" / f"{self.job_id}_cachegrind.out"
            )
        if massif:
            self.massif = self.output.parent / "valgrind" / f"{self.job_id}_massif.out"

        if not any([self.callgrind, self.cachegrind, self.massif]):
            self.base = True

    @staticmethod
    def _passthrough_args(passthrough) -> list[str]:
        """Generate arguments for passthrough options."""
        return [
            "--cols",
            ",".join(passthrough.keys()),
            "--vals",
            ",".join(passthrough.values()),
        ]

    @property
    def commands(self):
        """Return all possible subproccess commands given the parameters from init."""
        all_commands = []
        passthrough_opts = {
            "id": str(self.job_id),
            "description": str(self.description),
            "run_type": None,
        }
        base_command = [
            str(self.exec_path.absolute()),
            str(self.infile_path.absolute()),
            "--method",
            str(self.method),
            "--output",
            str(self.output),
            "--runs",
            str(self.runs),
            "--output-chunks",
            str(self.output_chunks),
        ]
        if self.threshold is not None:
            base_command.append("--threshold")
            base_command.append(str(self.threshold))

        base_valgrind_opts = [
            "valgrind",
            "--time-stamp=yes",
            "--quiet",
        ]

        if self.base:
            my_pt = deepcopy(passthrough_opts)
            my_pt["run_type"] = "base"
            all_commands.append(
                tuple(itertools.chain(base_command, self._passthrough_args(my_pt)))
            )

        # Parse valgrind specific stuff
        if self.callgrind:
            out = str(self.callgrind)
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
            ]
            if self.valgrind_opts is not None:
                opts.extend(self.valgrind_opts)
            opts.append("--")

            my_pt = deepcopy(passthrough_opts)
            my_pt["run_type"] = "callgrind"
            all_commands.append(
                tuple(
                    itertools.chain(
                        base_valgrind_opts,
                        opts,
                        base_command,
                        self._passthrough_args(my_pt),
                    )
                )
            )
        if self.cachegrind:
            out = str(self.cachegrind)
            opts = [
                "--tool=cachegrind",
                f"--cachegrind-out-file={out}",
                "--cache-sim=yes",
                "--branch-sim=yes",
            ]
            if self.valgrind_opts is not None:
                opts.extend(self.valgrind_opts)
            opts.append("--")

            my_pt = deepcopy(passthrough_opts)
            my_pt["run_type"] = "cachegrind"
            all_commands.append(
                tuple(
                    itertools.chain(
                        base_valgrind_opts,
                        opts,
                        base_command,
                        self._passthrough_args(my_pt),
                    )
                )
            )
        if self.massif:
            out = str(self.massif)
            opts = [
                "--tool=massif",
                f"--massif-out-file={out}",
                "--stacks=yes",
            ]
            if self.valgrind_opts is not None:
                opts.extend(self.valgrind_opts)
            opts.append("--")

            my_pt = deepcopy(passthrough_opts)
            my_pt["run_type"] = "massif"

            all_commands.append(
                tuple(
                    itertools.chain(
                        base_valgrind_opts,
                        opts,
                        base_command,
                        self._passthrough_args(my_pt),
                    )
                )
            )

        return all_commands

    @property
    def cli(self) -> list[str]:
        """Return the raw CLI equivalent of the subprocess command(s)."""
        return [" ".join([str(i) for i in command]) for command in self.commands]

    def run(self, quiet=False, pbar=None):
        """Call the subprocess and run the job."""
        for i in self.commands:
            if not quiet:
                print(" ".join(i))
            elif pbar is not None:
                pbar.update()

            try:
                subprocess.run(i, capture_output=True, check=True)
            except subprocess.CalledProcessError as e:
                print("".join(["-"] * 80), end="\n\n")
                print("stdout:")
                print(e.stdout.decode())
                print("".join(["-"] * 80), end="\n\n")
                print("stderr:")
                print(e.stderr.decode())
                raise e

    def __len__(self) -> int:
        """Return the number of required subcommand calls."""
        return len(self.commands)


def parse_threshold_arg(user_input):
    """
    Parse the --threshold argument from the CLI.

    @param user_input: User input from the --threshold CLI argument.
    @returns: List of range of thresholds to test.
    """
    if not user_input:
        return range(1, 2, 1)

    result = []
    for arg in user_input:
        tokens = arg.split(",")
        tokens = [int(i) for i in tokens]
        if len(tokens) < 1 or len(tokens) > 3:
            raise ValueError(f"Invalid threshold value: {arg}")

        if len(tokens) == 1:
            result = itertools.chain(result, range(tokens[0], tokens[0] + 1))
        else:
            # Include endpoint
            tokens[1] += 1
            result = itertools.chain(result, range(*tokens))

    return list(set(result))


def parse_args(args):
    """Parse CLI args from docopt."""
    parsed = {}

    # Data dir
    parsed["data_dir"] = Path(args.get("<DATA_DIR>"))
    if not parsed["data_dir"].is_dir():
        raise NotADirectoryError("Invalid data directory")

    # Executable location
    parsed["exec"] = args.get("<EXEC>")
    parsed["exec"] = Path(parsed["exec"])
    if not parsed["exec"].is_file():
        loc = parsed["exec"].absolute()
        raise FileNotFoundError(f"Can't find executable: '{loc}'")

    # Populate the supported methods by calling the subprocess.
    valid_methods, _ = get_supported_methods(parsed["exec"])

    # Num jobs
    if args.get("--jobs") == "CPU":
        parsed["jobs"] = multiprocessing.cpu_count() - 1
        parsed["jobs"] = parsed["jobs"] if parsed["jobs"] > 0 else 1
    else:
        parsed["jobs"] = args.get("--jobs") or 1
        parsed["jobs"] = int(parsed["jobs"])
        if parsed["jobs"] <= 0:
            raise ValueError("Jobs must be >= 1")

    # Chuncked output
    parsed["output_chunks"] = int(args.get("--output-chunks") or 0)

    # Methods
    try:
        methods = args.get("--methods").rsplit(",")
        for i in methods:
            if i not in valid_methods:
                raise ValueError(f"Invalid method: '{i}'")
    except AttributeError:
        methods = valid_methods
    parsed["methods"] = methods

    # Progress bar
    parsed["progress"] = args.get("--progress")

    # Num runs
    parsed["runs"] = args.get("--runs", 1)
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
            parent = Path(f"./results/{now}_{platform.node()}/")
            parent.mkdir(exist_ok=True, parents=True)
            parsed["output"] = Path(parent, f"./output_{now}.csv")
    else:
        parsed["output"] = Path(args.get("--output"))

    parsed["arcc_partition"] = json.loads(args.get("--arcc-partition") or "{}")

    # Threshold
    parsed["threshold"] = parse_threshold_arg(args.get("--threshold"))

    # Valgrind options
    parsed["base"] = args.get("--base")
    parsed["callgrind"] = args.get("--callgrind")
    parsed["cachegrind"] = args.get("--cachegrind")
    parsed["massif"] = args.get("--massif")
    parsed["valgrind_opts"] = args.get("--valgrind-opt")

    if parsed["slurm"] is None:
        Path(parsed["output"].parent, "valgrind").mkdir(exist_ok=True)

    return parsed


class Scheduler:
    """Utility class allowing for easy job generation and scheduling across many threads."""

    def __init__(
        self,
        data_dir: Path,
        exec: Path,
        jobs: int,
        output_chunks: int,
        methods: list[str],
        output: Path,
        runs: int,
        slurm: Path,
        threshold: range,
        progress: bool,
        arcc_partition: dict,
        base: bool,
        callgrind: bool,
        cachegrind: bool,
        massif: bool,
        valgrind_opts: Optional[list[str]],
    ):
        """
        Define the base parameters.

        @param data_dir: Path to input data.
        @param exec: Path to executable.
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
        self.output_chunks = output_chunks
        self.methods = methods
        self.output = output
        self.runs = runs
        self.slurm = slurm
        self.threshold = threshold
        self.progress = progress
        self.arcc_partition = arcc_partition

        self.base = base
        self.callgrind = callgrind
        self.cachegrind = cachegrind
        self.massif = massif
        self.valgrind_opts = valgrind_opts

        if not any([self.callgrind, self.cachegrind, self.massif]):
            self.base = True

        self.job_queue: "deque[Job]" = deque()
        self.active_queue: "deque[Job]" = deque()

        self.valid_methods, self.threshold_methods = get_supported_methods(self.exec)

        self._gen_jobs()

    def _get_exec_version(self) -> str:
        """Call the process and parse the version output."""
        cmd = (self.exec, "--version")
        p = subprocess.Popen(cmd, stdout=subprocess.PIPE)
        stdout, _ = p.communicate()
        return stdout.decode()

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

            params = {
                "job_id": job_id,
                "exec_path": self.exec,
                "infile_path": f,
                "description": desc,
                "method": None,
                "runs": self.runs,
                "output": self.output,
                "threshold": 1,
                "output_chunks": self.output_chunks,
                "base": self.base,
                "callgrind": self.callgrind,
                "cachegrind": self.cachegrind,
                "massif": self.massif,
                "valgrind_opts": self.valgrind_opts,
            }
            for method in self.methods:
                params["method"] = method
                # Only methods in THRESHOLD_METHODS care about threshold value.
                if method in self.threshold_methods:
                    for thresh in self.threshold:
                        params["threshold"] = thresh
                        job = Job(**params)
                        self.job_queue.append(job)
                        job_id += 1
                        params["job_id"] = job_id
                else:
                    params["threshold"] = None
                    job = Job(**params)
                    self.job_queue.append(job)
                    job_id += 1
                    params["job_id"] = job_id

        # random.shuffle(self.job_queue)
        self.active_queue = self.job_queue

    def _restore_jobs(self):
        """Bring all the jbos from the last _gen_jobs() back into the active queue."""
        self.active_queue = self.job_queue

    def _worker(self):
        """Worker function for each thread."""
        while self.active_queue:
            job = self.active_queue.pop()
            job.run(quiet=self.progress, pbar=self.pbar)

    def run_jobs(self):
        """Run all the jobs on the local machine."""
        total_num_jobs = sum([len(job) for job in self.active_queue])
        print("===========================", file=sys.stderr)
        print(f"About to run {total_num_jobs} jobs", file=sys.stderr)
        print("===========================", file=sys.stderr)
        # time.sleep(3)
        print("Okay, lets do it!", file=sys.stderr)

        self.pbar = tqdm(total=total_num_jobs, disable=not self.progress)

        # Log system info
        write_info(
            self.output.parent,
            command=" ".join(sys.argv),
            data_details_path=Path(self.data_dir, "details.json"),
            concurrent=self.jobs,
            exec_path=self.exec,
            runs=self.runs,
            total_num_jobs=total_num_jobs,
            total_num_sorts=total_num_jobs * self.runs,
            arcc_partition=self.arcc_partition,
            base=self.base,
            callgrind=self.callgrind,
            massif=self.massif,
        )
        # Create my own process group
        try:
            os.setpgrp()
        except PermissionError:
            pass

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
        total_num_jobs = sum([len(job) for job in self.active_queue])
        write_info(
            self.slurm,
            command=" ".join(sys.argv),
            concurrent="slurm",
            data_details_path=Path(self.data_dir, "details.json"),
            exec_path=self.exec,
            runs=self.runs,
            total_num_jobs=total_num_jobs,
            total_num_sorts=total_num_jobs * self.runs,
            base=self.base,
            callgrind=self.callgrind,
            massif=self.massif,
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
                    size += len(job)
            print(f"{current_file}: {size}")
            index += 1


if __name__ == "__main__":
    args = parse_args(docopt(__doc__, version=VERSION))

    s = Scheduler(**args)
    if args["slurm"]:
        s.gen_slurm()
    else:
        s.run_jobs()
