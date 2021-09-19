#!/usr/bin/env python3

import json
from dataclasses import dataclass
from pathlib import Path

import dash
import dash_core_components as dcc
import dash_html_components as html

# import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import plotly.express as px

RESULTS_DIR = Path("./results")
dirs = list(RESULTS_DIR.iterdir())
dirs.sort()

THRESHOLD_METHODS = (
    "qsort_asm",
    "qsort_c",
    "qort_c_improved",
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
    partition: str


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

    return Result(raw_df, stat_df, info, partition)


# external_stylesheets = ["https://codepen.io/chriddyp/pen/bWLwgP.css"]
# app = dash.Dash(__name__, external_stylesheets=external_stylesheets)
app = dash.Dash(__name__)

"""
app.layout = html.Div(
    [
        html.Div(
            [
                html.Div(
                    [
                        dcc.Dropdown(
                            id="result-dropdown",
                            options=[{"label": str(i), "value": str(i)} for i in dirs],
                            value=str(dirs[-1]),
                        ),
                        dcc.RadioItems(
                            id="time-unit",
                            options=[
                                {"label": i.capitalize(), "value": i}
                                for i in UNITS.keys()
                            ],
                            value=tuple(UNITS.keys())[0],
                            labelStyle={"display": "inline-block"},
                        ),
                    ],
                    style={"width": "49", "display": "inline-block"},
                ),
                # html.Div(
                #     [
                #         dcc.Dropdown(
                #             id="crossfilter-yaxis-column",
                #             options=[{"label": i, "value": i} for i in [1, 2, 3]],
                #             value="Life expectancy at birth, total (years)",
                #         ),
                #         dcc.RadioItems(
                #             id="crossfilter-yaxis-type",
                #             options=[
                #                 {"label": i, "value": i} for i in ["Linear", "Log"]
                #             ],
                #             value="Linear",
                #             labelStyle={"display": "inline-block"},
                #         ),
                #     ],
                #     style={"width": "49%", "float": "right", "display": "inline-block"},
                # ),
            ],
            style={
                "borderBottom": "thin lightgrey solid",
                "backgroundColor": "rgb(250, 250, 250)",
                "padding": "10px 5px",
            },
        ),
        html.Div(
            [
                dcc.Slider(
                    id="size-vs-runtime-slider",
                ),
                dcc.Graph(
                    id="size-vs-runtime-scatter",
                    # hoverData={"points": [{"customdata": "Japan"}]},
                ),
            ],
            style={"width": "98%", "display": "inline-block", "padding": "0 20"},
        ),
        # html.Div(
        #     [
        #         dcc.Graph(id="x-time-series"),
        #         dcc.Graph(id="y-time-series"),
        #     ],
        #     style={"display": "inline-block", "width": "49%"},
        # ),
        # html.Div(
        #     dcc.Slider(
        #         id="crossfilter-year--slider",
        #         min=df["Year"].min(),
        #         max=df["Year"].max(),
        #         value=df["Year"].max(),
        #         marks={str(year): str(year) for year in df["Year"].unique()},
        #         step=None,
        #     ),
        #     style={"width": "49%", "padding": "0px 20px 20px 20px"},
        # ),
    ]
)
"""

app.layout = html.Div(
    [
        dcc.Store(id="aggregate_data"),
        # empty Div to trigger javascript file for graph resizing
        html.Div(id="output-clientside"),
        html.Div(
            [
                html.Div(
                    [
                        html.H6(
                            "Info",
                            style={
                                "margin-top": "0",
                                "font-weight": "bold",
                                "text-align": "center",
                            },
                        ),
                        html.P(
                            "The cultures and customs of the 27 EU countries differ widely. The same applies to their eating and drinking habits."
                            " The map on the right explores the food supply in kilograms per capita per year.",
                            className="control_label",
                            style={"text-align": "justify"},
                        ),
                        html.P(),
                        html.P(
                            "Select a food category",
                            className="control_label",
                            style={"text-align": "center", "font-weight": "bold"},
                        ),
                        # radio_food_behaviour,
                    ],
                    className="pretty_container four columns",
                    id="cross-filter-options",
                    style={"text-align": "justify"},
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
                                                                "margin-right": "1em",
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
                                                "text-align": "left",
                                                "vertical-align": "baseline",
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
                                html.Label("Threshold"),
                                dcc.Slider(
                                    id="threshold-slider",
                                    min=0,
                                    max=1000,
                                    value=0,
                                ),
                            ],
                            className="pretty_container",
                            # style={"columnCount": 1},
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
                dcc.Graph(
                    id="size-vs-runtime-scatter",
                ),
            ],
            className="row pretty_container",
        ),
        html.Div(
            [
                html.Div(
                    # cor_behav,
                    [
                        html.H6(
                            "Exploring correlations",
                            style={
                                "margin-top": "0",
                                "font-weight": "bold",
                                "text-align": "center",
                            },
                        ),
                        html.P(
                            "In the heatmap below, the correlations between the 5 health variables and the 18 food variables can be explored.",
                            className="control_label",
                            style={"text-align": "justify"},
                        ),
                        # html.P("""<br>"""),
                        html.P(
                            "Select a health variable",
                            style={"font-weight": "bold", "text-align": "center"},
                        ),
                        # cor_behav,
                        html.Div(
                            [dcc.Graph(id="cor_ma")],
                            className="pretty_container twelve columns",
                        ),
                    ],  # ,cor_behav,
                    className="pretty_container four columns",
                ),
                html.Div(
                    [
                        html.H6(
                            "Analysing the correlations between food consumption and health",
                            style={
                                "margin-top": "0",
                                "font-weight": "bold",
                                "text-align": "center",
                            },
                        ),
                        html.P(
                            "Below, the correlations can be analysed in more detail. It is important to note that correlation in this case does not necessarily mean causation. For example, the wealth level of a country might often influence the variables, which is why it is represented through the size of the dots as GDP per capita. Furthermore, outliers should also be watched out for.",
                            className="control_label",
                            style={"text-align": "justify"},
                        ),
                        html.Div(
                            [
                                html.P(
                                    "Select a food category",
                                    className="control_label",
                                    style={
                                        "font-weight": "bold",
                                        "text-align": "center",
                                    },
                                ),
                                dcc.Dropdown(
                                    id="xaxis-column",
                                    options=[
                                        {"label": i, "value": i} for i in [1, 2, 3, 4]
                                    ],
                                    value="Alcoholic Beverages",  # ,className="pretty_container four columns",
                                ),
                                dcc.RadioItems(
                                    id="xaxis-type",
                                    options=[
                                        {"label": i, "value": i}
                                        for i in ["Box", "Violin"]
                                    ],
                                    value="Box",
                                    labelStyle={
                                        "display": "inline-block"
                                    },  # ,className="pretty_container four columns",
                                    style={"padding-left": "34%"},
                                ),
                            ],
                            className="pretty_container sixish columns",
                        ),
                        html.Div(
                            [
                                html.P(
                                    "Select a health variable",
                                    className="control_label",
                                    style={
                                        "font-weight": "bold",
                                        "text-align": "center",
                                    },
                                ),
                                dcc.Dropdown(
                                    id="yaxis-column",
                                    options=[
                                        {"label": i, "value": i} for i in [1, 2, 2]
                                    ],
                                    value=1,
                                ),
                                dcc.RadioItems(
                                    id="yaxis-type",
                                    options=[
                                        {"label": i, "value": i}
                                        for i in ["Box", "Violin"]
                                    ],
                                    value="Box",
                                    labelStyle={"display": "inline-block"},
                                    style={"padding-left": "34%"},
                                ),
                            ],
                            className="pretty_container sixish columns",
                        ),
                        html.Div(
                            [
                                dcc.Graph(id="indicator-graphic"),
                            ],
                            className="pretty_container almost columns",
                        ),
                    ],
                    className="pretty_container eight columns",
                ),
            ],
            className="row flex-display",
        ),
        html.Div(
            [
                html.H6(
                    "K-means clustering",
                    style={
                        "margin-top": "0",
                        "font-weight": "bold",
                        "text-align": "center",
                    },
                ),
                html.P(
                    "Finally, k-means clustering is carried out. The criterion can be selected on the left side, whereby either the 18 food variables, the 5 health variables or all of them in combination may be chosen for the clustering. On the right side, the resulting clusters can then be compared with respect to a chosen variable.",
                    className="control_label",
                    style={"text-align": "justify"},
                ),
                html.Div(
                    [
                        html.P(
                            "Select a clustering criterion",
                            className="control_label",
                            style={"text-align": "center", "font-weight": "bold"},
                        ),
                        # radio_clust_behaviour,
                        dcc.Graph(id="cluster_map"),
                    ],
                    className="pretty_container sixish columns",
                ),
                html.Div(
                    [
                        html.P(
                            "Select a variable for cluster comparison",
                            className="control_label",
                            style={"text-align": "center", "font-weight": "bold"},
                        ),
                        # ots_behaviour,
                        dcc.Graph(id="boxes"),
                    ],
                    className="pretty_container sixish columns",
                ),
            ],
            className="row pretty_container",
        ),
        html.Div(
            [
                html.H6(
                    "Authors",
                    style={
                        "margin-top": "0",
                        "font-weight": "bold",
                        "text-align": "center",
                    },
                ),
                html.P(
                    "fOO",
                    style={"text-align": "center", "font-size": "10pt"},
                ),
            ],
            className="row pretty_container",
        ),
        html.Div(
            [
                html.H6(
                    "Sources",
                    style={
                        "margin-top": "0",
                        "font-weight": "bold",
                        "text-align": "center",
                    },
                ),
                dcc.Markdown(
                    """\
                         - Eurostat: https://ec.europa.eu/eurostat/databrowser/view/HLTH_SHA11_HF__custom_227597/bookmark/table?lang=en&bookmarkId=1530a1e6-767e-4661-9e15-0ed2f7fae0d5
                         - Food and Agriculture Organization of the United Nations: http://www.fao.org/faostat/en/#data/FBS
                         - Opendatasoft: https://data.opendatasoft.com/explore/dataset/european-union-countries@public/export/
                         - Our World in Data: https://covid.ourworldindata.org/data/owid-covid-data.csv?v=2021-03-11
                        """,
                    style={"font-size": "10pt"},
                ),
            ],
            className="row pretty_container",
        ),
    ],
    id="mainContainer",
    style={"display": "flex", "flex-direction": "column"},
)


@app.callback(
    dash.dependencies.Output("size-vs-runtime-scatter", "figure"),
    [
        dash.dependencies.Input("result-dropdown", "value"),
        dash.dependencies.Input("time-unit", "value"),
        # dash.dependencies.Input("threshold-slider", "value"),
        # dash.dependencies.Input("crossfilter-yaxis-column", "value"),
        # dash.dependencies.Input("crossfilter-xaxis-type", "value"),
        # dash.dependencies.Input("crossfilter-yaxis-type", "value"),
        # dash.dependencies.Input("crossfilter-year--slider", "value"),
    ],
)
def update_graph(result_dir, time_unit, threshold=4):
    res = load(Path(result_dir))
    df = res.stat_df
    df = df[(df["threshold"] == threshold) | (df["threshold"] == 0)]

    fig = px.line(
        df,
        x=df["size"],
        y=list(df[(UNITS[time_unit], "mean")]),
        error_y=list(df[(UNITS[time_unit], "std")]),
        facet_col="description",
        facet_col_wrap=1,
        facet_row_spacing=0.04,
        category_orders={"description": list(sorted(df["description"].unique()))},
        color=df["method"],
        markers=True,
        labels={"x": "Size", "y": f"Runtime ({time_unit})"},
        height=2000,
    )

    fig.update_xaxes(showticklabels=True, title="Input Size")
    fig.update_yaxes(automargin=True, matches=None)
    fig.update_layout(
        legend_title_text="Sorting Method",
        title={
            # "x": 0.5,
            # "y": 0.9,
            "text": f"Size vs. Runtime, threshold = {threshold}",
            "xanchor": "left",
            # "yanchor": "top",
        },
    )
    fig.for_each_annotation(lambda x: x.update(text=x.text.split("=")[-1].capitalize()))

    return fig


if __name__ == "__main__":
    res = load()

    df = res.stat_df
    # df = df[df["method"] == "qsort_c"]
    # df = df[df["description"] == "random"]
    df = df[df["threshold"] == 4]

    print(df)

    # fig = px.line(
    #     df,
    #     x=df["size"],
    #     y=list(df[("elapsed_usecs", "mean")]),
    #     error_y=list(df[("elapsed_usecs", "std")]),
    #     color=df["method"],
    #     symbol=df["method"],
    #     labels={"x": "Size", "y": "Runtime (microseconds)"},
    # )
    # fig.show()

    # fig = px.line(
    #     df,
    #     x=df["size"],
    #     y=list(df[("elapsed_usecs", "mean")]),
    #     error_y=list(df[("elapsed_usecs", "std")]),
    #     color=df["method"],
    #     symbol=df["method"],
    #     labels={"x": "Size", "y": "Runtime (microseconds)"},
    #     facet_col="description",
    #     facet_col_wrap=1,
    #     height=2500,
    # )
    # fig.show()

    app.run_server(debug=True)
