#!/usr/bin/env python3
"""
Plot statistics about the tests, optionally generate new data.
TODO: This needs to be heavily documented. Most of it is obscure.

Usage:
    evaluation.py [-o OUTPUT_DIR | --output OUTPUT_DIR]
"""

import random
from datetime import datetime
from pathlib import Path

from docopt import docopt

INCREMENT = 50_000
MIN_ELEMENTS = INCREMENT
MAX_ELEMENTS = 1_000_000


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


def lognorm_variate(num_elements):
    pass


def normal_variate(num_elements):
    pass


def gamma_variate(num_elements):
    pass


def expo_variate(num_elements):
    pass


def gauss(num_elements):
    pass


if __name__ == "__main__":

    DIRS = {
        "sorted": sorted_,
        "reverse_sorted": reverse_sorted,
        "unsorted": unsorted,
        "uniform": uniform,
    }
    random.seed()

    args = docopt(__doc__)

    now = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
    base_path = args["OUTPUT_DIR"] or "data/"
    base_path = Path(base_path)

    if args["--output"]:
        if not base_path.exists() or not base_path.is_dir():
            raise Exception("Invalid output")

    base_path = Path(base_path, now)
    create_dirs(base_path, DIRS.keys())
    for k, v in DIRS.items():
        dest_folder = Path(base_path, k)
        for i, num_elements in enumerate(range(MIN_ELEMENTS, MAX_ELEMENTS, INCREMENT)):
            dest_filename = Path(dest_folder, f"{i}.txt")
            f = open(dest_filename, "w")
            for row in v(num_elements):
                f.write(str(row) + "\n")
            f.close()
