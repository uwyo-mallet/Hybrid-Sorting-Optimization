#!/usr/bin/env python3
"""
Automatically dispatch sort jobs of varying inputs, methods, and threshold values.

This essentially serves as a job scheduler, with optional features to easily
dispatch slurm job array batches. Only use the native scheduling features if
running on a local multi-core machine.

Usage:
    jobs.py <DATA_DIR> [options]
    jobs.py <DATA_DIR> [options] (--threshold=THRESH ...)
    jobs.py -h | --help

Options:
    -h, --help               Show this help.
    -e, --exec=EXEC          Path to GB_QST executable.
    -m, --methods=METHODS    Comma seperated list of methods to use for sorters.
    -o, --output=FILE        Output JSON to save results from GB_QST.
    -r, --runs=N             Number of times to run the same input data.
    -t, --threshold=THRESH   Comma seperated range for threshold (min,max,[step])
                             including both endpoints, or a single value. Can be
                             specified multiple times.
"""

import itertools
import json
import subprocess
import sys
from collections import defaultdict, deque
from datetime import datetime
from pathlib import Path

from docopt import docopt

# HACK, maybe globalize info as a module?
sys.path.insert(0, str(Path(sys.path[0]).parent.absolute()))

from info import write_info

VERSION = "0.0.1"

THRESHOLD_METHODS = {
    "qsort_asm",
    "qsort_c",
    "qsort_c_swp",
    "qsort_cpp",
    "qsort_cpp_no_comp",
}
DATA_TYPES = {"ascending", "descending", "random", "single_num"}


def get_valid_methods(gb_qst_exe: Path, input_file: Path):
    """Call the GB_QST subprocess and capture all the currently implemented benchmarks."""
    command = (
        str(gb_qst_exe.absolute()),
        str(input_file.absolute()),
        "--benchmark_list_tests",
    )
    result = subprocess.run(command, capture_output=True, check=True)
    result = result.stdout.decode()

    # Careful, not very resiliant.
    methods = [i.rsplit("/")[1] for i in result.split()]
    return methods


def parse_threshold_arg(user_input):
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

    parsed["data_dir"] = Path(args.get("<DATA_DIR>"))
    if not parsed["data_dir"].is_dir():
        raise NotADirectoryError("Invalid data directory")

    parsed["exec"] = args.get("--exec") or "./build/GB_QST"
    parsed["exec"] = Path(parsed["exec"])

    if not parsed["exec"].is_file():
        raise FileNotFoundError(f"{parsed['exec']} does not exist.")

    valid_methods = get_valid_methods(parsed["exec"], parsed["data_dir"])
    try:
        methods = args.get("--methods").rsplit(",")
        methods = [i.lower() for i in methods]
        for i in methods:
            if i not in valid_methods:
                raise ValueError(f"Invalid method: {i}")
    except AttributeError:
        methods = valid_methods
    parsed["methods"] = methods

    now = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
    if args.get("--output_dir") is None:
        parsed["output_dir"] = Path(f"./results/gb_{now}/")
    parsed["output_dir"].mkdir(exist_ok=True, parents=True)

    parsed["threshold"] = parse_threshold_arg(args.get("--threshold"))

    return parsed


class Job:
    def __init__(
        self,
        job_id: int,
        exec_path: Path,
        infile_path: Path,
        methods: list[str],
        description: str,
        threshold: int,
        output_dir: Path,
    ):

        self.job_id = job_id
        self.exec_path = exec_path
        self.infile_path = infile_path
        self.methods = methods
        self.description = description
        self.threshold = threshold
        self.methods = methods
        self.output_dir = output_dir

    @property
    def command(self):
        """Return CLI command for this job."""

        out_file = self.output_dir / f"{self.job_id}.{self.description}"

        return (
            str(self.exec_path.absolute()),
            str(self.infile_path),
            "--threshold",
            str(self.threshold),
            f"--benchmark_filter={'|'.join(self.methods)}",
            f"--benchmark_context='description={self.description}','threshold={self.threshold}'",
            f"--benchmark_out={str(out_file.absolute())}",
            "--benchmark_out_format=json",
            "--benchmark_format=console",
        )

    @property
    def cli(self):
        """Return the raw CLI equivalent of the subprocess.Popen command."""
        return " ".join(self.command)

    def run(self, quiet=False):
        """Call the subprocess and run the job."""
        subprocess.run(self.command, capture_output=quiet, check=True)


