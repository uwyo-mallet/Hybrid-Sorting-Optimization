#!/usr/bin/env python3
"""
Automatically dispatch google benchmark tests varying inputs, and threshold values.

Usage:
    gb_jobs.py <DATA_DIR> [options]
    gb_jobs.py <DATA_DIR> [options] [--threshold=THRESH ...] [--benchmark-opt=ARG ...]
    gb_jobs.py <DATA_DIR> [options]
    gb_jobs.py -h | --help

Options:
    -h, --help               Show this help.
    -b, --benchmark-opt=ARG  Send a benchmark CLI option directly to the GB_QST
                               executable. Can be used multiple times.
    -e, --exec=EXEC          Path to GB_QST executable.
    -m, --methods=METHODS    Comma seperated list of methods to use for sorters.
    -o, --output=FILE        Output JSON to save results from GB_QST.
    -s, --slurm=DIR          Generate a batch of slurm data files in this dir.
    -t, --threshold=THRESH   Comma seperated range for threshold (min,max,[step])
                               including both endpoints, or a single value.
                               Can be specified multiple times.
"""

import itertools
import json
import shutil
import subprocess
import sys
from collections import defaultdict, deque
from datetime import datetime
from pathlib import Path

from docopt import docopt

# HACK, maybe globalize info as a module?
# TODO: Rework write info just for GB to capture more useful information?
sys.path.insert(0, str(Path(sys.path[0]).parent.absolute()))

from info import write_info

VERSION = "1.0.0"

# TODO: Find a way to have the QST executable dynamically generate this.
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


def get_valid_methods(gb_qst_exe: Path, input_file: Path) -> list[str]:
    """
    Call the GB_QST subprocess and capture all the currently implemented benchmarks.

    @param gb_qst_exe: Path to GB_QST executable.
    @param input_file: Valid data input file for GB_QST.
    @returns: List of methods currently supported by GB_QST.

    GB_QST requires a valid input file to access the help options of the google
    benchmark library. This could change in the future, but for now it is just
    easier to pass the input file anyways, since we validate that file as well.
    """
    command = (
        str(gb_qst_exe.absolute()),
        str(input_file.absolute()),
        "--benchmark_list_tests",
    )
    result = subprocess.run(command, capture_output=True, check=True)
    result = result.stdout.decode()

    # Will break if the test interface changes in GB_QST.
    methods = [i.rsplit("/")[1] for i in result.split()]
    return methods


def parse_threshold_arg(user_input) -> list[range]:
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

    # Benchmark Paramters
    parsed["benchmark_params"] = args["--benchmark-opt"]

    # GB_QST Executable
    parsed["exec"] = args.get("--exec") or "./build/GB_QST"
    parsed["exec"] = Path(parsed["exec"])
    if not parsed["exec"].is_file():
        raise FileNotFoundError(f"{parsed['exec']} does not exist.")

    # Methods
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

    # Slurm
    parsed["slurm"] = (
        Path(args.get("--slurm")) if args.get("--slurm") is not None else None
    )

    # Output directory
    now = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
    if args.get("--output_dir") is None:
        if parsed["slurm"] is not None:
            # Don't create an output folder if using slurm,
            # that is the shell script's responsibility.
            # Assume that the output should be in the folder I'm in.
            parsed["output_dir"] = Path(".")
        else:
            parsed["output_dir"] = Path(f"./results/gb_{now}/")
    else:
        parsed["output_dir"] = Path(args.get("--output_dir"))

    # Threshold
    parsed["threshold"] = parse_threshold_arg(args.get("--threshold"))

    return parsed


class Job:
    """Represent a single call to GB_QST."""

    def __init__(
        self,
        job_id: int,
        benchmark_params: list[str],
        exec_path: Path,
        infile_path: Path,
        methods: list[str],
        description: str,
        threshold: int,
        output_dir: Path,
    ):
        """
        Define the base parameters.

        @param job_id: Unique identifier for this 'run'.
        @param benchmark_params: List of additional CLI arguments to pass to GB_QST.
        @param exec_path: Path to GB_QST executable.
        @param infile_path: Path to input data file for GB_QST.
        @param threshold: Threshold to pass on to GB_QST.
        @param methods: Methods to pass on to GB_QST.
        @param output_dir: Output directory to save JSON benchmark results.
                           Output files are named as job_id.description.
                           This allows for easy globbing and post-processing.
        """
        self.job_id = job_id
        self.benchmark_params = benchmark_params
        self.exec_path = exec_path
        self.infile_path = infile_path
        self.methods = methods
        self.description = description
        self.threshold = threshold
        self.methods = methods
        self.output_dir = output_dir

    @property
    def command(self) -> list[str]:
        """Return subprocess command for this job."""
        out_file = self.output_dir / f"{self.job_id}.{self.description}"

        base_params = [
            str(self.exec_path.absolute()),
            str(self.infile_path),
            "--threshold",
            str(self.threshold),
            f"--benchmark_filter={'|'.join(self.methods)}",
            f"--benchmark_context=job_id={self.job_id},description={self.description},threshold={self.threshold}",
            f"--benchmark_out={str(out_file)}",
            "--benchmark_out_format=json",
            "--benchmark_format=console",
        ]
        base_params.extend(self.benchmark_params)

        return base_params

    @property
    def cli(self) -> str:
        """Return the raw CLI equivalent of the subprocess command."""
        return " ".join(self.command)

    def run(self, quiet=False) -> None:
        """Call the subprocess and run the job."""
        subprocess.run(self.command, capture_output=quiet, check=True)


