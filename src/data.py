#!/usr/bin/env python3
"""
Generate large amounts of testing data as fast as possible.

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

# Default thresholds
INCREMENT = 100_000
MIN_ELEMENTS = INCREMENT
MAX_ELEMENTS = 1_000_000


class DataGen:
    """! Utility class for generating lots of data really fast."""

    def __init__(self, output: Path, minimum: int, maximum: int, increment: int):
        """!
        Initialize range and output parameters.

        @param output: Path to folder to output data.
        @param minimum: Minimum size data to create.
        @param maximum: Maximum size data to create.
        @param increment: Increments of data to create, should evenly divise maximum.

        @exception NotADirectoryError Output requires a directory, not a file
        """
        random.seed()
        self.dirs = {
            "ascending": self.ascending,
            "descending": self.descending,
            "random": self.random,
            "single_num": self.single_num,
        }

        self.min = minimum
        self.max = maximum + increment
        self.inc = increment

        self.base_path = output or "./data/"
        self.base_path = Path(self.base_path)

        if self.base_path.is_file():
            raise NotADirectoryError("Output requires a directory, not a file")

        self._create_dirs()

    def _create_dirs(self):
        """! Create all the directories for output files."""
        self.base_path.mkdir(parents=True, exist_ok=True)
        for d in self.dirs:
            real_path = Path(self.base_path, d)
            real_path.mkdir(exist_ok=True)

    def _copy_and_append(self, prev: Path, current: Path, data):
        """!
        Copy a .gz file and append to the new file assuming the data is an Iterable

        @param prev: Path to prev file.
        @param current: Path to new file to be created.
        @param data: Data to be appended to current.
        """
        data_str = ""
        for i in data:
            data_str += str(i) + "\n"

        shutil.copy(prev, current)
        with gzip.open(current, "a") as append_file:
            append_file.write(data_str.encode())

    @staticmethod
    def _save(output: Path, data):
        """! Save a np array as either txt or gz depending on the extension."""
        np.savetxt(output, data, fmt="%u", delimiter="\n", comments="")

    def _generic(self, output: Path, data):
        """!
        Generic save routine for all other methods.

        @param output: Path to folder to save outputs (0.EXT, 1.EXT, ...).
        @param data: Iterable to write to disk.
        """
        self._save(Path(output, "0.gz"), data[: self.inc])

        for i, n in enumerate(range(self.min, self.max - self.inc, self.inc), 1):
            prev = Path(output, f"{i - 1}.gz")
            current = Path(output, f"{i}.gz")

            self._copy_and_append(prev, current, data[n : n + self.inc])

    def ascending(self, output: Path):
        """! Ascending data, 0, 1, 2, 3, 4."""
        data = np.arange(self.min - self.inc, self.max - self.inc, dtype=np.uint64)
        self._generic(output, data)

    def descending(self, output: Path):
        """! Descending data, 4, 3, 2, 1, 0."""
        data = np.arange(
            self.max - self.inc - 1, self.min - self.inc - 1, -1, dtype=np.uint64
        )
        self._generic(output, data)

    def random(self, output: Path):
        """! Random data bounded by the min and max."""
        data = np.random.randint(
            low=self.min - self.inc,
            high=self.max - self.inc,
            size=self.max + 1,
            dtype=np.int64,
        )
        self._generic(output, data)

    def single_num(self, output: Path):
        """! The number 42 repeated."""
        data = np.empty(self.max + 1, dtype=np.int64)
        data.fill(42)
        self._generic(output, data)

    def generate(self, t=None):
        """!
        Generate data in parallel.

        @param t: Singular type of data to generate. If none, generate all types.
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
        processes = []
        if t is None:
            for k, v in self.dirs.items():
                output = Path(self.base_path, k)

                if output.exists():
                    shutil.rmtree(output)
                output.mkdir()
                p = Process(target=v, args=(output,))
                p.start()

            for p in processes:
                p.join()

        else:
            if t not in self.dirs:
                raise ValueError("Invalid type")

            output = Path(self.base_path, t)
            self.dirs[t](output)


if __name__ == "__main__":
    args = docopt(__doc__)
    if args.get("evaluate"):
        print("[Deprecated]: Use the evaluate.ipynb jupyter notebook instead.")

    if args.get("generate"):
        # Parse threshold and validate
        if args.get("--threshold") is not None:
            try:
                buf = args.get("--threshold").rsplit(",")
                if len(buf) != 2 and len(buf) != 3:
                    raise ValueError
                buf = [int(i) for i in buf]

                minimum = buf[0]
                maximum = buf[1]
                try:
                    increment = buf[2]
                except IndexError:
                    increment = minimum

            except ValueError as e:
                raise ValueError(f"Invalid threshold: {buf}") from e
        else:
            minimum = MIN_ELEMENTS
            maximum = MAX_ELEMENTS
            increment = INCREMENT
        if maximum <= minimum or minimum < 0 or maximum < 0 or increment < 0:
            raise ValueError("Invalid threshold range")

        print("Generating data...", file=sys.stderr)
        print(f"Minimum: {minimum:,}", file=sys.stderr)
        print(f"Maximum: {maximum:,}", file=sys.stderr)
        print(f"Increment: {increment:,}", file=sys.stderr)

        d = DataGen(args.get("--output"), minimum, maximum, increment)
        d.generate(args.get("--type"))