class Scheduler:
    """Utility class allowing for easy job generation."""

    def __init__(
        self,
        data_dir: Path,
        exec: Path,
        methods: list[str],
        output_dir: Path,
        threshold: int,
    ):
        """
        Define the base parameters.

        @param data_dir: Path to input data.
        @param exec: Path to GB_QST executable.
        @param methods: List of all the methods to test.
        @param output_dir: Path to output folder where JSON files with all test results will be saved.
        @param threshold: Range of thresholds to test with each method.
        """
        self.data_dir = data_dir
        self.exec = exec
        self.methods = set(methods)
        self.output_dir = output_dir
        self.threshold = threshold

        self.job_queue: "deque[Job]" = deque()
        self.active_queue: "deque[Job]" = deque()

        self.output_dir.mkdir(exist_ok=True)

        self._gen_jobs()

    def _get_exec_version(self) -> str:
        """Call the QST process and parse the version output."""
        cmd = (self.exec, "--version")
        p = subprocess.Popen(cmd, stdout=subprocess.PIPE)
        stdout, _ = p.communicate()
        return stdout.decode()

    def _merge_json(self):
        # data = {i : {"benchmarks": []} for i in }
        data = defaultdict(list)
        for i in DATA_TYPES:
            for f in self.output_dir.rglob(f"*.{i}"):
                with open(f, "r") as json_file:
                    raw = json.load(json_file)
                    data[i] = raw["benchmarks"]

        with open(self.output_dir / "output.json", "w") as out_file:
            json.dump(data, out_file, indent=4)

    def _gen_jobs(self):
        files = self.data_dir.glob(r"**/*.gz")
        self.job_queue.clear()

        # Submit the threshold methods and normal methods as two separate
        # jobs. Threshold methods need several runs while normal methods
        # can be run just once.
        threshold_methods = self.methods & THRESHOLD_METHODS
        normal_methods = [i for i in self.methods if i not in THRESHOLD_METHODS]

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
                "methods": None,
                "output_dir": self.output_dir,
                "threshold": 1,
            }

            params["methods"] = threshold_methods
            for thresh in self.threshold:
                params["threshold"] = thresh
                job = Job(**params)
                self.job_queue.append(job)
                job_id += 1
                params["job_id"] = job_id

            if normal_methods:
                params["methods"] = normal_methods
                params["threshold"] = 42
                job = Job(**params)
                self.job_queue.append(job)
                job_id += 1

        self.active_queue = self.job_queue

    def run_jobs(self):
        """Run all the jobs on the local machine."""
        total_num_jobs = len(self.active_queue)
        print("===========================", file=sys.stderr)
        print(f"About to run {total_num_jobs} jobs", file=sys.stderr)
        print("===========================", file=sys.stderr)
        print("Okay, lets do it!", file=sys.stderr)

        # Log system info
        write_info(
            self.output_dir,
            command=" ".join(sys.argv),
            data_details_path=Path(self.data_dir, "details.json"),
            concurrent=1,
            qst_vers=self._get_exec_version(),
            runs=1,
            total_num_jobs=total_num_jobs,
            total_num_sorts=total_num_jobs,
        )

        while self.active_queue:
            job = self.active_queue.pop()
            job.run()

        self._merge_json()


if __name__ == "__main__":
    args = parse_args(docopt(__doc__, version=VERSION))
    s = Scheduler(**args)
    s.run_jobs()
