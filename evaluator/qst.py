"""QST result viewer."""
import itertools
import json
import logging
from dataclasses import dataclass
from pathlib import Path
from pprint import pprint
from typing import Optional

import numpy as np
import pandas as pd
from bokeh.io import output_file
from bokeh.layouts import column, gridplot
from bokeh.models import ColumnDataSource, Div, RangeSlider, Spinner
from bokeh.palettes import Spectral4
from bokeh.plotting import figure, show
from bokeh.transform import factor_cmap

# TODO: Add cachegrind / callgrind / massif support.

# pd.set_option("display.max_rows", None)
pd.set_option("display.max_columns", None)
pd.set_option("display.width", None)


def get_avg_df(df: pd.DataFrame) -> pd.DataFrame:
    """TODO."""
    pivot_columns = [
        "method",
        "description",
        "run_type",
        "threshold",
        "size",
    ]
    df = df.sort_index(axis=1)
    df = df.pivot_table(index=pivot_columns, aggfunc=[np.mean, np.std])

    df = df.reset_index()
    df = df.sort_values(by=["size", "threshold"])
    return df


@dataclass
class Result:
    """TODO."""

    path: Path
    df: pd.DataFrame
    job_details: Optional[dict]
    partition: Optional[str]

    _standard_methods: list[str]
    _threshold_methods: list[str]

    def __init__(self, p: Path) -> None:
        """TODO."""
        self.path = p
        self.df = pd.DataFrame()
        self.job_details = {}
        self.partition = None

        self._standard_methods = []
        self._threshold_methods = []

        self._load_from_disk()

    def _load_from_disk(self):
        """Load all the necessary data from disk."""
        if not self.path.is_dir():
            raise NotADirectoryError(f"'{self.path}' is not a directory")

        # Load df
        csvs = tuple(self.path.glob("output*.csv"))
        if not csvs:
            raise FileNotFoundError(f"No CSV files found in '{self.path}'")
        in_csv = Path(csvs[0])
        self.df = pd.read_csv(in_csv)
        # Drop unnecessary columns
        self.df = self.df.drop(["input", "id"], axis=1)

        # Load job details
        job_details_path = self.path / "job_details.json"
        if job_details_path.is_file():
            with open(job_details_path, "r") as fp:
                self.job_details = json.load(fp)
            # Parse the methods
            all_methods = set(self.job_details["QST"]["Methods"]["All"])
            self._threshold_methods = set(
                self.job_details["QST"]["Methods"]["Threshold"]
            )
            self._standard_methods = all_methods - self._threshold_methods
        else:
            logging.warning("'%s' does not exist.", str(job_details_path))

        # Load partition
        partition_path = self.path / "partition"
        if partition_path.is_file():
            self.partition = partition_path.read_text().strip()
        else:
            logging.info("'%s' does not exist.", str(partition_path))

    @staticmethod
    def _create_grid_plots(
        df,
        types,
        num_lines,
        x,
        y,
        title=None,
        height=400,
        width=1000,
    ):
        tools = "pan,wheel_zoom,box_zoom,reset,hover,save"
        tooltips = [
            ("method", "@method"),
            ("(x,y)", "($x, $y)"),
            ("radius", "@radius"),
            ("fill color", "$color[hex, swatch]:colors"),
        ]

        plots = []
        methods = sorted(df["method"].unique())
        for i in types:
            type_df = df[df["description"] == i]
            type_df = get_avg_df(type_df)

            fig = figure(
                height=height,
                # width=width,
                sizing_mode="stretch_width",
                background_fill_color="#fafafa",
                title=i,
                tools=tools,
                tooltips=tooltips,
            )

            colors = itertools.cycle(Spectral4)
            for m, c in zip(methods, colors):
                method_df = type_df[type_df["method"] == m]
                # Ensure the data is sorted by the correct axis `rstrip` is a
                # consequence of how bokeh handles pandas multiindex.
                method_df = method_df.sort_values(by=x.rstrip("_"))

                fig.circle(
                    x,
                    y,
                    legend_label=m,
                    color=c,
                    source=method_df,
                )
                fig.line(x, y, legend_label=m, line_color=c, source=method_df)
                # fig.hover.renderers = [circle]

            fig.legend.click_policy = "hide"
            plots.append([fig])

        return plots

    def add_widgets(self):
        """TODO."""
        # threshold_slider =
        pass

    def plot(self, title=None):
        """TODO."""
        result_path = self.path / "plot.html"
        if title is None:
            title = "Sorting Optimization Results"
        output_file(result_path, title=title)

        # Prep the data
        standard_data = self.df.query("method in @self._standard_methods")
        # TODO: Filter standard data by desired threshold
        threshold_data = self.df.query("method in @self._threshold_methods")
        # TODO: Filter threshold data by desired size

        standard_plots = self._create_grid_plots(
            standard_data,
            sorted(standard_data["description"].unique()),
            num_lines=len(standard_data["method"].unique()),
            x="size_",
            y="mean_wall_nsecs",
        )
        threshold_plots = self._create_grid_plots(
            threshold_data,
            sorted(threshold_data["description"].unique()),
            num_lines=len(threshold_data["method"].unique()),
            x="threshold_",
            y="mean_wall_nsecs",
        )

        show(gridplot(standard_plots + threshold_plots))
        return (standard_plots, threshold_plots)


def get_latest_subdir(path: Path) -> Path:
    """Get the last subdirectory within a directory."""
    dirs = sorted(list(path.iterdir()))
    try:
        return dirs[-1]
    except IndexError as e:
        raise NotADirectoryError(f"No subdirectory within {str(path)}") from e


def main():
    """TODO."""
    # Setup the logger
    logger = logging.getLogger(__name__)

    formatter = logging.Formatter(
        "%(asctime)s - %(name)s :: %(levelname)-8s :: %(message)s"
    )
    ch = logging.StreamHandler()
    ch.setLevel(logging.DEBUG)
    ch.setFormatter(formatter)
    logger.addHandler(ch)

    base_results_dir = Path("./results")
    if not base_results_dir.is_dir():
        raise NotADirectoryError(f"'{base_results_dir}' is not a directory")

    last_result_path = get_latest_subdir(base_results_dir)
    result = Result(last_result_path)

    result.plot()
