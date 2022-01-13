#!/usr/bin/env python3
"""
Google benchmark result comparer/viewer.

"""

from functools import partial
from pathlib import Path
from pprint import pprint

import dash
import json

from .generics import GRAPH_ORDER, df_from_json, update_info
from .gb_layout import gen_layout
from .gb_loader import load
import pandas as pd

import plotly.express as px
import plotly.graph_objects as go


from dash.dependencies import Input, Output, State

# Debug Options
pd.set_option("display.max_columns", None)
pd.set_option("display.max_rows", 500)
pd.set_option("display.max_columns", 500)
pd.set_option("display.width", 1000)


app = dash.Dash(__name__)
app.layout = gen_layout


@app.callback(
    Output("gb-data", "data"),
    Output("info-data", "data"),
    Input("load-button", "n_clicks"),
    State("result-dropdown", "value"),
)
def load_result(n_clicks, results_dir=None):
    if not n_clicks:
        raise dash.exceptions.PreventUpdate

    try:
        res = load(Path(results_dir))
    except FileNotFoundError as e:
        return dash.no_update, str(e)

    return res.df.to_json(), json.dumps(res.info)


update_info = app.callback(
    Output("info", "children"),
    Input("info-data", "data"),
)(update_info)


@app.callback(
    Output("threshold-vs-runtime-scatter", "figure"),
    [
        Input("gb-data", "data"),
        Input("size-slider", "value"),
        Input("error-bars-checkbox", "value"),
    ],
)
def update_threshold_v_runtime(
    json_df,
    size=None,
    error_bars=False,
):
    df = df_from_json(json_df)
    print(df)

    if size is None or not any(df["size"] == size):
        size = df["size"].min()

    df = df[df["size"] == size]

    # Try to use insertion sort and std as baselines
    # ins_sort_df = df[(df["method"] == "insertion_sort")]
    # Retain backwards compatilbility with older naming scheme
    # std_sort_df = df[(df["method"] == "std::sort") | (df["method"] == "std")]
    # vanilla_df = df[df["method"] == "qsort_vanilla"]

    # Get all methods that support a varying threshold.
    df = df[df["threshold"] != 0]
    df = df.sort_values(["threshold", "method"])
    if df.empty:
        return px.line()

    # Create dummy values for insertion sort, since it isn't affected by threshold,
    # we are just illustrating a comparison. It is okay to manually iterate over the
    # rows here, since there should only ever be one row per input data type.
    # TODO: Find a way to generalize / make the user pick baselines.
    # TODO: Reevaluate the performance implifications of this.
    # for _, row in ins_sort_df.iterrows():
    #     for t in df["threshold"].unique():
    #         row["threshold"] = t
    #         df = df.append(row)

    # for _, row in std_sort_df.iterrows():
    #     row["threshold"] = df["threshold"].min()
    #     df = df.append(row)
    #     row["threshold"] = df["threshold"].max()
    #     df = df.append(row)

    # for _, row in vanilla_df.iterrows():
    #     row["threshold"] = df["threshold"].min()
    #     df = df.append(row)
    #     row["threshold"] = df["threshold"].max()
    #     df = df.append(row)

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
