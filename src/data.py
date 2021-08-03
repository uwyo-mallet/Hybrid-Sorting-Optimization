#!/usr/bin/env python3
"""
Generate large amounts of testing data as fast as possible.

Usage:
    data.py evaluate FILE [options]
    data.py generate [options]
    data.py validate DIR
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
    validate            Ensure all generated files are the correct length.
"""
import gzip
import json
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
    """Utility class for generating lots of data really fast."""

    def __init__(self, output: Path, minimum: int, maximum: int, increment: int):
        """
        Initialize range and output parameters.

        Args:
            output: Path to folder to output data.
            minimum: Minimum size data to create.
            maximum: Maximum size data to create.
            increment: Increments of data to create, must evenly divise maximum.

        Raises:
            NotADirectoryError: Output requires a directory, not a file
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

        self.create_dirs()

    def create_dirs(self):
        """Create all the directories for output files from self.dirs."""
        self.base_path.mkdir(parents=True, exist_ok=True)
        for d in self.dirs:
            real_path = Path(self.base_path, d)
            real_path.mkdir(exist_ok=True)

    def _append(self, prev: Path, current: Path, data: str):
        """
        Copy a .gz file and append to the new file assuming the data is text.

        Args:
            prev: Path to prev file.
            current: Path to new file to be created.
            data: Data to be appended to current.
        """
        shutil.copy(prev, current)
        with gzip.open(current, "a") as append_file:
            append_file.write(data.encode())

    @staticmethod
    def _save(output: Path, data):
        """Save an np array as either txt or gz depending on the extension."""
        np.savetxt(output, data, fmt="%u", delimiter="\n", comments="")

    def ascending(self, output: Path):
        """Generate ascending data and save to dir output."""
        for i, num_elements in enumerate(range(self.min, self.max, self.inc)):
            if i == 0:
                self._save(
                    Path(output, f"{i}.gz"),
                    np.arange(num_elements, dtype=np.int64),
                )
            else:
                prev = Path(output, f"{i - 1}.gz")
                current = Path(output, f"{i}.gz")

                data = np.arange(
                    num_elements - self.inc, num_elements, dtype=np.int64
                ).tolist()
                data = [str(i) for i in data]
                data = "\n".join(data) + "\n"

                self._append(prev, current, data)

    def descending(self, output: Path):
        """Generate descending data and save to dir output."""
        for i, num_elements in enumerate(range(self.min, self.max, self.inc)):
            if i == 0:
                self._save(
                    Path(output, f"{i}.gz"),
                    np.arange(num_elements - 1, -1, -1, dtype=np.int64),
                )
            else:
                prev = Path(output, f"{i - 1}.gz")
                current = Path(output, f"{i}.gz")

                # Prepend by reading the old file, then writing a new file with
                # the new data then the old data.
                with gzip.open(prev, "r") as prev_file:
                    prev_data = prev_file.read()

                with gzip.open(current, "wb") as current_file:
                    data = np.arange(
                        num_elements - 1,
                        num_elements - self.inc - 1,
                        -1,
                        dtype=np.int64,
                    ).tolist()

                    data = [str(i) for i in data]
                    data = "\n".join(data) + "\n"
                    data = data.encode()

                    current_file.write(data + prev_data)

    def _random(self, args):
        """Save random data to file. Helper for multiprocessing."""
        data = np.random.randint(
            low=self.min, high=self.max + 1, size=args[1], dtype=np.int64
        )
        np.savetxt(
            args[0],
            data,
            fmt="%d",
            delimiter="\n",
            comments="",
        )

    def random(self, output: Path):
        """Generate random data and save to dir output."""
        sets = [
            (Path(output, f"{i}.gz"), num_elements)
            for i, num_elements in enumerate(range(self.min, self.max, self.inc))
        ]
        pool = Pool(multiprocessing.cpu_count())
        pool.map(self._random, sets)
        pool.close()
        pool.join()

    def single_num(self, output: Path):
        """Generate single num data and save to dir output."""
        for i, num_elements in enumerate(range(self.min, self.max, self.inc)):
            if i == 0:
                data = np.empty(num_elements, dtype=np.int64)
                data.fill(42)
                self._save(Path(output, f"{i}.gz"), data)
            else:
                prev = Path(output, f"{i - 1}.gz")
                current = Path(output, f"{i}.gz")

                data = np.empty(self.inc, dtype=np.int64)
                data.fill(42)
                data = data.tolist()

                str_dat = "\n".join([str(i) for i in data]) + "\n"

                self._append(prev, current, str_dat)

    def generate(self, t=None):
        """Generate all types of data in parallel."""

        if t is None:
            processes = []
            for k, v in self.dirs.items():
                output = Path(self.base_path, k)
                # Remove existing
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
            p = Process(target=self.dirs[t], args=(output,))
            p.start()
            p.join()

        with open(Path(self.base_path, "details.json"), "w") as f:
            data = {
                "minimum": self.min,
                "maximum": self.max - self.inc,
                "increment": self.inc,
            }
            json.dump(data, f, indent=4)


class Validate:
    subdirs = [
        "ascending",
        "descending",
        "random",
        "single_num",
    ]

    def __init__(self, in_dir: Path):
        self.in_dir = in_dir

        details_path = Path(self.in_dir, "details.json")
        if not details_path.exists():
            raise FileNotFoundError("Missing deatils.json")

        with open(details_path, "r") as details_file:
            details = json.load(details_file)

        self.min = details["minimum"]
        self.max = details["maximum"]
        self.inc = details["increment"]

        if not self.in_dir.exists():
            raise NotADirectoryError("Missing input directory")

    def _validate_dir(self, path):
        files = sorted(list(path.glob("*.gz")), key=lambda x: int(x.stem))
        if len(files) != self.correct_num_files:
            print(f"[Warning]: Possibly missing files in {path}")

        correct_num_lines = self.min

        for f in files:
            num_lines = sum(1 for _ in gzip.open(f, "rb"))
            if num_lines != correct_num_lines:
                print(f"[Error]: {f}, {num_lines} != {correct_num_lines}")
            correct_num_lines += self.inc

    def validate(self):
        self.correct_num_files = int(self.max / self.inc)

        existing = []
        for subdir in self.subdirs:
            path = Path(self.in_dir, subdir)
            if not path.exists():
                print(f"[Warning]: Missing subdirectory: {subdir}")
                continue
            existing.append(path)

        pool = Pool(multiprocessing.cpu_count())
        pool.map(self._validate_dir, existing)
        pool.close()
        pool.join()


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
    elif args.get("validate"):
        v = Validate(Path(args.get("DIR")))
        v.validate()
