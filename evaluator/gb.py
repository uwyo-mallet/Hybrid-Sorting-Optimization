"""GB_QST result viewer."""

import json
from pathlib import Path

import dash
import plotly.express as px
from dash.dependencies import Input, Output, State

from .gb_layout import gen_layout
from .gb_loader import load
from .generics import (GRAPH_ORDER, df_from_json, update_info,
                       update_size_slider, update_threshold_slider)

# Debug Options
# print("DEBUG")
# from .debug import *

app = dash.Dash(__name__)
app.layout = gen_layout


@app.callback(
    Output("gb-data", "data"),
    Output("info-data", "data"),
    Input("load-button", "n_clicks"),
    State("result-dropdown", "value"),
)
def load_result(n_clicks, results_dir=None):
    """User callback to load results from disk."""
    if not n_clicks:
        raise dash.exceptions.PreventUpdate

    try:
        df, info = load(Path(results_dir))
    except FileNotFoundError as e:
        return dash.no_update, str(e)

    return df.to_json(), json.dumps(info)


update_info = app.callback(
    Output("info", "children"),
    Input("info-data", "data"),
)(update_info)

update_threshold_slider = app.callback(
    Output("threshold-slider", "min"),
    Output("threshold-slider", "max"),
    Output("threshold-slider", "marks"),
    Input("gb-data", "data"),
)(update_threshold_slider)


update_size_slider = app.callback(
    Output("size-slider", "min"),
    Output("size-slider", "max"),
    Output("size-slider", "marks"),
    Input("gb-data", "data"),
)(update_size_slider)


@app.callback(
    Output("size-vs-runtime-scatter", "figure"),
    [
        Input("gb-data", "data"),
        Input("threshold-slider", "value"),
        Input("error-bars-checkbox", "value"),
    ],
)
def update_size_v_runtime(json_df, threshold=None, error_bars=False):
    """User callback to update size vs. runtime plots."""
    if not json_df:
        return px.line()
    df = df_from_json(json_df)

    if threshold is None or not any(df["threshold"] == threshold):
        threshold = df["threshold"].min()

    df = df[(df["threshold"] == threshold) | (df["threshold"] == 0)]
    df = df.sort_values(["size"])

    if df.empty:
        return px.line()

    # If only one size was sampled during the experiment, display a bar graph,
    # otherwise use a line graph.
    if len(df["size"].unique()) == 1:
        size = df["size"].min()
        df = df[df["size"] == size]
        df = df.sort_values(["method"])
        fig = px.bar(
            df,
            x=df["method"],
            y=list(df[("cpu_time", "mean")]),
            error_y=list(df[("cpu_time", "std")]) if error_bars else None,
            facet_col="description",
            facet_col_wrap=1,
            facet_row_spacing=0.04,
            category_orders={"description": GRAPH_ORDER},
            color=df["method"],
            labels={"x": "Method", "y": "Runtime (nanoseconds)"},
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
        df = df.sort_values(["size", "method"])
        fig = px.line(
            df,
            x=df["size"],
            y=list(df[("cpu_time", "mean")]),
            error_y=list(df[("cpu_time", "std")]) if error_bars else None,
            facet_col="description",
            facet_col_wrap=1,
            facet_row_spacing=0.04,
            category_orders={"description": GRAPH_ORDER},
            color=df["method"],
            markers=True,
            labels={"x": "Size", "y": "Runtime (nanoseconds)"},
            height=2000,
        )
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
        title="Runtime (nanoseconds)",
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
    Output("threshold-vs-runtime-scatter", "figure"),
    [
        Input("gb-data", "data"),
        Input("size-slider", "value"),
        Input("error-bars-checkbox", "value"),
    ],
)
def update_threshold_v_runtime(json_df, size=None, error_bars=False):
    """User callback to update threshold vs. runtime plots."""
    df = df_from_json(json_df)

    if size is None or not any(df["size"] == size):
        size = df["size"].min()

    df = df[df["size"] == size]

    # Try to use insertion sort and std as baselines
    # ins_sort_df = df[(df["method"] == "insertion_sort")]
    # Retain backwards compatilbility with older naming scheme
    std_sort_df = df[(df["method"] == "std::sort") | (df["method"] == "std")]
    vanilla_df = df[df["method"] == "qsort_vanilla"]

    # Get all methods that support a varying threshold.
    df = df[df["threshold"] != 0]
    df = df.sort_values(["threshold", "method"])
    if df.empty:
        return px.line()

    # Create dummy values for std::sort, since it isn't affected by threshold,
    # we are just illustrating a comparison. It is okay to manually iterate over the
    # rows here, since there should only ever be one row per input data type.
    # TODO: Find a way to generalize / make the user pick baselines.
    # TODO: Reevaluate the performance implifications of this.

    for _, row in std_sort_df.iterrows():
        row["threshold"] = df["threshold"].min()
        df = df.append(row)
        row["threshold"] = df["threshold"].max()
        df = df.append(row)

    for _, row in vanilla_df.iterrows():
        row["threshold"] = df["threshold"].min()
        df = df.append(row)
        row["threshold"] = df["threshold"].max()
        df = df.append(row)

    fig = px.line(
        df,
        x=df["threshold"],
        y=list(df[("cpu_time", "mean")]),
        error_y=list(df[("cpu_time", "std")]) if error_bars else None,
        facet_col="description",
        facet_col_wrap=1,
        facet_row_spacing=0.04,
        category_orders={"description": GRAPH_ORDER},
        color=df["method"],
        markers=True,
        labels={"x": "Threshold", "y": "Runtime (nanoseconds)"},
        height=2000,
    )
    fig.update_xaxes(
        showticklabels=True,
    )
    fig.update_yaxes(
        automargin=True,
        matches=None,
        title="Runtime (nanoseconds)",
    )
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
