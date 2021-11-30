from dash import dcc, html
from pathlib import Path

RESULTS_DIR = Path("./results")

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


def gen_layout(units, clocks, data_types, dirs=None):
    if dirs is None:
        dirs = sorted(list(RESULTS_DIR.iterdir()))
    res = html.Div(
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
                                                    html.Button(
                                                        "Load",
                                                        id="load-button",
                                                        n_clicks=0,
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
                                    html.Label(
                                        "Clock Type", style={"margin-top": "5px"}
                                    ),
                                    dcc.RadioItems(
                                        id="clock-type",
                                        options=[
                                            {
                                                "label": i.capitalize(),
                                                "value": i,
                                            }
                                            for i in clocks
                                        ],
                                        value=clocks[0],
                                    ),
                                    html.Label(
                                        "Data Type", style={"margin-top": "5px"}
                                    ),
                                    dcc.RadioItems(
                                        id="data-type",
                                        options=[
                                            {
                                                "label": i.capitalize(),
                                                "value": i,
                                            }
                                            for i in data_types
                                        ],
                                        value=data_types[0],
                                    ),
                                    dcc.Checklist(
                                        id="error-bars-checkbox",
                                        options=[
                                            {
                                                "label": "Error Bars",
                                                "value": "error_bars",
                                            }
                                        ],
                                        style={"margin-top": "5px"},
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
                                    dcc.Slider(
                                        id="threshold-slider", step=None, value=0
                                    ),
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
    return res
