"""Loader for QST results."""
import json
import re
import subprocess
import sys
from pathlib import Path
from typing import Optional

import numpy as np
import pandas as pd
from qstpy import QST

# IGNORE_CACHE = False

# UNITS = {
#     "seconds": "_secs",
#     "milliseconds": "_msecs",
#     "microseconds": "_usecs",
#     "nanoseconds": "_nsecs",
# }

# RAW_COLUMNS = {
#     "id": np.uint64,
#     "method": str,
#     "input": str,
#     "size": np.uint64,
#     "threshold": np.uint64,
#     "wall_nsecs": np.uint64,  # Elapsed wall time
#     "user_nsecs": np.uint64,  # Elapsed user cpu time
#     "system_nsecs": np.uint64,  # Elapsed system cpu time
#     "description": str,
#     "run_type": str,
# }
# POST_PROCESS_COLUMNS = [
#     "id",
#     "input",
#     "method",
#     "description",
#     "threshold",
#     "size",
#     "run_type",
#     "wall_nsecs",
#     "user_nsecs",
#     "system_nsecs",
#     "wall_secs",
#     "user_secs",
#     "system_secs",
# ]
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
    # base_cols = [
    #     "method",
    #     "description",
    #     "threshold",
    #     "size",  # Necessary?
    # ]
    # cachegrind_df = pd.DataFrame(columns=base_cols + CACHEGRIND_COLS, dtype=np.uint64)
    # if valgrind_dir is None:
    #     return cachegrind_df

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


def load(
    in_dir=None,
    qst_results_dir="./results",
    job_details_file="job_details.json",
    partition_file="partition",
):
    """TODO."""
    if in_dir is None:
        try:
            qst_results_dir = Path(qst_results_dir)
            dirs = sorted(list(qst_results_dir.iterdir()))
            in_dir = dirs[-1]
        except IndexError as e:
            raise FileNotFoundError(
                f"No result subdirectories in '{qst_results_dir}'"
            ) from e

    csvs = tuple(in_dir.glob("*.csv"))

    if not csvs:
        raise FileNotFoundError(f"No CSV files found in {in_dir}")

    # Load the data
    in_csv = Path(csvs[0])
    df = pd.read_csv(in_csv, engine="c")
    df = df.drop(["input", "id"], axis=1)
    df["wall_secs"] = df["wall_nsecs"] / 1_000_000_000
    df["user_secs"] = df["user_nsecs"] / 1_000_000_000
    df["system_secs"] = df["system_nsecs"] / 1_000_000_000

    # Load any misc metadata
    info_path = in_dir / job_details_file
    info = json.loads(info_path.read_text()) if info_path.is_file() else {}

    partition_path = in_dir / partition_file
    partition = partition_path.read_text() if partition_path.is_file() else None

    info["partition"] = partition
    info["actual_num_sorts"] = len(df)

    return df, info
