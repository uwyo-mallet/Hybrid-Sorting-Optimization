import json
from dash import dcc
from pathlib import Path

import dash
import pandas as pd
import ast

QST_RESULTS_DIR = Path("./results")
GB_RESULTS_DIR = Path("./gb_results")

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
DATA_TYPES = {
    "ascending",
    "descending",
    "random",
    "single_num",
}

md_template = """\
`{command}`

Architecture: {arch}

Node: {node}

Platform: {platform}

Partition: {partition}

Cores: {num_cpus}

Concurrent jobs: {num_concurrent}

Runs: {runs}

Expected \# of sorts: {total_num_sorts}

Actual \# of sorts: {actual_num_sorts}

QST Version:
```txt
{qst_version}
```
"""


def update_info(info_json):
    info = json.loads(info_json)
    arch = info["Processor"]
    command = info["Command"]
    node = info["Node"]
    num_cpus = info["Number of CPUs"]
    num_concurrent = info["Number of concurrent jobs"]
    platform = info["Platform"]
    partition = info.get("partition", "")
    qst_version = info["QST Version"]
    runs = info["Runs"]
    total_num_sorts = info.get("Total number of sorts", "")

    actual_num_sorts = info.get("actual_num_sorts", "")
    if actual_num_sorts != total_num_sorts:
        actual_num_sorts = f"\[Warning\] {actual_num_sorts}"

    formatted_md = md_template.format(
        arch=arch,
        command=command,
        node=node,
        num_cpus=num_cpus,
        num_concurrent=num_concurrent,
        platform=platform,
        partition=partition,
        qst_version=qst_version,
        runs=runs,
        total_num_sorts=total_num_sorts,
        actual_num_sorts=actual_num_sorts,
    )

    return [dcc.Markdown(formatted_md)]


def df_from_json(json_df):
    if not json_df:
        raise dash.exceptions.PreventUpdate
    df = pd.read_json(json_df)
    tuples = [ast.literal_eval(i) for i in df.columns]
    df.columns = pd.MultiIndex.from_tuples(tuples)
    return df


def update_threshold_slider(json_df):
    df = df_from_json(json_df)
    marks = {int(i): "" for i in sorted(df["threshold"].unique())}
    return (
        int(df["threshold"].unique().min()),
        int(df["threshold"].unique().max()),
        marks,
    )


def update_size_slider(json_df):
    df = df_from_json(json_df)
    # Format marks as 'general'
    # https://docs.python.org/3.4/library/string.html#format-string-syntax
    marks = {int(i): "" for i in sorted(df["size"].unique())}
    return (
        int(df["size"].unique().min()),
        int(df["size"].unique().max()),
        marks,
    )
