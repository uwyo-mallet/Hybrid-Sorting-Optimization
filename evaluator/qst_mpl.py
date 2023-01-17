#!/usr/bin/env python3
import json
import logging
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path
from pprint import pprint
from typing import Optional

import matplotlib as mpl
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

# pd.set_option("display.max_rows", None)
pd.set_option("display.max_columns", None)
pd.set_option("display.width", None)

plt.style.use("ggplot")
mpl.rcParams["errorbar.capsize"] = 3


def get_latest_subdir(path: Path) -> Path:
    """Get the last subdirectory within a directory."""
    dirs = sorted(list(path.iterdir()))
    try:
        return dirs[-1]
    except IndexError as e:
        raise NotADirectoryError(f"No subdirectory within {str(path)}") from e


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

        # Convert from nanoseconds to seconds
        time_columns = [i for i in list(self.df.columns) if i.endswith("_nsecs")]
        for i in time_columns:
            new_name = i.split("_nsecs")[0] + "_secs"
            self.df[new_name] = self.df[i] / 1_000_000_000
        self.cds = {}

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

    def _gen_sub_dfs(self, df):
        types = sorted(df["description"].unique())
        methods = sorted(df["method"].unique())
        dfs = defaultdict(dict)
        for i in types:
            type_df = df[df["description"] == i]
            type_df = get_avg_df(type_df)
            for m in methods:
                method_df = type_df[type_df["method"] == m]
                dfs[i][m] = method_df

        # Follow the following order for types
        return dfs

    def _plot(self, dfs, baseline_dfs, min_threshold=0, max_threshold=0):
        fig, axes = plt.subplots(nrows=len(dfs), figsize=(16, 12))
        index = 0
        for type_, sub_df in dfs.items():
            for method, df in sub_df.items():
                yerr = list(df[("std", "wall_secs")])
                df.plot.line(
                    x="threshold",
                    y=("mean", "wall_secs"),
                    marker="o",
                    yerr=yerr,
                    title=type_.capitalize(),
                    ax=axes[index],
                    label=method,
                )

            for method, df in baseline_dfs[type_].items():
                if len(df) > 1:
                    logging.warning("Baseline df has more than one entry, using first")
                elif len(df) < 1:
                    msg = "Baseline df has no entries"
                    logging.error(msg)
                    continue
                y = df.iloc[0][("mean", "wall_secs")]
                axes[index].plot([0, max_threshold], [y, y], "--", label=method)

            axes[index].legend(loc="upper right")
            axes[index].set_ylabel("Wall secs")
            index += 1
        plt.tight_layout()
        return fig, axes

    def _prompt_for_size(self, sizes):
        pprint(sizes)
        while picked := input("Please select a size: ") not in sizes:
            print("Invalid size, please pick from the list printed above.")

        return picked

    def plot(self, interactive=False):
        """TODO."""
        standard_data = self.df.query("method in @self._standard_methods")
        threshold_data = self.df.query("method in @self._threshold_methods")

        min_threshold = threshold_data["threshold"].min()
        max_threshold = threshold_data["threshold"].max()

        sizes = threshold_data["size"].unique()
        if len(sizes) > 1:
            if interactive:
                size = self._prompt_for_size(list(sizes))
            else:
                size = sizes[-1]
        else:
            size = sizes[0]

        standard_data = standard_data[standard_data["size"] == size]
        threshold_data = threshold_data[threshold_data["size"] == size]
        standard_dfs = self._gen_sub_dfs(standard_data)
        threshold_dfs = self._gen_sub_dfs(threshold_data)

        fig, axes = self._plot(
            threshold_dfs,
            standard_dfs,
            min_threshold=min_threshold,
            max_threshold=max_threshold,
        )
        fig.suptitle(f"Threshold vs. Runtime (size = {size:,})", fontsize=16)
        fig.tight_layout()

        plt.show()


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


if __name__ == "__main__":
    main()
