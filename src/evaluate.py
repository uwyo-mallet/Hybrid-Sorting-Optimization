#!/usr/bin/env python3

import ast
import json
from dataclasses import dataclass
from pathlib import Path

import dash
import numpy as np
import pandas as pd
import plotly.express as px
from dash import dcc, html

from pprint import pprint

pd.set_option("display.max_columns", None)

RESULTS_DIR = Path("./results")
dirs = list(RESULTS_DIR.iterdir())
dirs.sort()

THRESHOLD_METHODS = (
    "qsort_asm",
    "qsort_c",
    "qsort_c_swp",
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
    info["partition"] = partition
    info["actual_num_sorts"] = len(raw_df)

    return Result(raw_df, stat_df, info)


app = dash.Dash(__name__)
app.layout = html.Div(
    [
        dcc.Store(id="stat-data"),
        dcc.Store(id="info-data"),
        html.Div(
            [
                html.Div(
                    [
                        html.H6(
                            "Info",
                            style={
                                "marginTop": "0",
                                "fontWeight": "bold",
                                "textAlign": "center",
                            },
                        ),
                        html.Div(id="info"),
                    ],
                    className="pretty_container",
                    id="cross-filter-options",
                    style={"textAlign": "left"},
                ),
                html.Div(
                    [
                        html.Div(
                            [
                                html.Div(
                                    [
                                        html.Div(
                                            [
                                                html.Div(
                                                    [
                                                        html.Label(
                                                            "Result:",
                                                            style={
                                                                "marginRight": "1em",
                                                            },
                                                        )
                                                    ]
                                                ),
                                                dcc.Dropdown(
                                                    id="result-dropdown",
                                                    options=[
                                                        {
                                                            "label": str(i),
                                                            "value": str(i),
                                                        }
                                                        for i in dirs
                                                    ],
                                                    value=str(dirs[-1]),
                                                    style=dict(
                                                        width="100%",
                                                        verticalAlign="baseline",
                                                    ),
                                                ),
                                            ],
                                            style={
                                                "display": "flex",
                                                "textAlign": "left",
                                                "verticalAlign": "baseline",
                                            },
                                        ),
                                    ],
                                    className="mini_container",
                                    id="wells",
                                ),
                            ],
                            id="info-container",
                            className="row container-display",
                        ),
                        html.Div(
                            [
                                html.Label("Time Unit"),
                                dcc.RadioItems(
                                    id="time-unit",
                                    options=[
                                        {
                                            "label": i.capitalize(),
                                            "value": i,
                                        }
                                        for i in UNITS
                                    ],
                                    value=next(iter(UNITS.keys())),
                                ),
                                dcc.Checklist(
                                    id="error-bars-checkbox",
                                    options=[
                                        {"label": "Error Bars", "value": "error_bars"}
                                    ],
                                ),
                            ],
                            className="pretty_container",
                        ),
                    ],
                    id="right-column",
                    className="eight columns",
                ),
            ],
            className="row flex-display",
        ),
        html.Div(
            [
                html.Div(
                    [
                        html.Div(
                            [
                                html.P(
                                    "Threshold",
                                    className="control_label",
                                    style={
                                        "fontWeight": "bold",
                                        "textAlign": "center",
                                    },
                                ),
                                dcc.Slider(id="threshold-slider", step=None, value=1),
                            ],
                            className="pretty_container",
                        ),
                        dcc.Graph(
                            id="size-vs-runtime-scatter",
                        ),
                    ]
                )
            ],
            className="row pretty_container",
        ),
        html.Div(
            [
                html.Div(
                    [
                        html.Div(
                            [
                                html.P(
                                    "Size",
                                    className="control_label",
                                    style={
                                        "fontWeight": "bold",
                                        "textAlign": "center",
                                    },
                                ),
                                dcc.Slider(id="size-slider", step=None, value=0),
                            ],
                            className="pretty_container",
                        ),
                        dcc.Graph(
                            id="threshold-vs-runtime-scatter",
                        ),
                    ]
                )
            ],
            className="row pretty_container",
        ),
    ],
    id="mainContainer",
    style={"display": "flex", "flexDirection": "column"},
)


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

    md = f""" `{command}`

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
    return [dcc.Markdown(md)]


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
    marks = {int(i): str(i) for i in sorted(df["size"].unique())}
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

    fig.update_xaxes(
        showticklabels=True,
        # title="Input Size",
    )
    fig.update_yaxes(automargin=True, matches=None, title=f"Runtime ({time_unit})")
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
def update_threshold_v_runtime(json_df, time_unit, size=4, error_bars=False):
    df = df_from_json(json_df)

    ins_sort_df = df[
        (df["size"] == size)
        & (df["method"] == "insertion_sort")
        & (df["description"] == "single_num")
    ].reset_index()

    df = df[(df["size"] == size) & (df["threshold"] != 0)]
    df.sort_values(["threshold"], inplace=True)

    if not len(df):
        return dash.no_update

    print("************************")
    print(df)
    print(df[(UNITS[time_unit], "mean")])
    print("************************")

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

    fig.update_xaxes(
        showticklabels=True,
    )
    fig.update_yaxes(automargin=True, matches=None, title=f"Runtime ({time_unit})")
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
    fig.for_each_annotation(lambda x: x.update(text=x.text.split("=")[-1].capitalize()))

    # fig.add_hline(
    #     y=ins_sort_df.loc[0, (UNITS[time_unit], "mean")],
    #     line_dash="dot",
    #     annotation_text="Insertion Sort",
    #     annotation_position="bottom right",
    # )

    return fig


if __name__ == "__main__":
    app.run_server(debug=True)
