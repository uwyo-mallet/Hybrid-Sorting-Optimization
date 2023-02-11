#!/usr/bin/env python3
"""Generate large amounts of testing data as fast as possible.

Multithreaded, super-fast, data generation manipulating syscalls to achieve
maximum performance!

Usage:
    data.py evaluate FILE [options]
    data.py generate [options]
    data.py -h | --help

Options:
    -h, --help               Show this help.
    -o, --output=DIR         Output to save data (default: ./data/).
    -t, --threshold=THRESH   Comma seperated range for output data.
                             If no increment is specified, the minimum
                             value is used. Example: 500_000,20_000_000,500_000
    --type=TYPE              Only generate one type of data

Commands:
    evaluate            Evaluate an output CSV from QST run(s).
    generate            Generate testing data.
"""
import gzip
import json
import random
import shutil
import sys
from multiprocessing import Process
from pathlib import Path

import numpy as np
from docopt import docopt

VERSION = "1.0.3"

# Default thresholds
INCREMENT = 100_000
MIN_ELEMENTS = INCREMENT
MAX_ELEMENTS = 1_000_000


class DataGen:
    """Utility class for generating lots of data really fast."""

    def __init__(self, output: Path, minimum: int, maximum: int, increment: int):
        """Initialize range and output parameters.

        :param output: Path to folder to output data.
        :param minimum: Minimum size data to create.
        :param maximum: Maximum size data to create.
        :param increment: Increments of data to create, should evenly divise maximum.

        :raises NotADirectoryError: Output requires a directory, not a file
        """
        random.seed()
        self.dirs = {
            "ascending": self.ascending,
            "descending": self.descending,
            "random": self.random,
            "single_num": self.single_num,
            "pipe_organ": self.pipe_organ,
        }

        self.min = minimum
        self.max = maximum
        self.inc = increment

        self.base_path = output or "./data/"
        self.base_path = Path(self.base_path)

        if self.base_path.is_file():
            raise NotADirectoryError("Output requires a directory, not a file")

        self._create_dirs()

    def _create_dirs(self):
        """Create all the directories for output files."""
        self.base_path.mkdir(parents=True, exist_ok=True)
        for d in self.dirs:
            real_path = Path(self.base_path, d)
            real_path.mkdir(exist_ok=True)

    def _copy_and_append(self, prev: Path, current: Path, data: np.array):
        """Copy a .gz file and append to the new file.

        This leverages the OS APIs to very quickly generate new data, which just
        extends the previous data.

        :param prev: Path to prev file.
        :param current: Path to new file to be created.
        :param data: Data to be appended to current.
        """
        shutil.copy(prev, current)
        with gzip.open(current, "a") as append_file:
            np.savetxt(append_file, data, fmt="%u", delimiter="\n", comments="")

    @staticmethod
    def _save(output: Path, data):
        """Save a np array as either txt or gz depending on the extension."""
        np.savetxt(output, data, fmt="%u", delimiter="\n", comments="")

    def _generic(self, output: Path, data):
        """Generic save routine for all other methods.

        :param output: Path to folder to save outputs (0.EXT, 1.EXT, ...).
        :param data: Iterable to write to disk.
        """
        self._save(Path(output, "0.gz"), data[: self.min])

        for i, n in enumerate(range(self.min, self.max - self.inc, self.inc), 1):
            prev = Path(output, f"{i - 1}.gz")
            current = Path(output, f"{i}.gz")
            to_write = data[n : n + self.inc]
            self._copy_and_append(prev, current, to_write)

    def ascending(self, output: Path):
        """Ascending data, 0, 1, 2, 3, 4."""
        data = np.arange(0, self.max, dtype=np.uint64)
        self._generic(output, data)

    def descending(self, output: Path):
        """Descending data, 4, 3, 2, 1, 0."""
        data = np.flip(np.arange(0, self.max, dtype=np.uint64))
        self._generic(output, data)

    def random(self, output: Path):
        """Random unbounded data."""
        data = np.random.randint(
            low=0,
            high=self.max + 1,
            size=self.max + 1,
            dtype=np.int64,
        )
        self._generic(output, data)

    def single_num(self, output: Path):
        """Repeated single number, 42."""
        data = np.empty(self.max, dtype=np.uint64)
        data.fill(42)
        self._generic(output, data)

    def pipe_organ(self, output: Path):
        """Ascending followed by descending data.

        Ex: 12321
        """
        for i, n in enumerate(range(self.min, self.max + self.inc, self.inc)):
            first = np.arange(0, n // 2, dtype=np.uint64)
            second = np.flip(first)
            # Handle the even/odd debacle
            if len(first) * 2 < n:
                data = np.concatenate([first, [first[-1]], second])
            else:
                data = np.concatenate([first, second])

            self._save(Path(output, f"{i}.gz"), data)

    def generate(self, t=None):
        """
        Generate data in parallel.

        :param t: Singular type of data to generate. If none, generate all types.
        """
        # Save some details about the data
        with open(Path(self.base_path, "details.json"), "w") as f:
            data = {
                "minimum": self.min,
                "maximum": self.max - self.inc,
                "increment": self.inc,
            }
            json.dump(data, f, indent=4)

        # Generate and write the actual data
        # There isn't a ton of optimization possible here.
        # We are largely IO bound. But if we run all the types in parrallel,
        # we can get a sizeable performance gain since the smaller arrays
        # are not IO bound. This brought the runtime from ~7 mins to ~3
        # for a dataset of threshold 5_000_000,100_000_000. This result is still
        # largely subjective and hardware specific.
        procs = []
        if t is None:
            for k, v in self.dirs.items():
                output = Path(self.base_path, k)

                if output.exists():
                    shutil.rmtree(output)
                output.mkdir()
                p = Process(target=v, args=(output,))
                procs.append(p)
                p.start()

            for p in procs:
                p.join()

            return

        if t not in self.dirs:
            raise ValueError("Invalid type")

        output = Path(self.base_path, t)
        self.dirs[t](output)


def main(output, minimum=None, maximum=None, increment=None, type=None):
    """Entrypoint."""
    if minimum is None and maximum is None and increment is None:
        minimum = MIN_ELEMENTS
        maximum = MAX_ELEMENTS
        increment = INCREMENT
    elif maximum is None and increment is None:
        maximum = minimum
        increment = minimum
    elif maximum < minimum or minimum < 0 or maximum < 0 or increment < 0:
        raise ValueError("Invalid threshold range")

    print("Generating data...", file=sys.stderr)
    print(f"Minimum: {minimum:,}", file=sys.stderr)
    print(f"Maximum: {maximum:,}", file=sys.stderr)
    print(f"Increment: {increment:,}", file=sys.stderr)

    d = DataGen(output, minimum, maximum, increment)
    return d.generate(type)


if __name__ == "__main__":
    args = docopt(__doc__, version=VERSION)
    if args.get("evaluate"):
        print("[Deprecated]: Use the evaluate.ipynb jupyter notebook instead.")
        exit(1)

    # Parse threshold and validate
    if args.get("--threshold") is not None:
        try:
            buf = args.get("--threshold").rstrip(",")
            buf = buf.split(",")
            if 0 <= len(buf) > 3:
                raise ValueError
            buf = [int(i) for i in buf]

            if len(buf) == 1:
                minimum = buf[0]
                maximum = buf[0]
                increment = buf[0]
            else:
                minimum = buf[0]
                maximum = buf[1]
                try:
                    increment = buf[2]
                except IndexError:
                    increment = minimum

        except ValueError as e:
            raise ValueError(f"Invalid threshold: {buf}") from e
    else:
        minimum = None
        maximum = None
        increment = None

    main(
        output=args.get("--output"),
        minimum=minimum,
        maximum=maximum,
        increment=increment,
        type=args.get("--type"),
    )
