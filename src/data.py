#!/usr/bin/env python3
"""
Generate large amounts of testing data as fast as possible.

Usage:
    data.py evaluate FILE [options]
    data.py generate [options]
    data.py -h | --help

Options:
    -h, --help               Show this help.
    -f, --force              Overwrite existing.
    -o, --output=DIR         Output to save data (default: ./data/).
    -t, --threshold=THRESH   Comma seperated range for output data.
                             If no increment is specified, the minimum
                             value is used. Example: 500_000,20_000_000,500_000

Commands:
    evaluate            Evaluate an output CSV from QST run(s).
    generate            Generate testing data.
"""


import gzip
import multiprocessing
import random
import shutil
import sys
from multiprocessing import Pool, Process
from pathlib import Path

import numpy as np
from docopt import docopt

# Default thresholds
INCREMENT = 100_000
MIN_ELEMENTS = INCREMENT
MAX_ELEMENTS = 1_000_000


class DataGen:
    """
    An extremely optimized, parallel approach to generating large amounts of
    testing data as quickly as possible.
    """

    def __init__(self, output, minimum, maximum, increment, force):
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

        if self.base_path.exists() and not force:
            raise Exception("Output directory already exists")
        elif self.base_path.is_file():
            raise Exception("Output requires a directory, not a file")

        if force:
            shutil.rmtree(self.base_path, ignore_errors=True)

        self.create_dirs()

    def create_dirs(self):
        """Create all the directories for output files."""
        self.base_path.mkdir(parents=True, exist_ok=True)
        for dir in self.dirs.keys():
            real_path = Path(self.base_path, dir)
            real_path.mkdir()

    def ascending(self, output):
        for i, num_elements in enumerate(range(self.min, self.max, self.inc)):
            if i == 0:
                np.savetxt(
                    Path(output, f"{i}.dat.gz"),
                    np.arange(num_elements, dtype=np.int64),
                    fmt="%u",
                    delimiter="\n",
                    comments="",
                )
            else:
                prev = Path(output, f"{i - 1}.dat.gz")
                current = Path(output, f"{i}.dat.gz")
                shutil.copy(prev, current)

                with gzip.open(current, "a") as f:
                    data = np.arange(
                        num_elements - self.inc, num_elements, dtype=np.int64
                    ).tolist()
                    data = [str(i) for i in data]
                    data = "\n".join(data) + "\n"

                    f.write(data.encode())

    def descending(self, output):
        for i, num_elements in enumerate(range(self.min, self.max, self.inc)):
            if i == 0:
                np.savetxt(
                    Path(output, f"{i}.dat.gz"),
                    np.arange(num_elements - 1, -1, -1, dtype=np.int64),
                    fmt="%u",
                    delimiter="\n",
                    comments="",
                )
            else:
                prev = Path(output, f"{i - 1}.dat.gz")
                current = Path(output, f"{i}.dat.gz")

                with gzip.open(prev, "r") as prev_file:
                    prev_data = prev_file.read()

                with gzip.open(current, "wb") as f:
                    data = np.arange(
                        num_elements - 1,
                        num_elements - self.inc - 1,
                        -1,
                        dtype=np.int64,
                    ).tolist()

                    data = [str(i) for i in data]
                    data = "\n".join(data) + "\n"
                    data = data.encode()

                    f.write(data + prev_data)

    def _random(self, args):
        data = np.random.randint(self.max + 1, size=args[1], dtype=np.int64)
        np.savetxt(
            args[0],
            data,
            fmt="%u",
            delimiter="\n",
            comments="",
        )

    def random(self, output):
        sets = [
            (Path(output, f"{i}.dat.gz"), num_elements)
            for i, num_elements in enumerate(range(self.min, self.max, self.inc))
        ]
        pool = Pool(multiprocessing.cpu_count())
        pool.map(self._random, sets)
        pool.close()
        pool.join()

    def single_num(self, output):
        for i, num_elements in enumerate(range(self.min, self.max, self.inc)):
            if i == 0:
                data = np.empty(num_elements, dtype=np.int64)
                data.fill(42)

                np.savetxt(
                    Path(output, f"{i}.dat.gz"),
                    data,
                    fmt="%u",
                    delimiter="\n",
                    comments="",
                )
            else:
                prev = Path(output, f"{i - 1}.dat.gz")
                current = Path(output, f"{i}.dat.gz")
                shutil.copy(prev, current)

                with gzip.open(current, "a") as f:
                    data = np.empty(self.inc, dtype=np.int64)
                    data.fill(42)
                    data = data.tolist()

                    data = [str(i) for i in data]
                    data = "\n".join(data) + "\n"

                    f.write(data.encode())

    def generate(self):
        processes = []
        for k, v in self.dirs.items():
            output = Path(self.base_path, k)
            p = Process(target=v, args=(output,))
            p.start()

        for p in processes:
            p.join()

        with open(Path(self.base_path, "details.txt"), "w") as f:
            f.write(
                f"MIN_ELEMENTS: {self.min}\nMAX_ELEMENTS: {self.max}\nINCREMENT: {self.inc}"
            )


if __name__ == "__main__":
    args = docopt(__doc__)
    if args.get("evaluate"):
        print("[Deprecated]: Use the evaluate.ipynb jupyter notebook instead.")
    elif args.get("generate"):
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
                    increment = buf[3]
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

        d = DataGen(
            args.get("--output"), minimum, maximum, increment, args.get("--force")
        )
        d.generate()
