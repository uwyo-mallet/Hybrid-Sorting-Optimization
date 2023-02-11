#!/usr/bin/env python3

import shutil
import sys
from pathlib import Path

import numpy as np
import pytest

# HACK: There really isn't a better way to do this just for testing IMO.
sys.path.insert(0, "./src")
import data

np.set_printoptions(linewidth=sys.maxsize)

OUTPUT_DIR = Path("./.test_tmp")
test_params = [
    # min, max, inc
    (2, None, None),
    (10, None, None),
    (100, None, None),
    (1000, None, None),
]
for i in range(3, 200, 7):
    for j in range(i, 200, 7):
        for k in range(3, 25, 7):
            test_params.append((i, j, k))


def setup_function():
    if OUTPUT_DIR.is_dir():
        shutil.rmtree(OUTPUT_DIR)
    OUTPUT_DIR.mkdir()


def teardown_function():
    shutil.rmtree(OUTPUT_DIR)


def is_ascending(arr: np.array):
    for i in range(0, len(arr) - 1):
        if arr[i] > arr[i + 1]:
            raise ValueError(f"Array is in ascending order, {arr}")


def is_descending(arr: np.array):
    for i in range(0, len(arr) - 1):
        if arr[i] < arr[i + 1]:
            raise ValueError(f"Array is in descending order, {arr}")


def is_single_num(arr: np.array):
    if not np.ma.allequal(arr, 42):
        raise ValueError(f"Array contains non-expected value, {arr}")


def is_pipe_organ(arr: np.array):
    n = len(arr)
    first, second = np.array_split(arr, 2)
    if len(first) != len(second):
        first = first[:-1]

    if (first != np.flip(second)).all():
        raise ValueError(f"Array contains non pipe organ sequence, {arr}")


validator_funcs = {
    "ascending": is_ascending,
    "descending": is_descending,
    "random": lambda _: None,
    "single_num": is_single_num,
    "pipe_organ": is_pipe_organ,
}


@pytest.mark.parametrize("min,max,inc", test_params)
def test_generate(min: int, max: int, inc: int):
    if max is None and inc is None:
        max = min
        inc = min

    data.main(OUTPUT_DIR, min, max, inc)

    lengths = list(range(min, max, inc))

    for i, l in enumerate(lengths):
        for t in validator_funcs.keys():
            # Validate data file existing
            path = OUTPUT_DIR / t / f"{i}.gz"
            assert path.is_file()

            # Validate length
            d = np.loadtxt(path, ndmin=1, dtype=np.uint64)
            assert len(d) == l, f"Length check: {str(path)}"

            # Validate contents
            validator_funcs[t](d)
