"""Loader for QST results."""
import json
import re
import subprocess
from pathlib import Path
from typing import Optional

import numpy as np
import pandas as pd
import sys

from .generics import QST_RESULTS_DIR

IGNORE_CACHE = False
JOB_DETAILS_FILE = "job_details.json"
PARTITION_FILE = "partition"

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


def downcast(df: pd.DataFrame, cols: list[str], cast="unsigned"):
    """Convert columns to their smallest possible datatype to save some memory."""
    df[cols] = df[cols].apply(pd.to_numeric, downcast=cast)
    return df


def load_cachegrind(df, valgrind_dir: Optional[Path]):
    # TODO: Handle a different method name in CSV vs C source code.
    base_cols = [
        "method",
        "description",
        "threshold",
        "size",  # Necessary?
    ]
    cachegrind_df = pd.DataFrame(columns=base_cols + CACHEGRIND_COLS, dtype=np.uint64)
    if valgrind_dir is None:
        return cachegrind_df

    pattern = re.compile(r"(\d+,?\d*)(?:\s+)")

    df = df[df["run_type"] == "cachegrind"]
    df = df.drop_duplicates(subset=["id"], keep="first")
    df = df.set_index("id")

    for row in df.itertuples():
        i = row.Index
        method = row.method
        cachegrind_file = valgrind_dir / f"{i}_cachegrind.out"

        p = subprocess.run(
            [
                "cg_annotate",
                "--threshold=0",
                "--auto=no",
                str(cachegrind_file.absolute()),
            ],
            capture_output=True,
            text=True,
        )
        lines = p.stdout.split("\n")
        for line in lines:
            line = line.lower()
            if ("time" in line or method in line) and "command" not in line:
                tokens = [int(i.replace(",", "")) for i in pattern.findall(line)]
                if len(tokens) != len(CACHEGRIND_COLS):
                    print("[Warning]: Malformed line found, skipping.", file=sys.stderr)
                    continue

                data = {k: v for k, v in row._asdict().items() if k in base_cols}
                data.update(dict(zip(CACHEGRIND_COLS, tokens)))
                cachegrind_df.loc[i] = data
                break
        else:
            print(f"[Warning]: Could not locate method: {method}", file=sys.stderr)

    cachegrind_df = downcast(cachegrind_df, CACHEGRIND_COLS)
    return cachegrind_df


def preprocess_csv(csv_file: Path, valgrind_dir: Optional[Path] = None):
    in_raw_parq = Path(csv_file.parent, csv_file.stem + "_raw.parquet")
    in_stat_parq = Path(csv_file.parent, csv_file.stem + "_stat.parquet")
    in_cg_parq = Path(csv_file.parent, csv_file.stem + "_cachegrind.parquet")

    raw_df = pd.read_csv(
        csv_file,
        usecols=RAW_COLUMNS,
        dtype=RAW_COLUMNS,
        engine="c",
    )

    raw_df["wall_secs"] = raw_df["wall_nsecs"] / 1_000_000_000
    raw_df["user_secs"] = raw_df["user_nsecs"] / 1_000_000_000
    raw_df["system_secs"] = raw_df["system_nsecs"] / 1_000_000_000

    # Downcast whenever possible to save memory.
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
    )
    stat_df = stat_df.reset_index()
    stat_df = stat_df.sort_values(["size"])

    cachegrind_df = load_cachegrind(raw_df, valgrind_dir)

    # Save to disk as cache
    raw_df.to_parquet(in_raw_parq)
    stat_df.to_parquet(in_stat_parq)
    cachegrind_df.to_parquet(in_cg_parq)

    return raw_df, stat_df, cachegrind_df


def load_without_cache(in_csv: Path):
    valgrind_dir = in_csv.parent / "valgrind"
    return preprocess_csv(in_csv, valgrind_dir if valgrind_dir.exists() else None)


def load(in_dir=None):
    if in_dir is None:
        try:
            dirs = sorted(list(QST_RESULTS_DIR.iterdir()))
            in_dir = dirs[-1]
        except IndexError as e:
            raise FileNotFoundError("No result directories found") from e

    parqs = tuple(in_dir.glob("*.parquet"))
    csvs = tuple(in_dir.glob("*.csv"))

    in_raw_parq = None
    in_stat_parq = None
    in_cachegrind_parq = None

    if len(csvs):
        in_csv = Path(csvs[0])
    else:
        in_csv = None

    if IGNORE_CACHE and in_csv is None:
        raise FileNotFoundError("No CSV data files found.")

    if IGNORE_CACHE:
        # Load without cache
        raw_df, stat_df, cachegrind_df = load_without_cache(in_csv)
    else:
        # Load from cache
        for i in parqs:
            if str(i).endswith("_raw.parquet"):
                in_raw_parq = Path(i)
            elif str(i).endswith("_cachegrind.parquet"):
                in_cachegrind_parq = Path(i)
            elif str(i).endswith("_stat.parquet"):
                in_stat_parq = Path(i)

            if all([in_raw_parq, in_stat_parq, in_cachegrind_parq]):
                raw_df = pd.read_parquet(in_raw_parq)
                stat_df = pd.read_parquet(in_stat_parq)
                cachegrind_df = pd.read_parquet(in_cachegrind_parq)
                break
        else:
            if in_csv is None:
                raise FileNotFoundError("Neither CSV nor parquet data files found.")
            # Parquets do not exist, fallback to loading from CSV.
            raw_df, stat_df, cachegrind_df = load_without_cache(in_csv)

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

    return raw_df, stat_df, cachegrind_df, info


if __name__ == "__main__":
    raw_df, stat_df, cachegrind_df = load()
    print(raw_df)
    print(stat_df)
    print(cachegrind_df)

    # Basic memory benchmarking
    # print(raw_df.memory_usage(deep=True))
    # print(raw_df.dtypes)
    # print(stat_df.memory_usage(deep=True))
    # print(stat_df.dtypes)
