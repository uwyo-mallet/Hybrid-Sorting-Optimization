#!/usr/bin/env python3
"""
Plot statistics about the tests, optionally generate new data.

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
                             value is used. Example: 500000,20000000,500000

Commands:
    evaluate            Evaluate an output CSV from QST run(s).
    generate            Generate testing data.
"""

import random
import shutil
from pathlib import Path

import numpy as np
from docopt import docopt

import sys

# Default thresholds
INCREMENT = 100_000
MIN_ELEMENTS = INCREMENT
MAX_ELEMENTS = 1_000_000


def create_dirs(base_path, dirs):
    """Create all the directories for output files."""
    base_path.mkdir(parents=True, exist_ok=True)
    for dir in dirs:
        real_path = Path(base_path, dir)
        real_path.mkdir()


def ascending(num_elements):
    """Return an array of ascending numbers from 0 to num_elements."""
    return cache[:num_elements]


def descending(num_elements):
    """Return the reverse of ascending."""
    return np.flip(ascending(num_elements))


def random_nums(num_elements):
    """Generate num_elements random numbers in a list."""
    ret = []
    for _ in range(num_elements):
        ret.append(random.randint(0, num_elements))

    return np.asarray(ret, dtype=np.int64)


def single_num(num_elements):
    """Return an array of length num_elements where every element is 42."""
    arr = np.empty(num_elements, dtype=np.int64)
    arr.fill(42)
    return arr


def evaluate(in_file):
    print("Use the evaluate.ipynb notebook instead, at least for now...")


def generate(
    output,
    minimum,
    maximum,
    increment,
    force=False,
):
    """Generate all random data and write to output folder."""
    DIRS = {
        "ascending": ascending,
        "descending": descending,
        "random": random_nums,
        "single_num": single_num,
    }
    random.seed()

    base_path = output or "data/"
    base_path = Path(base_path)

    if base_path.exists() and not force:
        raise Exception("Output directory already exists")
    elif base_path.is_file():
        raise Exception("Output requires a directory, not a file")

    if force:
        shutil.rmtree(base_path, ignore_errors=True)

    create_dirs(base_path, DIRS.keys())
    for k, v in DIRS.items():
        dest_folder = Path(base_path, k)
        for i, num_elements in enumerate(
            range(minimum, maximum + increment, increment)
        ):
            dest_filename = Path(dest_folder, f"{i}.dat.gz")
            np.savetxt(
                dest_filename,
                v(num_elements),
                fmt="%u",
                delimiter="\n",
                comments="",
            )

    with open(Path(base_path, "details.txt"), "w") as f:
        f.write(
            f"MIN_ELEMENTS: {MIN_ELEMENTS}\nMAX_ELEMENTS: {MAX_ELEMENTS}\nINCREMENT: {INCREMENT}"
        )


if __name__ == "__main__":
    args = docopt(__doc__)
    if args.get("evaluate"):
        evaluate(args.get("FILE"))
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

        cache = np.arange(0, maximum, 1, dtype=np.int64)
        generate(args.get("--output"), minimum, maximum, increment, args.get("--force"))
