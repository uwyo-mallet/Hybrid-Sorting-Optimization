#!/usr/bin/env python3
"""Evaluator for QST data collection."""

import ast
import json
from functools import partial
from pathlib import Path

import dash
import pandas as pd
import plotly.express as px
from dash import dcc

from .layout import gen_layout, md_template
from .loader import CLOCKS, DATA_TYPES, GRAPH_ORDER, UNITS, load

# Debug Options
# pd.set_option("display.max_columns", None)
# pd.set_option("display.max_rows", 500)
# pd.set_option("display.max_columns", 500)
# pd.set_option("display.width", 1000)


app = dash.Dash(__name__)
app.layout = partial(gen_layout, UNITS, CLOCKS, DATA_TYPES, None)


@app.callback(
    dash.dependencies.Output("stat-data", "data"),
    dash.dependencies.Output("info-data", "data"),
    dash.dependencies.Input("load-button", "n_clicks"),
    dash.dependencies.State("result-dropdown", "value"),
)
def load_result(n_clicks, results_dir):
    if not n_clicks:
        raise dash.exceptions.PreventUpdate

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
    if not json_df:
        raise dash.exceptions.PreventUpdate
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
        dash.dependencies.Input("clock-type", "value"),
        dash.dependencies.Input("data-type", "value"),
        dash.dependencies.Input("threshold-slider", "value"),
        dash.dependencies.Input("error-bars-checkbox", "value"),
    ],
)
def update_size_v_runtime(
    json_df,
    clock_type,
    data_type,
    threshold=None,
    error_bars=False,
):

    # index = clock_type + time_unit
    index = "".join([clock_type, "_secs"])
    df = df_from_json(json_df)

    if threshold is None or not any(df["threshold"] == threshold):
        threshold = df["threshold"].min()
    df = df[(df["threshold"] == threshold) | (df["threshold"] == 0)]
    df = df[df["run_type"] == data_type]
    if df.empty:
        return px.line()

    df.sort_values(["size"], inplace=True)

    if len(df["size"].unique()) == 1:
        size = df["size"].min()
        df = df[df["size"] == size]
        df.sort_values(["method"], inplace=True)
        fig = px.bar(
            df,
            x=df["method"],
            y=list(df[(index, "mean")]),
            error_y=list(df[(index, "std")]) if error_bars else None,
            facet_col="description",
            facet_col_wrap=1,
            facet_row_spacing=0.04,
            category_orders={"description": GRAPH_ORDER},
            color=df["method"],
            labels={"x": "Method", "y": f"Runtime ({index})"},
            height=2000,
        )

        fig.update_layout(
            xaxis_title="Method",
            title={
                "text": f"Method vs. Runtime, size = {size}, threshold = {threshold}",
                "xanchor": "left",
            },
        )
    else:
        # Scatter
        fig = px.line(
            df,
            x=df["size"],
            y=list(df[(index, "mean")]),
            error_y=list(df[(index, "std")]) if error_bars else None,
            facet_col="description",
            facet_col_wrap=1,
            facet_row_spacing=0.04,
            category_orders={"description": GRAPH_ORDER},
            color=df["method"],
            markers=True,
            labels={"x": "Size", "y": f"Runtime ({index})"},
            height=2000,
        )

        # General other formatting
        fig.update_layout(
            xaxis_title="Size",
            title={
                "text": f"Size vs. Runtime, threshold = {threshold}",
                "xanchor": "left",
            },
        )

    # Axis formatting
    fig.update_xaxes(
        showticklabels=True,
    )
    fig.update_yaxes(
        automargin=True,
        matches=None,
        title=f"Runtime ({index})",
    )
    fig.update_layout(
        legend_title_text="Sorting Method",
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
        dash.dependencies.Input("clock-type", "value"),
        dash.dependencies.Input("data-type", "value"),
        dash.dependencies.Input("size-slider", "value"),
        dash.dependencies.Input("error-bars-checkbox", "value"),
    ],
)
def update_threshold_v_runtime(
    json_df,
    clock_type,
    data_type,
    size=None,
    error_bars=False,
):
    df = df_from_json(json_df)

    # Check to ensure that a size is provided, and that the size is valid within the DF.
    # This is used to catch a nasty bug that, on startup, leaves the graph empty.
    if size is None or not any(df["size"] == size):
        size = df["size"].min()

    df = df[(df["size"] == size)]
    df = df[df["run_type"] == data_type]

    # Try to use insertion sort and std as baselines
    # ins_sort_df = df[(df["method"] == "insertion_sort")]
    # std_sort_df = df[(df["method"] == "std")]

    # Get all methods that support a varying threshold.
    df = df[(df["threshold"] != 0)]
    df.sort_values(["threshold"], inplace=True)

    if df.empty:
        return px.line()

    # Create dummy values for insertion sort, since it isn't affected by threshold,
    # we are just illustrating a comparison. It is okay to manually iterate over the
    # rows here, since there should only ever be one row per input data type.
    # TODO: Find a way to generalize / make the user pick baselines.
    # for _, row in ins_sort_df.iterrows():
    #     for t in df["threshold"].unique():
    #         row["threshold"] = t
    #         df = df.append(row)
    # for _, row in std_sort_df.iterrows():
    #     for t in df["threshold"].unique():
    #         row["threshold"] = t
    #         df = df.append(row)

    index = "".join([clock_type, "_secs"])

    fig = px.line(
        df,
        x=df["threshold"],
        y=list(df[(index, "mean")]),
        error_y=list(df[(index, "std")]) if error_bars else None,
        facet_col="description",
        facet_col_wrap=1,
        facet_row_spacing=0.04,
        category_orders={"description": GRAPH_ORDER},
        color=df["method"],
        markers=True,
        labels={"x": "Threshold", "y": f"Runtime ({index})"},
        height=2000,
    )

    # Axis formatting
    fig.update_xaxes(
        showticklabels=True,
    )
    fig.update_yaxes(
        automargin=True,
        matches=None,
        title=f"Runtime ({index})",
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
