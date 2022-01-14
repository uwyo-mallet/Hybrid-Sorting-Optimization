from dash import dcc, html
from .generics import QST_RESULTS_DIR


def gen_layout(clocks, data_types, dirs=None):
    if dirs is None:
        dirs = sorted(list(QST_RESULTS_DIR.iterdir()))
    res = html.Div(
        [
            dcc.Store(id="stat-data"),
            dcc.Store(id="cachegrind-data"),
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
                                        id="threshold-slider",
                                        step=None,
                                        value=0,
                                        tooltip={
                                            "placement": "bottom",
                                            "always_visible": True,
                                        },
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
                                    dcc.Slider(
                                        id="size-slider",
                                        step=None,
                                        value=0,
                                        tooltip={
                                            "placement": "bottom",
                                            "always_visible": True,
                                        },
                                    ),
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
            html.Div(
                [
                    html.Div(
                        [
                            html.P(
                                "Options",
                                className="control_label",
                                style={
                                    "fontWeight": "bold",
                                    "textAlign": "center",
                                },
                            ),
                            dcc.Checklist(
                                id="cachegrind-options",
                                options=[
                                    {
                                        "label": "Relative",
                                        "value": "relative",
                                    },
                                    {
                                        "label": "Log Scale",
                                        "value": "log",
                                    },
                                ],
                                value=["log"],
                                labelStyle={"display": "inline-block"},
                                style={"margin-top": "5px"},
                            ),
                        ],
                        className="pretty_container",
                    ),
                    html.Div(
                        [
                            dcc.Graph(
                                id="cachegrind-figure",
                            ),
                        ]
                    ),
                    dcc.Dropdown(
                        id="cachegrind-metric-dropdown",
                        options=[
                            {
                                "label": str(i),
                                "value": str(i),
                            }
                            for i in [
                                "I1mr",
                                "ILmr",
                                "D1mr",
                                "DLmr",
                                "D1mw",
                                "DLmw",
                                "Bcm",
                                "Bim",
                            ]
                        ],
                        value="I1mr",
                    ),
                    html.Div(
                        [
                            dcc.Graph(
                                id="threshold-vs-cache",
                            ),
                        ]
                    ),
                ],
                className="row pretty_container",
            ),
        ],
        id="mainContainer",
        style={"display": "flex", "flexDirection": "column"},
    )
    return res
