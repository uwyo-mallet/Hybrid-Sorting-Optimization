from dataclasses import dataclass
from json.decoder import JSONDecodeError
from .generics import GB_RESULTS_DIR, DATA_TYPES
import json


import pandas as pd
import numpy as np

from collections import defaultdict
from pathlib import Path

# Debug Options
pd.set_option("display.max_columns", None)
pd.set_option("display.max_rows", 500)
pd.set_option("display.max_columns", 500)
pd.set_option("display.width", 1000)


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
JOB_DETAILS_FILE = "job_details.json"
DATA_FILENAME = "output.json"


@dataclass
class Result:
    df: pd.DataFrame
    info: dict


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
    out_file = in_dir / DATA_FILENAME

    # Merge was done before, just return the saved contents.
    if out_file.exists() and not ignore_existing:
        with open(out_file, "r") as json_file:
            return json.load(json_file)

    data = defaultdict(lambda: defaultdict(list))

    for i in DATA_TYPES:
        for f in in_dir.rglob(f"*.{i}"):
            with open(f, "r") as json_file:
                try:
                    raw = json.load(json_file)
                except JSONDecodeError:
                    print(f"Malformed JSON: {f}")
                    continue

                input_file = raw["context"]["input"]
                size = int(raw["context"]["size"])
                threshold = int(raw["context"]["threshold"])
                uses_threshold = int(raw["context"]["uses_threshold"])

                if uses_threshold:
                    for run in raw["benchmarks"]:
                        run["input"] = input_file
                        run["size"] = size
                        data[i][threshold].append(run)
                else:
                    for run in raw["benchmarks"]:
                        run["input"] = input_file
                        run["size"] = size
                        data[i][0].append(run)

    with open(out_file, "w") as out_file:
        json.dump(data, out_file, indent=4, sort_keys=True)

    return data


def load(in_dir=None):
    if in_dir is None:
        try:
            dirs = sorted(list(GB_RESULTS_DIR.iterdir()))
            in_dir = dirs[-1]
        except IndexError as e:
            raise FileNotFoundError("No result directories found") from e

    raw_data = merge_json(in_dir)

    data = []
    for input_type in raw_data:
        for threshold in raw_data[input_type]:
            for result in raw_data[input_type][threshold]:
                if "aggregate_name" not in result.keys():
                    row = {
                        "description": input_type,
                        "threshold": threshold,
                        **{k: v for k, v in result.items() if k in RAW_COLUMNS},
                    }
                    row["name"] = lookup_run_name(row["name"])
                    data.append(row)

    df = pd.DataFrame.from_dict(data)
    df = df.rename(columns={"name": "method"})

    stats = (np.mean, np.std)
    df = df.groupby(["input", "description", "threshold", "size", "method",]).agg(
        {
            "cpu_time": stats,
            "real_time": stats,
        }
    )
    df = df.reset_index()
    df = df.sort_values(["size"])

    info_path = in_dir / JOB_DETAILS_FILE
    if info_path.is_file():
        info = json.loads(info_path.read_text())
    else:
        info = {}

    return Result(df, info)
