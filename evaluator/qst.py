"""QST result viewer."""
import json
from pathlib import Path

import dash
import numpy as np
import pandas as pd
import plotly.express as px
import plotly.graph_objects as go
from dash.dependencies import Input, Output, State
from qstpy import QST

from .generics import (CACHEGRIND_COLS, df_from_json, update_info,
                       update_size_slider, update_threshold_slider)
from .qst_layout import gen_layout
from .qst_loader import load

CLOCKS = {
    "wall": "wall_secs",
    "user": "user_secs",
    "system": "system_secs",
}

RUN_TYPES = (
    "base",
    "callgrind",
    "cachegrind",
    "massif",
)

CACHE_MAP = {
    "I1mr": "Ir",
    "ILmr": "Ir",
    "D1mr": "Dr",
    "DLmr": "Dr",
    "D1mw": "Dw",
    "DLmw": "Dw",
    "Bcm": "Bc",
    "Bim": "Bi",
}


app = dash.Dash(__name__)
app.layout = gen_layout(CLOCKS, RUN_TYPES, CACHE_MAP.keys())
q = QST()


@app.callback(
    Output("run-data", "data"),
    Output("cachegrind-data", "data"),
    Output("info-data", "data"),
    Input("load-button", "n_clicks"),
    State("result-dropdown", "value"),
)
def load_result_callback(n_clicks, results_dir):
    """TODO."""
    if not n_clicks:
        raise dash.exceptions.PreventUpdate

    try:
        df, cachegrind_df, info = load(Path(results_dir))
    except FileNotFoundError as e:
        return dash.no_update, str(e)

    return df.to_json(), cachegrind_df.to_json(), json.dumps(info)


update_info_callback = app.callback(
    Output("info", "children"),
    Input("info-data", "data"),
)(update_info)


update_threshold_slider_callback = app.callback(
    Output("threshold-slider", "min"),
    Output("threshold-slider", "max"),
    Output("threshold-slider", "marks"),
    Input("run-data", "data"),
)(update_threshold_slider)


update_size_slider_callback = app.callback(
    Output("size-slider", "min"),
    Output("size-slider", "max"),
    Output("size-slider", "marks"),
    Input("run-data", "data"),
)(update_size_slider)


@app.callback(
    Output("size-vs-runtime-scatter", "figure"),
    [
        Input("run-data", "data"),
        Input("clock-type", "value"),
        Input("run-type", "value"),
        Input("threshold-slider", "value"),
        Input("error-bars-checkbox", "value"),
    ],
)
def update_size_v_runtime_callback(
    json_df,
    clock_type,
    run_type,
    threshold=None,
    error_bars=False,
):
    """TODO."""
    if not json_df:
        raise dash.exceptions.PreventUpdate

    df = df_from_json(json_df)
    if run_type not in df["run_type"].values:
        raise dash.exceptions.PreventUpdate

    pivot_columns = [
        "method",
        "description",
        "run_type",
        "threshold",
        "size",
    ]
    stat_df = df.pivot_table(index=pivot_columns, aggfunc=[np.mean, np.std])

    try:
        data = stat_df.loc[(slice(None), slice(None), run_type, 0)]
    except KeyError:
        # No runs with nonthreshold methods
        data = pd.DataFrame()
        if threshold == 0:
            threshold = df["threshold"].min()

    if threshold > 0:
        data = pd.concat(
            (data, stat_df.loc[slice(None), slice(None), run_type, threshold])
        )

    try:
        error_y = data["std"][clock_type].reset_index()[clock_type]
    except KeyError:
        # Only one sample was taken, can't calculate error
        error_y = None

    if len(df["size"].unique()) == 1:
        fig = px.bar(
            data["mean"][clock_type].reset_index(),
            x="method",
            y=clock_type,
            error_y=error_y if error_bars else None,
            color="method",
            facet_col="description",
            category_orders={
                "description": ("random", "ascending", "descending", "single_num")
            },
            facet_col_wrap=1,
            height=1500,
        )
    else:
        fig = px.line(
            data["mean"][clock_type].reset_index(),
            x="size",
            y=clock_type,
            error_y=error_y if error_bars else None,
            color="method",
            markers=True,
            facet_col="description",
            category_orders={
                "description": ("random", "ascending", "descending", "single_num")
            },
            facet_col_wrap=1,
            height=1500,
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
        autorange=True,
        title=f"Runtime ({clock_type})",
    )

    fig.update_layout(
        legend_title_text="Sorting Method",
        font=dict(
            family="Courier New, monospace",
            size=18,
        ),
    )

    # Fix facet titles
    fig.for_each_annotation(lambda x: x.update(text=x.text.split("=")[-1].capitalize()))
    return fig


