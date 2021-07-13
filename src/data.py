#!/usr/bin/env python3
"""
Plot statistics about the tests, optionally generate new data.

Usage:
    data.py evaluate FILE [options]
    data.py generate [options]
    data.py -h | --help

Options:
    -h, --help            Show this help.
    -f, --force           Overwrite existing.
    -o DIR, --output=DIR  Output to save data (default: ./data/).

Commands:
    evaluate              Evaluate an output CSV from QST run(s).
    generate              Generate testing data.
"""

import csv
import random
import shutil
from collections import defaultdict
from datetime import datetime
from pathlib import Path
# REMOVE ME
from pprint import pprint

import matplotlib.pyplot as plt
import matplotlib.ticker as mtick
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
    """Return a list of sorted numbers from 0 to num_elements."""
    global sorted_cache
    cache_len = len(sorted_cache)

    if num_elements > cache_len:
        rest = list(range(cache_len, num_elements))
        sorted_cache = sorted_cache[:num_elements].extend(rest)

    return sorted_cache[:num_elements]


def descending(num_elements):
    """Return the reverse of sorted_."""
    return reversed(ascending(num_elements))


def random_nums(num_elements):
    """Generate num_elements random numbers in a list."""
    ret = []
    for _ in range(num_elements):
        ret.append(random.randint(0, num_elements))

    return tuple(ret)


def uniform(num_elements):
    """Generate num_elements random numbers with a uniform distribution."""
    ret = []
    for _ in range(num_elements):
        ret.append(int(random.uniform(0, num_elements)))

    return tuple(ret)


def single_num(num_elements):
    """Return a list of num_elements where every element is 42."""
    return [42] * num_elements


def evaluate(in_file):
    # Given a method and a type of a input data how does the runtime
    # scale as the input size increases?
    # Also, does this runtime change as the threshold value changes?

    # TODO: Overlay plots

    """
    Plots that we care about:
        1. How does runtime scale generally as the input size increases dramatically?
        2. How does threshold value affect runtime as the input size increases?
    """

    # Data preprocess / cleanup
    with open(in_file, "r") as csv_file:
        csv_reader = csv.DictReader(csv_file)
        contents = list(csv_reader)

    data = defaultdict(lambda: defaultdict(lambda: defaultdict(list)))
    # data = { method: { input_type: { threshold: { [pt1, pt2,...] }}}}
    for row in contents:
        data[row["Method"]][row["Description"]][row["Threshold"]].append(
            (row["Size"], row["Elapsed Time (microseconds)"])
        )

    # Addresses plot #1
    for method in data:
        for descrip in data[method]:
            thresh_text = ""
            for thresh in data[method][descrip]:
                if method == "qsort_c":
                    thresh_text = thresh

                dat = np.asarray(data[method][descrip][thresh])
                dat = dat.astype(int)
                # Sort to ensure line plots look nice
                dat = np.sort(dat, axis=0)

                x, y = dat.T
                # Convert from microseconds to milliseconds
                # TODO: At somepoint decide to use seconds?
                y = y / 1000

                fig, ax = plt.subplots()
                ax.scatter(x, y)
                ax.plot(x, y)

                ax.set(
                    xlabel="Input Size",
                    ylabel="Runtime (milliseconds)",
                    title=f"Method: {method} Input Type: {descrip}\nThresh:{thresh_text}",
                )
                ax.grid()
                ax.get_xaxis().set_major_formatter(
                    mtick.FuncFormatter(lambda x, p: format(int(x), ","))
                )

                fig.autofmt_xdate()
                plt.show()


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
