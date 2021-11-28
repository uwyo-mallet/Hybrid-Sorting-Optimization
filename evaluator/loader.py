#!/usr/bin/env python3

import json
from dataclasses import dataclass
from pathlib import Path

import numpy as np
import pandas as pd

IGNORE_CACHE = True
JOB_DETAILS_FILE = "job_details.json"
PARTITION_FILE = "partition"
RESULTS_DIR = Path("./results")

THRESHOLD_METHODS = {
    "qsort_asm",
    "qsort_c",
    "qsort_c_swp",
    "qsort_cpp",
    "qsort_cpp_no_comp",
}
GRAPH_ORDER = (
    "random",
    "ascending",
    "descending",
    "single_num",
)
UNITS = {
    "seconds": "_secs",
    "milliseconds": "_msecs",
    "microseconds": "_usecs",
    "nanoseconds": "_nsecs",
}
CLOCKS = (
    "wall",
    "user",
    "system",
)
DATA_TYPES = (
    "base",
    "callgrind",
    "massif",
)
RAW_COLUMNS = {
    "method": str,
    "input": str,
    "size": np.uint64,
    "threshold": np.uint64,
    "wall_nsecs": np.uint64,  # Elapsed wall time
    "user_nsecs": np.uint64,  # Elapsed user cpu time
    "system_nsecs": np.uint64,  # Elapsed system cpu time
    "description": str,
    "base": bool,
    "callgrind": bool,
    "massif": bool,
}
POST_PROCESS_COLUMNS = [
    "input",
    "method",
    "description",
    "threshold",
    "size",
    "base",
    "callgrind",
    "massif",
    "wall_nsecs",
    "wall_usecs",
    "wall_msecs",
    "wall_secs",
    "user_nsecs",
    "user_usecs",
    "user_msecs",
    "user_secs",
    "system_nsecs",
    "system_usecs",
    "system_msecs",
    "system_secs",
]
CLOCK_COLUMNS = POST_PROCESS_COLUMNS[9:]


@dataclass
class Result:
    raw_df: pd.DataFrame
    stat_df: pd.DataFrame
    info: dict


def preprocess_csv(csv_file: Path):
    in_raw_parq = Path(csv_file.parent, csv_file.stem + ".parquet")
    in_stat_parq = Path(csv_file.parent, csv_file.stem + "_stat.parquet")

    raw_df = pd.read_csv(
        csv_file,
        usecols=RAW_COLUMNS,
        dtype=RAW_COLUMNS,
        engine="c",
        memory_map=True,
    )

    # Save some memory
    raw_df["input"] = raw_df["input"].astype("category")
    raw_df["method"] = raw_df["method"].astype("category")
    raw_df["description"] = raw_df["description"].astype("category")

    raw_df["base"] = raw_df["base"].astype(bool)
    raw_df["callgrind"] = raw_df["callgrind"].astype(bool)
    raw_df["massif"] = raw_df["massif"].astype(bool)

    # Downcast whenever possible
    uint_columns = ["threshold", "size", "wall_nsecs", "user_nsecs", "system_nsecs"]
    raw_df[uint_columns] = raw_df[uint_columns].apply(
        pd.to_numeric, downcast="unsigned"
    )

    units = list(reversed(UNITS.values()))
    for clock in CLOCKS:
        for prev, current in zip(units, units[1:]):
            prev_index = "".join((clock, prev))
            current_index = "".join((clock, current))
            raw_df[current_index] = raw_df[prev_index].div(1000)
            raw_df[current_index] = pd.to_numeric(
                raw_df[current_index], downcast="float"
            )

    # Reorder columns
    raw_df = raw_df[POST_PROCESS_COLUMNS]

    stats = (np.mean, np.std)
    stat_df = raw_df
    stat_df = stat_df.groupby(
        [
            "input",
            "method",
            "description",
            "size",
            "threshold",
            "base",
            "callgrind",
            "massif",
        ]
    ).agg(
        {
            "wall_nsecs": stats,
            "user_nsecs": stats,
            "system_nsecs": stats,
            "wall_msecs": stats,
            "user_msecs": stats,
            "system_msecs": stats,
            "wall_secs": stats,
            "user_secs": stats,
            "system_secs": stats,
        }
    )

    stat_df = stat_df.reset_index()
    stat_df.sort_values(["size"], inplace=True)

    # Save to disk as cache
    raw_df.to_parquet(in_raw_parq)
    stat_df.to_parquet(in_stat_parq)

    return raw_df, stat_df


def load(in_dir=None):
    if in_dir is None:
        try:
            dirs = sorted(list(RESULTS_DIR.iterdir()))
            in_dir = dirs[-1]
        except IndexError as e:
            raise FileNotFoundError("No result directories found") from e

    parqs = tuple(in_dir.glob("*.parquet"))
    csvs = tuple(in_dir.glob("*.csv"))

    in_raw_parq = None
    in_stat_parq = None
    in_csv = None

    if len(csvs):
        in_csv = Path(csvs[0])

    if IGNORE_CACHE:
        if in_csv is None:
            raise FileNotFoundError("CSV data files found.")
        raw_df, stat_df = preprocess_csv(in_csv)
    else:
        for i in parqs:
            if str(i).endswith("_stat.parquet"):
                in_stat_parq = Path(i)
            elif str(i).endswith(".parquet"):
                in_raw_parq = Path(i)
            if in_raw_parq is not None and in_stat_parq is not None:
                # Cache files found, load them and move on.
                raw_df = pd.read_parquet(in_raw_parq)
                stat_df = pd.read_parquet(in_stat_parq)
                break
        else:
            if in_csv is None:
                raise FileNotFoundError("Neither CSV nor parquet data files found.")
            # CSV is found, parquets not found, generate them to serve as a cache.
            raw_df, stat_df = preprocess_csv(in_csv)

    info_path = in_dir / JOB_DETAILS_FILE
    if info_path.is_file():
        info = json.loads(info_path.read_text())
    else:
        info = {}

    partition_path = in_dir / PARTITION_FILE
    if partition_path.is_file():
        partition = partition_path.read_text()
    else:
        partition = None
    info["partition"] = partition
    info["actual_num_sorts"] = len(raw_df)

    return Result(raw_df, stat_df, info)


if __name__ == "__main__":
    # Basic memory benchmarking
    # TODO: Try to reduce memory usages more by chunking?
    # TODO: Or try making data post-processing a singular step, then only load the stat df.
    res = load()
    print(res.raw_df.memory_usage(deep=True))
    print(res.raw_df.dtypes)
    print(res.stat_df.memory_usage(deep=True))
    print(res.stat_df.dtypes)