@app.callback(
    Output("threshold-vs-runtime-scatter", "figure"),
    [
        Input("run-data", "data"),
        Input("clock-type", "value"),
        Input("run-type", "value"),
        Input("size-slider", "value"),
        Input("error-bars-checkbox", "value"),
    ],
)
def update_threshold_v_runtime(
    json_df,
    clock_type,
    run_type,
    size=None,
    error_bars=False,
):
    """TODO."""
    if not json_df:
        raise dash.exceptions.PreventUpdate

    df = df_from_json(json_df)
    if run_type not in df["run_type"].values:
        raise dash.exceptions.PreventUpdate
    if size is None or size == 0:
        size = df["size"].min()

    pivot_columns = [
        "method",
        "description",
        "run_type",
        "threshold",
        "size",
    ]
    stat_df = df.pivot_table(index=pivot_columns, aggfunc=[np.mean, np.std])
    data = stat_df.loc[(slice(None), slice(None), run_type, slice(None), size)]
    try:
        error_y = data["std"][clock_type].reset_index()[clock_type]
    except KeyError:
        # Only one sample was taken, can't calculate error
        error_y = None

    if len(df["threshold"].unique()) == 1:
        fig = px.bar(
            data["mean"][clock_type].reset_index(),
            x="method",
            y=clock_type,
            error_y=error_y if error_bars else None,
            color="method",
            facet_col="description",
            category_orders={
                "description": ("random", "ascending", "descending", "single_num")
            },
            facet_col_wrap=1,
            height=1500,
        )
    else:
        fig = px.line(
            data["mean"][clock_type].reset_index(),
            x="threshold",
            y=clock_type,
            error_y=error_y if error_bars else None,
            color="method",
            markers=True,
            facet_col="description",
            category_orders={
                "description": ("random", "ascending", "descending", "single_num")
            },
            facet_col_wrap=1,
            height=1500,
        )

    fig.update_xaxes(
        showticklabels=True,
    )
    fig.update_yaxes(
        automargin=True,
        matches=None,
        title=f"Runtime ({clock_type})",
    )

    fig.update_layout(
        xaxis_title="Size",
        title={
            "text": f"Threshold vs. Runtime, size = {size}",
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
    return fig


@app.callback(
    Output("cachegrind-figure", "figure"),
    [
        Input("cachegrind-data", "data"),
        Input("cachegrind-options", "value"),
    ],
)
def update_cachegrind_callback(json_df, opts):
    """TODO."""
    if json_df is None:
        raise dash.exceptions.PreventUpdate
    if opts is None:
        opts = []

    df = df_from_json(json_df)

    groups = df.groupby(["method", "size"])
    means = groups.aggregate(np.mean)
    means = means.reset_index()
    means = means.fillna(0)

    MISS_COLS = ["I1mr", "ILmr", "D1mr", "DLmr", "D1mw", "DLmw", "Bcm", "Bim"]

    # # TODO: Globalize this

    # # Associate totals with subgroups
    COL_MAP = {
        "Ir": ("I1mr", "ILmr"),
        "Dr": ("D1mr", "DLmr"),
        "Dw": ("D1mw", "DLmw"),
        "Bc": ("Bcm",),
        "Bi": ("Bim",),
    }

    if "relative" in opts:
        relative = means[["method"] + CACHEGRIND_COLS].copy()
        for total, sub in COL_MAP.items():
            for miss in sub:
                relative[miss] /= relative[total]
                relative[miss] *= 100
        misses = relative[["method"] + MISS_COLS]
        y_axis_title = "Percent"
    else:
        misses = means[["method"] + MISS_COLS]
        y_axis_title = "Number of misses"

    # TODO: Find a more pythonic way to do this.
    bars = []
    means = means.sort_values(["threshold", "method"])
    for i in misses["method"].unique():
        y = misses[misses["method"] == i].reset_index()
        y = y.loc[0][MISS_COLS]
        bars.append(go.Bar(name=i, x=MISS_COLS, y=y))
    fig = go.Figure(bars)

    fig.update_xaxes(
        showticklabels=True,
    )
    fig.update_yaxes(
        title=y_axis_title,
    )
    if "log" in opts:
        fig.update_yaxes(
            automargin=True,
            autorange=True,
            type="log",
            dtick="D2",
        )

    fig.update_layout(
        legend_title_text="Sorting Method",
        title={
            "text": "Mean Cache Performance Across All Types and Thresholds",
            "xanchor": "left",
        },
        font=dict(
            family="Courier New, monospace",
            size=18,
        ),
        legend=dict(yanchor="top", xanchor="left"),
        barmode="group",
    )

    # Fix facet titles
    # TODO: Find a less gross way to do this...
    fig.for_each_annotation(lambda x: x.update(text=x.text.split("=")[-1].capitalize()))

    return fig


@app.callback(
    Output("threshold-vs-cache", "figure"),
    [
        Input("cachegrind-data", "data"),
        Input("cachegrind-options", "value"),
        Input("cachegrind-metric-dropdown", "value"),
        Input("info-data", "data"),
        Input("size-slider", "value"),
    ],
)
def update_threshold_v_cache(json_df, opts, metric, run_info, size=None):
    if json_df is None:
        return px.line()
    if opts is None:
        opts = []

    df = pd.read_json(json_df)
    if size is None or not any(df["size"] == size):
        size = df["size"].min()

    run_info = json.loads(run_info)
    threshold_methods = set(run_info["QST"]["Methods"]["Threshold"])
    non_threshold_dfs = df[~df["method"].isin(threshold_methods)]

    df = df[df["size"] == size]
    df = df[df["threshold"] != 0]

    if df.empty:
        return px.line()

    for method in non_threshold_dfs["method"].unique():
        for _, row in non_threshold_dfs[
            non_threshold_dfs["method"] == method
        ].iterrows():
            row["threshold"] = df["threshold"].min()
            df = df.append(row)
            row["threshold"] = df["threshold"].max()
            df = df.append(row)

    # Associate totals with subgroups
    if "relative" in opts:
        df[metric] /= df[CACHE_MAP[metric]]
        df[metric] *= 100
        y_axis_title = f"Percent of {CACHE_MAP[metric]}"
    else:
        y_axis_title = f"Number of {CACHE_MAP[metric]} misses"

    df = df.sort_values(["threshold", "method"])
    fig = px.line(
        df,
        x=df["threshold"],
        y=df[metric],
        facet_col="description",
        facet_col_wrap=1,
        facet_row_spacing=0.04,
        category_orders={
            "description": ("random", "ascending", "descending", "single_num")
        },
        color=df["method"],
        markers=True,
        height=2000,
    )

    fig.update_xaxes(
        showticklabels=True,
    )
    fig.update_yaxes(
        automargin=True,
        matches=None,
        title=y_axis_title,
    )
    fig.update_layout(
        xaxis_title="Threshold",
        legend_title_text="Sorting Method",
        title={
            "text": f"Threshold vs. {metric}, size = {size:,}",
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
