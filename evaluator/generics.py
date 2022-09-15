import json
from pathlib import Path

import dash
import pandas as pd
from dash import dcc

QST_EXEC_PATH = Path("./build/QST")
QST_RESULTS_DIR = Path("./results")
GB_RESULTS_DIR = Path("./gb_results")

GRAPH_ORDER = (
    "random",
    "ascending",
    "descending",
    "single_num",
)

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
    """TODO."""
    info = json.loads(info_json)
    arch = info["Processor"]
    command = info["Command"]
    node = info["Node"]
    num_cpus = info["Number of CPUs"]
    num_concurrent = info["Number of concurrent jobs"]
    platform = info["Platform"]
    partition = info.get("partition", "")
    qst_version = info["QST"]["Version"]
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
    """TODO."""
    if not json_df:
        raise dash.exceptions.PreventUpdate
    df = pd.read_json(json_df)
    return df


def update_threshold_slider(json_df):
    """TODO."""
    df = df_from_json(json_df)
    marks = {int(i): "" for i in sorted(df["threshold"].unique())}
    return (
        df["threshold"].min(),
        df["threshold"].max(),
        marks,
    )


def update_size_slider(json_df):
    """TODO."""
    df = df_from_json(json_df)
    marks = {int(i): "" for i in sorted(df["size"].unique())}
    return (
        df["size"].min(),
        df["size"].max(),
        marks,
    )
