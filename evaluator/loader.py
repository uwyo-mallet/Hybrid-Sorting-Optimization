#!/usr/bin/env python3

import json
import re
import subprocess
from dataclasses import dataclass
from pathlib import Path
from typing import Union

import numpy as np
import pandas as pd


IGNORE_CACHE = False
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
    "cachegrind",
    "massif",
)
RAW_COLUMNS = {
    "id": np.uint64,
    "method": str,
    "input": str,
    "size": np.uint64,
    "threshold": np.uint64,
    "wall_nsecs": np.uint64,  # Elapsed wall time
    "user_nsecs": np.uint64,  # Elapsed user cpu time
    "system_nsecs": np.uint64,  # Elapsed system cpu time
    "description": str,
    "run_type": str,
}
POST_PROCESS_COLUMNS = [
    "id",
    "input",
    "method",
    "description",
    "threshold",
    "size",
    "run_type",
    "wall_nsecs",
    "user_nsecs",
    "system_nsecs",
    "wall_secs",
    "user_secs",
    "system_secs",
]


@dataclass
class Result:
    raw_df: pd.DataFrame
    stat_df: pd.DataFrame
    info: dict


def downcast(df, cols, cast="unsigned"):
    df[cols] = df[cols].apply(pd.to_numeric, downcast=cast)
    return df


def load_cachegrind(df, valgrind_dir: Path):
    CACHEGRIND_COLS = [
        "Ir",
        "I1mr",
        "ILmr",
        "Dr",
        "D1mr",
        "DLmr",
        "Dw",
        "D1mw",
        "DLmw",
        "Bc",
        "Bcm",
        "Bi",
        "Bim",
    ]
    pattern = re.compile(r"(\d+,?\d*)(?:\s+)")
    df = df.drop_duplicates(subset=["id"], keep="first")
    df = df[df["run_type"] == "cachegrind"]

    df = df.set_index("id")
    cachegrind_df = pd.DataFrame(columns=CACHEGRIND_COLS, dtype=np.uint64)
    for row in df.itertuples():
        i = row.Index
        method = row.method
        cachegrind_file = valgrind_dir / f"{i}_cachegrind.out"

        p = subprocess.run(
            ["cg_annotate", str(cachegrind_file.absolute())],
            capture_output=True,
            text=True,
        )
        lines = p.stdout.split("\n")[10:]
        for line in lines:
            line = line.lower()
            if method in line:
                tokens = [int(i.replace(",", "")) for i in pattern.findall(line)]
                if len(tokens) != len(CACHEGRIND_COLS):
                    print("[Warning]: Malformed line found, skipping.")
                    continue

                stats = dict(zip(CACHEGRIND_COLS, tokens))
                cachegrind_df.loc[i] = stats
                break

    cachegrind_df = downcast(cachegrind_df, CACHEGRIND_COLS)
    df = df.join(cachegrind_df)
    return df


def preprocess_csv(csv_file: Path, valgrind_dir: Union[None, Path] = None):
    in_raw_parq = Path(csv_file.parent, csv_file.stem + ".parquet")
    in_stat_parq = Path(csv_file.parent, csv_file.stem + "_stat.parquet")

    raw_df = pd.read_csv(
        csv_file,
        usecols=RAW_COLUMNS,
        dtype=RAW_COLUMNS,
        engine="c",
    )

    # Save some memory
    raw_df["input"] = raw_df["input"].astype("category")
    raw_df["method"] = raw_df["method"].astype("category")
    raw_df["description"] = raw_df["description"].astype("category")
    raw_df["run_type"] = raw_df["run_type"].astype("category")

    raw_df["wall_secs"] = raw_df["wall_nsecs"] / 1_000_000_000
    raw_df["user_secs"] = raw_df["user_nsecs"] / 1_000_000_000
    raw_df["system_secs"] = raw_df["system_nsecs"] / 1_000_000_000

    # Downcast whenever possible
    uint_columns = [
        "id",
        "threshold",
        "size",
        "wall_nsecs",
        "user_nsecs",
        "system_nsecs",
    ]
    raw_df = downcast(raw_df, uint_columns)

    # Reorder columns
    raw_df = raw_df[POST_PROCESS_COLUMNS]

    stats = (np.mean, np.std)
    stat_df = raw_df
    stat_df = stat_df.groupby(
        [
            "input",
            "method",
            "description",
            "threshold",
            "size",
            "run_type",
        ]
    ).agg(
        {
            "wall_nsecs": stats,
            "user_nsecs": stats,
            "system_nsecs": stats,
            "wall_secs": stats,
            "user_secs": stats,
            "system_secs": stats,
        },
        skipna=True,
    )
    stat_df = stat_df.reset_index()
    stat_df.sort_values(["size"], inplace=True)

    if valgrind_dir is not None:
        raw_df = load_cachegrind(raw_df, valgrind_dir)

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
        valgrind_dir = in_csv.parent / "valgrind"
        valgrind_dir = valgrind_dir if valgrind_dir.exists() else None
        raw_df, stat_df = preprocess_csv(in_csv, valgrind_dir)
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
            valgrind_dir = in_csv.parent / "valgrind"
            valgrind_dir = valgrind_dir if valgrind_dir.exists() else None
            raw_df, stat_df = preprocess_csv(in_csv, valgrind_dir)

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
