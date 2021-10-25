#!/usr/bin/env python3
"""Evaluator for QST data collection."""

import ast
import json
from dataclasses import dataclass
from pathlib import Path

import dash
import numpy as np
import pandas as pd
import plotly.express as px
from dash import dcc

from .layout import gen_layout, md_template

pd.set_option("display.max_columns", None)

RESULTS_DIR = Path("./results")
dirs = sorted(list(RESULTS_DIR.iterdir()))

THRESHOLD_METHODS = {
    "qsort_asm",
    "qsort_c",
    "qsort_c_swp",
    "qsort_cpp",
    "qsort_cpp_no_comp",
}
DEFAULT_METHOD = "qsort_c"
GRAPH_ORDER = (
    "random",
    "ascending",
    "descending",
    "single_num",
)
UNITS = {
    "seconds": "elapsed_secs",
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

app = dash.Dash(__name__)
app.layout = gen_layout(UNITS, dirs)


@dataclass
class Result:
    raw_df: pd.DataFrame
    stat_df: pd.DataFrame
    info: dict


def load(in_dir=None):
    if in_dir is None:
        try:
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
        in_raw_parq = Path(in_csv.parent, in_csv.stem + ".parquet")
        in_stat_parq = Path(in_csv.parent, in_csv.stem + "_stat.parquet")

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

        # Save to disk as cache
        raw_df.to_parquet(in_raw_parq)
        stat_df.to_parquet(in_stat_parq)

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
    info["partition"] = partition
    info["actual_num_sorts"] = len(raw_df)

    return Result(raw_df, stat_df, info)


@app.callback(
    dash.dependencies.Output("stat-data", "data"),
    dash.dependencies.Output("info-data", "data"),
    [dash.dependencies.Input("result-dropdown", "value")],
)
def load_result(results_dir):
    try:
        res = load(Path(results_dir))
    except FileNotFoundError as e:
        return dash.no_update, str(e)

    return res.stat_df.to_json(), json.dumps(res.info)


@app.callback(
    dash.dependencies.Output("info", "children"),
    [dash.dependencies.Input("info-data", "data")],
)
def update_info(info_json):
    info = json.loads(info_json)
    arch = info["Processor"]
    command = info["Command"]
    node = info["Node"]
    num_cpus = info["Number of CPUs"]
    num_concurrent = info["Number of concurrent jobs"]
    platform = info["Platform"]
    partition = info["partition"]
    qst_version = info["QST Version"]
    runs = info["Runs"]
    total_num_sorts = info["Total number of sorts"]

    actual_num_sorts = info["actual_num_sorts"]
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
    df = pd.read_json(json_df)
    tuples = [ast.literal_eval(i) for i in df.columns]
    df.columns = pd.MultiIndex.from_tuples(tuples)
    return df


@app.callback(
    dash.dependencies.Output("threshold-slider", "min"),
    dash.dependencies.Output("threshold-slider", "max"),
    dash.dependencies.Output("threshold-slider", "marks"),
    [dash.dependencies.Input("stat-data", "data")],
)
def update_threshold_slider(json_df):
    df = df_from_json(json_df)
    marks = {int(i): str(i) for i in sorted(df["threshold"].unique())}
    return (
        int(df["threshold"].unique().min()),
        int(df["threshold"].unique().max()),
        marks,
    )


@app.callback(
    dash.dependencies.Output("size-slider", "min"),
    dash.dependencies.Output("size-slider", "max"),
    dash.dependencies.Output("size-slider", "marks"),
    [dash.dependencies.Input("stat-data", "data")],
)
def update_size_slider(json_df):
    df = df_from_json(json_df)
    # Format marks as 'general'
    # https://docs.python.org/3.4/library/string.html#format-string-syntax
    marks = {int(i): format(i, "g") for i in sorted(df["size"].unique())}
    return (
        int(df["size"].unique().min()),
        int(df["size"].unique().max()),
        marks,
    )


@app.callback(
    dash.dependencies.Output("size-vs-runtime-scatter", "figure"),
    [
        dash.dependencies.Input("stat-data", "data"),
        dash.dependencies.Input("time-unit", "value"),
        dash.dependencies.Input("threshold-slider", "value"),
        dash.dependencies.Input("error-bars-checkbox", "value"),
    ],
)
def update_size_v_runtime(json_df, time_unit, threshold=4, error_bars=False):
    df = df_from_json(json_df)

    df = df[(df["threshold"] == threshold) | (df["threshold"] == 0)]
    df.sort_values(["size"], inplace=True)

    fig = px.line(
        df,
        x=df["size"],
        y=list(df[(UNITS[time_unit], "mean")]),
        error_y=list(df[(UNITS[time_unit], "std")]) if error_bars else None,
        facet_col="description",
        facet_col_wrap=1,
        facet_row_spacing=0.04,
        category_orders={"description": GRAPH_ORDER},
        color=df["method"],
        markers=True,
        labels={"x": "Size", "y": f"Runtime ({time_unit})"},
        height=2000,
    )

    # Axis formatting
    fig.update_xaxes(
        showticklabels=True,
    )
    fig.update_yaxes(
        automargin=True,
        matches=None,
        title=f"Runtime ({time_unit})",
    )

    # General other formatting
    fig.update_layout(
        xaxis_title="Size",
        legend_title_text="Sorting Method",
        title={
            "text": f"Size vs. Runtime, threshold = {threshold}",
            "xanchor": "left",
        },
        font=dict(
            family="Courier New, monospace",
            size=18,
        ),
    )
    # Fix facet titles
    # TODO: Find a less gross way to do this...
    fig.for_each_annotation(lambda x: x.update(text=x.text.split("=")[-1].capitalize()))

    return fig


@app.callback(
    dash.dependencies.Output("threshold-vs-runtime-scatter", "figure"),
    [
        dash.dependencies.Input("stat-data", "data"),
        dash.dependencies.Input("time-unit", "value"),
        dash.dependencies.Input("size-slider", "value"),
        dash.dependencies.Input("error-bars-checkbox", "value"),
    ],
)
def update_threshold_v_runtime(json_df, time_unit, size=None, error_bars=False):
    df = df_from_json(json_df)

    # Check to ensure that a size is provided, and that the size is valid within the DF.
    # This is used to catch a nasty bug that, on startup, leaves the graph empty.
    if size is None or not any(df["size"] == size):
        size = df["size"].min()

    df = df[(df["size"] == size)]

    # Try to use insertion sort and std as baselines
    ins_sort_df = df[(df["method"] == "insertion_sort")]
    std_sort_df = df[(df["method"] == "std")]

    # Get all methods that support a varying threshold.
    df = df[(df["threshold"] != 0)]
    df.sort_values(["threshold"], inplace=True)

    # Create dummy values for insertion sort, since it isn't affected by threshold,
    # we are just illustrating a comparison. It is okay to manually iterate over the
    # rows here, since there should only ever be one row per input data type.
    # TODO: Find a way to generalize / make the user pick baselines.
    for _, row in ins_sort_df.iterrows():
        for t in df["threshold"].unique():
            row["threshold"] = t
            df = df.append(row)
    for _, row in std_sort_df.iterrows():
        for t in df["threshold"].unique():
            row["threshold"] = t
            df = df.append(row)

    fig = px.line(
        df,
        x=df["threshold"],
        y=list(df[(UNITS[time_unit], "mean")]),
        error_y=list(df[(UNITS[time_unit], "std")]) if error_bars else None,
        facet_col="description",
        facet_col_wrap=1,
        facet_row_spacing=0.04,
        category_orders={"description": GRAPH_ORDER},
        color=df["method"],
        markers=True,
        labels={"x": "Threshold", "y": f"Runtime ({time_unit})"},
        height=2000,
    )

    # Axis formatting
    fig.update_xaxes(
        showticklabels=True,
    )
    fig.update_yaxes(
        automargin=True,
        matches=None,
        title=f"Runtime ({time_unit})",
    )

    # General other formatting
    fig.update_layout(
        xaxis_title="Threshold",
        legend_title_text="Sorting Method",
        title={
            "text": f"Threshold vs. Runtime, size = {size:,}",
            "xanchor": "left",
        },
        font=dict(
            family="Courier New, monospace",
            size=18,
        ),
        legend=dict(yanchor="top", xanchor="left"),
    )

    # Fix facet titles
    # TODO: Find a less gross way to do this...
    fig.for_each_annotation(lambda x: x.update(text=x.text.split("=")[-1].capitalize()))

    return fig


if __name__ == "__main__":
    app.run_server(debug=True)
