#!/usr/bin/env python3

from dataclasses import dataclass
from pathlib import Path

import numpy as np
import pandas as pd
import json

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
    "size": np.uint32,
    "threshold": np.uint64,
    "wall_nsecs": np.uint64,  # Elapsed wall time
    "user_nsecs": np.uint64,  # Elapsed user cpu time
    "system_nsecs": np.uint64,  # Elapsed system cpu time
    "id": np.uint32,
    "description": str,
    "base": np.uint8,
    "callgrind": np.uint8,
    "massif": np.uint8,
}
POST_PROCESS_COLUMNS = {
    "id": np.uint64,
    "input": str,
    "method": str,
    "description": str,
    "threshold": np.uint64,
    "size": np.uint64,
    "base": bool,
    "callgrind": bool,
    "massif": bool,
    "wall_nsecs": np.uint64,
    "wall_usecs": np.uint64,
    "wall_msecs": np.uint64,
    "wall_secs": np.uint64,
    "user_nsecs": np.uint64,
    "user_usecs": np.uint64,
    "user_msecs": np.uint64,
    "user_secs": np.uint64,
    "system_nsecs": np.uint64,
    "system_usecs": np.uint64,
    "system_msecs": np.uint64,
    "system_secs": np.uint64,
}


@dataclass
class Result:
    raw_df: pd.DataFrame
    stat_df: pd.DataFrame
    info: dict


def preprocess_csv(csv_file: Path):
    in_raw_parq = Path(csv_file.parent, csv_file.stem + ".parquet")
    in_stat_parq = Path(csv_file.parent, csv_file.stem + "_stat.parquet")

    raw_df = pd.read_csv(csv_file, dtype=RAW_COLUMNS, engine="c", memory_map=True)

    # Cleanup and add some additional columns
    raw_df["wall_usecs"] = raw_df["wall_nsecs"] / 1000
    raw_df["user_usecs"] = raw_df["user_nsecs"] / 1000
    raw_df["system_usecs"] = raw_df["system_nsecs"] / 1000

    raw_df["wall_msecs"] = raw_df["wall_usecs"] / 1000
    raw_df["user_msecs"] = raw_df["user_usecs"] / 1000
    raw_df["system_msecs"] = raw_df["system_usecs"] / 1000

    raw_df["wall_secs"] = raw_df["wall_msecs"] / 1000
    raw_df["user_secs"] = raw_df["user_msecs"] / 1000
    raw_df["system_secs"] = raw_df["system_msecs"] / 1000

    raw_df["base"] = raw_df["base"].astype(bool)
    raw_df["callgrind"] = raw_df["callgrind"].astype(bool)
    raw_df["massif"] = raw_df["massif"].astype(bool)

    # Reorder columns
    raw_df = raw_df[POST_PROCESS_COLUMNS.keys()]

    stats = (np.mean, np.std)
    stat_df = raw_df
    stat_df = stat_df.groupby(
        [
            "id",
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

    info_path = in_dir / "job_details.json"
    if info_path.is_file():
        info = json.loads(info_path.read_text())
    else:
        info = {}

    partition_path = in_dir / "partition"
    if partition_path.is_file():
        partition = partition_path.read_text()
    else:
        partition = None
    info["partition"] = partition
    info["actual_num_sorts"] = len(raw_df)

    return Result(raw_df, stat_df, info)


if __name__ == "__main__":
    load()
