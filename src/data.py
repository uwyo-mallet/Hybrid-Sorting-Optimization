#!/usr/bin/env python3
"""
Plot statistics about the tests, optionally generate new data.

Usage:
    data.py evaluate FILE [options]
    data.py generate [options]
    data.py -h | --help

Options:
    -h, --help          Show this help.
    -f, --force         Overwrite existing.
    -o, --output=DIR    Output to save data (default: ./data/).

Commands:
    evaluate            Evaluate an output CSV from QST run(s).
    generate            Generate testing data.
"""

import random
import shutil
from pathlib import Path

import numpy as np
from docopt import docopt

# Linear increase in number of numbers
INCREMENT = 50_000
MIN_ELEMENTS = INCREMENT
MAX_ELEMENTS = 1_000_000

# Number of repeated lists for unsorted and uniform.
# TODO: Greatly increase the number of samples I am testing.
NUM_REPEATS = 10


# Cache sorted values, since there are so many to be made each time
sorted_cache = list(range(0, MAX_ELEMENTS))


def create_dirs(base_path, dirs):
    """Create all the directories for output files."""
    base_path.mkdir(parents=True, exist_ok=True)
    for dir in dirs:
        real_path = Path(base_path, dir)
        real_path.mkdir()


def ascending(num_elements):
    """Return an array of ascending numbers from 0 to num_elements."""
    return np.arange(0, num_elements, 1)


def descending(num_elements):
    """Return the reverse of ascending."""
    return reversed(ascending(num_elements))


def random_nums(num_elements):
    """Generate num_elements random numbers in a list."""
    ret = []
    for _ in range(num_elements):
        ret.append(random.randint(0, num_elements))

    return tuple(ret)


def single_num(num_elements):
    """Return an array of length num_elements where every element is 42."""
    arr = np.empty(num_elements)
    arr.fill(42)
    return arr


def evaluate(in_file):
    print("Use the evaluate.ipynb notebook instead, at least for now...")


def generate(output, force=False):
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
        shutil.rmtree(base_path)

    create_dirs(base_path, DIRS.keys())
    for k, v in DIRS.items():
        dest_folder = Path(base_path, k)
        for i, num_elements in enumerate(range(MIN_ELEMENTS, MAX_ELEMENTS, INCREMENT)):
            dest_filename = Path(dest_folder, f"{i}.dat")

            f = open(dest_filename, "w")
            for row in v(num_elements):
                f.write(str(row) + "\n")
            f.close()


if __name__ == "__main__":
    args = docopt(__doc__)
    if args.get("evaluate"):
        evaluate(args.get("FILE"))
    elif args.get("generate"):
        generate(args.get("--output"), args.get("--force"))
