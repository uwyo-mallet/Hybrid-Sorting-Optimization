#!/usr/bin/env python3
"""
Plot statistics about the tests, optionally generate new data.
TODO: This needs to be heavily documented. Most of it is obscure.

Usage:
    evaluation.py evaluate FILE [options]
    evaluation.py generate [options]
    evaluation.py -h | --help

Options:
    -h, --help            Show this help.
    -f, --force           Overwrite existing.
    -o DIR, --output=DIR  Output to save data (default: ./data/).

Commands:
    evaluate              Evaluate an output CSV from QST.
    generate              Generate testing data.
"""

# TODO: Probably rename this whole file to something more logical

import csv
import random
import shutil
from collections import defaultdict
from datetime import datetime
from pathlib import Path

from docopt import docopt

INCREMENT = 50_000
MIN_ELEMENTS = INCREMENT
MAX_ELEMENTS = 1_000_000


# Cache sorted values, since there are so many to be made each time
sorted_cache = list(range(0, MAX_ELEMENTS))


def create_dirs(base_path, dirs):
    base_path.mkdir(parents=True, exist_ok=True)
    for dir in dirs:
        real_path = Path(base_path, dir)
        real_path.mkdir()


def sorted_(num_elements):
    global sorted_cache
    cache_len = len(sorted_cache)

    if num_elements > cache_len:
        rest = list(range(cache_len, num_elements))
        sorted_cache = sorted_cache[:num_elements].extend(rest)

    return sorted_cache[:num_elements]


def reverse_sorted(num_elements):
    return reversed(sorted_(num_elements))


def unsorted(num_elements):
    ret = []
    for _ in range(num_elements):
        ret.append(random.randint(0, num_elements))

    return tuple(ret)


def uniform(num_elements):
    ret = []
    for _ in range(num_elements):
        ret.append(int(random.uniform(0, num_elements)))

    return tuple(ret)


def evaluate(in_file):
    pass
    # with open(in_file, "r") as csv_file:
    #     csv_reader = csv.DictReader(csv_file)
    #     contents = list(csv_reader)

    # methods_and_type = defaultdict(defaultdict(list))
    # for row in contents:
    #     methods_and_type[row["Method"]].append(row)

    # from pprint import pprint

    # pprint(methods)


def generate(output, force=False):
    DIRS = {
        "sorted": sorted_,
        "reverse_sorted": reverse_sorted,
        "unsorted": unsorted,
        "uniform": uniform,
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
