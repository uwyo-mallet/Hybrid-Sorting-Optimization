from dataclasses import dataclass
from .generics import GB_RESULTS_DIR
import json


import pandas as pd
from pprint import pprint
import numpy as np

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


def load(in_dir=None):
    if in_dir is None:
        try:
            dirs = sorted(list(GB_RESULTS_DIR.iterdir()))
            in_dir = dirs[-1]
        except IndexError as e:
            raise FileNotFoundError("No result directories found") from e

    output_file = in_dir / DATA_FILENAME
    with open(output_file, "r") as json_file:
        raw_data = json.load(json_file)

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
