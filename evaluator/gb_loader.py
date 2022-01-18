"""Loader for GB_QST results."""
import json
from collections import defaultdict
from json.decoder import JSONDecodeError
from pathlib import Path
from typing import Optional

import numpy as np
import pandas as pd

from .generics import GB_RESULTS_DIR

# Debug Options
# print("DEBUG")
# from .debug import *

RAW_COLUMNS = {
    "method",
    "input",
    "size",
    "threshold",
    "description",
    "cpu_time",
    "real_time",
    "iterations",
    "name",
    "size",
    "repetitions_index",
    "repetitions",
}

IGNORE_CACHE = False
JOB_DETAILS_FILENAME = "job_details.json"
MERGED_JSON_FILENAME = "output.json"
CACHE_FILENAME = "output.parquet"


def lookup_run_name(run_name: str):
    """TODO: Document this."""
    return run_name.rsplit("/")[1]


def merge_json(in_dir: Path, ignore_existing=False) -> dict:
    """
    Merge all the relevant information from all the output files from GB_QST.

    Writes resulting merged json to `in_dir/DATA_FILENAME`.
    If this file already exists, it is read and returned instead.

    @param in_dir: Directory containing all the JSON files to merge.
    @param ignore_existing: Ignore an existing DATA_FILENAME file and merge again.
    @returns: Dict with all merged JSON contents.

    == Output Format ==
    Dict of datatypes with a nested dict of threshold values
    (0 if not supported for those methods):

    {
        "ascending":
        {
            1: [Name:"qsort_c", "cpu_time": 1234, . . .],
            1: [Name:"qsort_c", "cpu_time": 1234, . . .],
            0: [Name: "insertion_sort", "cpu_time": 1234, . . .],
        }
        "descending":
        . . .
    }

    When written to JSON, keys and values are marshalled according to this
    RFC (https://datatracker.ietf.org/doc/html/rfc7159.html) and this
    (https://www.ecma-international.org/publications-and-standards/standards/ecma-404/)
    standard. Most notably, integer keys become strings.
    """
    out_file = in_dir / MERGED_JSON_FILENAME

    # Merge was done before, just return the saved contents.
    if out_file.exists() and not ignore_existing:
        with open(out_file, "r") as json_file:
            return json.load(json_file)

    data = defaultdict(lambda: defaultdict(list))

    for f in in_dir.rglob("*.json"):
        with open(f, "r") as json_file:
            try:
                raw = json.load(json_file)
            except JSONDecodeError:
                print(f"Malformed JSON file: {f}")
                continue

            description = raw["context"].get("description", "N/A")
            input_file = raw["context"]["input"]
            size = int(raw["context"]["size"])

            uses_threshold = int(raw["context"]["uses_threshold"])
            threshold = int(raw["context"]["threshold"]) if uses_threshold else 0

            for run in raw["benchmarks"]:
                # Ignore aggregate results
                if run["run_type"] == "iteration":
                    run["input"] = input_file
                    run["size"] = size
                    data[description][threshold].append(run)

    with open(out_file, "w") as out_file:
        json.dump(data, out_file, indent=4, sort_keys=True)

    return data


def load(in_dir: Optional[Path] = None) -> (pd.DataFrame, dict):
    """
    Load a GB_QST result directory from disk.

    If necessary, merges all JSON output files, then caches the resulting
    dataframe as a parquet file (in_dir/CACHE_FILENAME). Technically there are
    two caches, since the merged json files are also saved as
    in_dir/json/OUTPUT_FILENAME.

    @param in_dir: GB_QST result directory.
    @returns: Processed pandas dataframe, and general system information.
    """
    if in_dir is None:
        try:
            dirs = sorted(list(GB_RESULTS_DIR.iterdir()))
            in_dir = dirs[-1]
        except IndexError as e:
            raise FileNotFoundError("No result directories found") from e

    # Actually loading the results
    cache_file = Path(in_dir, CACHE_FILENAME)
    if not IGNORE_CACHE and cache_file.is_file():
        df = pd.read_parquet(cache_file)
    else:
        # No cache available, process the raw data.
        raw_data = merge_json(in_dir / "json")

        # Cleanup and convert to pandas df
        data = []
        for input_type in raw_data:
            for threshold in raw_data[input_type]:
                for result in raw_data[input_type][threshold]:
                    row = {
                        "description": input_type,
                        "threshold": threshold,
                        # Discard unused keys
                        **{k: v for k, v in result.items() if k in RAW_COLUMNS},
                    }
                    row["name"] = lookup_run_name(row["name"])
                    data.append(row)

        df = pd.DataFrame.from_dict(data)
        df = df.rename(columns={"name": "method"})

        # Compute mean and stddev
        stats = (np.mean, np.std)
        df = df.groupby(["input", "description", "threshold", "size", "method"])
        df = df.agg({"cpu_time": stats, "real_time": stats})
        df = df.reset_index()
        df = df.sort_values(["size"])

        # Cache so we don't have to compute this again.
        df.to_parquet(cache_file)

    # System info
    info_path = in_dir / JOB_DETAILS_FILENAME
    if info_path.is_file():
        info = json.loads(info_path.read_text())
    else:
        info = {}

    return df, info
