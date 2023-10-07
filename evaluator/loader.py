"""Loader for results."""
import json
import re
import subprocess
import sys
from pathlib import Path
from typing import Optional

import numpy as np
import pandas as pd

from .generics import CACHEGRIND_COLS


def load_cachegrind(df, valgrind_dir: Optional[Path]):
    """TODO."""
    # TODO: Handle a different method name in CSV vs C source code.
    cachegrind_df = None
    if valgrind_dir is None:
        return None

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
            if "command" in line:
                continue

            if method in line:
                tokens = [int(i.replace(",", "")) for i in pattern.findall(line)]
                if len(tokens) != len(CACHEGRIND_COLS):
                    print("[Warning]: Malformed line found, skipping.", file=sys.stderr)
                    continue

                data = row._asdict()
                data.update(dict(zip(CACHEGRIND_COLS, tokens)))

                # Dynamically figure out what columns we need
                if cachegrind_df is None:
                    cachegrind_df = pd.DataFrame(columns=list(data.keys()))

                # Append row to output dataframe
                cachegrind_df.loc[i] = data
                break
        else:
            print(f"[Warning]: Could not locate method: {method}", file=sys.stderr)

    return cachegrind_df


def load(
    in_dir=None,
    results_dir=Path("./results"),
    job_details_file=Path("job_details.json"),
    partition_file=Path("partition"),
    valgrind_dir=Path("valgrind/"),
):
    """TODO."""
    if in_dir is None:
        try:
            results_dir = Path(results_dir)
            dirs = sorted(list(results_dir.iterdir()))
            in_dir = dirs[-1]
        except IndexError as e:
            raise FileNotFoundError(
                f"No result subdirectories in '{results_dir}'"
            ) from e

    csvs = tuple(in_dir.glob("output*.csv"))
    if not csvs:
        raise FileNotFoundError(f"No CSV files found in {in_dir}")

    # Load the data
    in_csv = Path(csvs[0])
    df = pd.read_csv(in_csv, engine="c")
    df["wall_secs"] = df["wall_nsecs"] / 1_000_000_000
    df["user_secs"] = df["user_nsecs"] / 1_000_000_000
    df["system_secs"] = df["system_nsecs"] / 1_000_000_000

    base_df = df[df["run_type"] == "base"]
    base_df = base_df.drop(["input", "id"], axis=1)

    # Load cachegrind
    valgrind_path = in_dir / valgrind_dir
    cachegrind_cache_path = Path(in_dir / "cachegrind.cache.csv")
    if cachegrind_cache_path.is_file():
        cachegrind_df = pd.read_csv(cachegrind_cache_path)
    else:
        cachegrind_df = df[df["run_type"] != "base"]
        cachegrind_df = load_cachegrind(cachegrind_df, valgrind_path)
        cachegrind_df.to_csv(cachegrind_cache_path)

    # Load any misc metadata
    info_path = in_dir / job_details_file
    info = json.loads(info_path.read_text()) if info_path.is_file() else {}

    partition_path = in_dir / partition_file
    partition = partition_path.read_text() if partition_path.is_file() else None

    info["partition"] = partition
    info["actual_num_sorts"] = len(df)

    return base_df, cachegrind_df, info
