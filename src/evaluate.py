#!/usr/bin/env python3

import json
from dataclasses import dataclass
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import plotly.express as px

RESULTS_DIR = Path("./results")
THRESHOLD_METHODS = (
    "qsort_asm",
    "qsort_c",
    "qort_c_improved",
    "qsort_cpp",
    "qsort_cpp_no_comp",
)
DEFAULT_METHOD = "qsort_c"
INPUT_TYPES = (
    "ascending",
    "descending",
    "random",
    "single_num",
    "N/A",
)
UNITS = {
    "seconds": "secs",
    "milliseconds": "elapsed_msecs",
    "microseconds": "elapsed_usecs",
}
COLUMNS = {
    "method": str,
    "input": str,
    "description": str,
    "size": np.uint64,
    "elapsed_usecs": np.uint64,
    "threshold": np.uint64,
}


@dataclass
class Result:
    raw_df: pd.DataFrame
    stat_df: pd.DataFrame
    info: dict
    partition: str


def load(in_dir=None):
    if in_dir is None:
        dirs = list(RESULTS_DIR.iterdir())
        dirs.sort()
        try:
            in_dir = dirs[-1]
        except IndexError as e:
            raise FileNotFoundError("No result directories found") from e

    # TODO: Refactor this to support just having a parquet file.
    try:
        csvs = tuple(in_dir.glob("*.csv"))
        in_csv = csvs[0]
    except IndexError as e:
        raise FileNotFoundError("No csv found") from e

    # If not already in parquet form, create new parquet version
    # to save load time on subsequent runs.
    in_raw_parq = Path(in_csv.parent, in_csv.stem + ".parquet")
    in_stat_parq = Path(in_csv.parent, in_csv.stem + "_stat.parquet")

    if not in_raw_parq.exists():
        raw_df = pd.read_csv(in_csv, dtype=COLUMNS, engine="c")
        # Cleanup and add some additional columns
        raw_df["elapsed_msecs"] = raw_df["elapsed_usecs"] / 1000
        raw_df["elapsed_secs"] = raw_df["elapsed_msecs"] / 1000

        # Reorder columns
        raw_df = raw_df[
            [
                "input",
                "method",
                "description",
                "threshold",
                "size",
                "elapsed_usecs",
                "elapsed_msecs",
                "elapsed_secs",
            ]
        ]

        raw_df.to_parquet(in_raw_parq)
    else:
        raw_df = pd.read_parquet(in_raw_parq)

    if not in_stat_parq.exists():
        stats = ("mean", "std")
        stat_df = raw_df
        stat_df = stat_df.groupby(
            ["input", "method", "description", "size", "threshold"]
        ).agg(
            {
                "elapsed_usecs": stats,
                "elapsed_msecs": stats,
                "elapsed_secs": stats,
            }
        )
        stat_df = stat_df.reset_index()
        stat_df.sort_values(["size"], inplace=True)
        stat_df.to_parquet(in_stat_parq)
    else:
        stat_df = pd.read_parquet(in_stat_parq)

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

    return Result(raw_df, stat_df, info, partition)


if __name__ == "__main__":
    res = load()

    df = res.stat_df
    df = df[df["method"] == "qsort_c"]
    df = df[df["description"] == "random"]
    df = df[df["threshold"] == 4]

    print(df)

    fig = px.line(df, x=df["size"], y=list(df[("elapsed_usecs", "mean")]))
    fig.show()