class Scheduler:
    """Utility class allowing for easy job generation."""

    def __init__(
        self,
        data_dir: Path,
        benchmark_params: list[str],
        exec: Path,
        methods: list[str],
        output_dir: Path,
        slurm: Path,
        threshold: int,
    ):
        """
        Define the base parameters.

        @param data_dir: Path to input data.
        @param benchmark_params: List of additional CLI arguments to pass to GB_QST.
        @param exec: Path to GB_QST executable.
        @param methods: List of all the methods to test.
        @param output_dir: Path to output folder where JSON files with all test results will be saved.
        @param threshold: Range of thresholds to test with each method.
        """
        self.data_dir = data_dir
        self.benchmark_params = benchmark_params
        self.exec = exec
        self.methods = set(methods)
        self.output_dir = output_dir
        self.slurm = slurm
        self.threshold = threshold

        self.job_queue: "deque[Job]" = deque()

        self.output_dir.mkdir(exist_ok=True)

        self._threshold_job_ids = []
        self._gen_jobs()

    def _get_exec_version(self) -> str:
        """Call the QST process and parse the version output."""
        cmd = (self.exec, "--version")
        p = subprocess.Popen(cmd, stdout=subprocess.PIPE)
        stdout, _ = p.communicate()
        return stdout.decode()

    def _merge_json(self) -> None:
        """
        Merge all the relevant information from all the output files from GB_QST.

        Write to `output_dir/output.json`.

        == Output Format ==
        Dict of datatypes with a nested dict of threshold values
        (0 if not supported for those methods):

        {
          "ascending":
            {
                1: [Name:"qsort_c", "cpu_time": 1234, . . .],
                1: [Name:"qsort_c", "cpu_time": 1234, . . .],
                0: [Name: "insertion_sort", "cpu_time": 1234, . . .],
            }
          "descending":
            . . .
        }

        When written to JSON, keys and values are marshalled according to this
        RFC (https://datatracker.ietf.org/doc/html/rfc7159.html) and this
        (https://www.ecma-international.org/publications-and-standards/standards/ecma-404/)
        standard. Most notably, integer keys become strings.
        """
        data = defaultdict(lambda: defaultdict(list))

        for i in DATA_TYPES:
            for f in self.output_dir.rglob(f"*.{i}"):
                with open(f, "r") as json_file:
                    raw = json.load(json_file)
                    threshold = int(raw["context"]["threshold"])
                    job_id = int(raw["context"]["job_id"])

                    if job_id in self._threshold_job_ids:
                        data[i][threshold].extend(raw["benchmarks"])
                    else:
                        data[i][0].extend(raw["benchmarks"])

        with open(self.output_dir / "output.json", "w") as out_file:
            json.dump(data, out_file, indent=4, sort_keys=True)

    def _gen_jobs(self) -> None:
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
                "benchmark_params": self.benchmark_params,
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
                # Keep track of which job_ids are for threshold dependent methods.
                # Allows for easier post-processing.
                self._threshold_job_ids.append(job_id)
                job_id += 1
                params["job_id"] = job_id

            if normal_methods:
                params["methods"] = normal_methods
                params["threshold"] = 42
                job = Job(**params)
                self.job_queue.append(job)
                job_id += 1

    def run_jobs(self) -> None:
        """Run all the jobs."""
        total_num_jobs = len(self.job_queue)
        print("===========================", file=sys.stderr)
        print(f"About to run {total_num_jobs} jobs", file=sys.stderr)
        print("===========================", file=sys.stderr)
        print("Okay, lets do it!", file=sys.stderr)

        # Ensure output directory exists
        self.output_dir.mkdir(exist_ok=True, parents=True)

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

        while self.job_queue:
            job = self.job_queue.popleft()
            job.run()

        self._merge_json()

    def gen_slurm(self):
        """Create the slurm.d/ directory with all the necessary parameters."""
        if self.slurm.exists() and self.slurm.is_dir():
            shutil.rmtree(self.slurm)
        elif self.slurm.is_file():
            raise FileExistsError("Slurm output cannot be a file")
        self.slurm.mkdir(exist_ok=True, parents=True)

        total_num_jobs = len(self.job_queue)
        write_info(
            self.slurm,
            command=" ".join(sys.argv),
            concurrent="slurm",
            data_details_path=Path(self.data_dir, "details.json"),
            qst_vers=self._get_exec_version(),
            runs=1,
            total_num_jobs=total_num_jobs,
            total_num_sorts=total_num_jobs,
        )

        index = 0
        while self.job_queue:
            current_file = Path(self.slurm, f"{index}.dat")
            with open(current_file, "w") as slurm_file:
                size = 0
                while self.job_queue and size < MAX_BATCH:
                    job = self.job_queue.popleft()
                    slurm_file.write(job.cli + "\n")
                    size += 1
            print(f"{current_file}: {size}")
            index += 1


if __name__ == "__main__":
    raw_args = docopt(__doc__, version=VERSION)
    args = parse_args(raw_args)

    s = Scheduler(**args)
    if args["slurm"]:
        s.gen_slurm()
    else:
        s.run_jobs()
