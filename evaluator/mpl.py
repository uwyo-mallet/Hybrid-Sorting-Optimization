#!/usr/bin/env python3
import json
import logging
import sys
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path
from pprint import pprint
from typing import Optional

import matplotlib as mpl
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import numpy as np
import pandas as pd
import scienceplots

pd.set_option("display.max_rows", None)
pd.set_option("display.max_columns", None)
pd.set_option("display.width", None)

plt.style.use(["science", "ieee"])
plt.style.use("seaborn-v0_8-paper")
# plt.rcParams.update({"figure.dpi": "100"})
mpl.rcParams["errorbar.capsize"] = 3
mpl.rcParams["lines.linewidth"] = 1


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
    job_details: dict
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
        pprint(self.job_details)

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
            all_methods = set(self.job_details["Executable"]["Methods"]["All"])
            self._threshold_methods = set(
                self.job_details["Executable"]["Methods"]["Threshold"]
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

    def _plot_threshold_v_runtime(
        self,
        dfs,
        baseline_dfs,
        min_threshold=0,
        max_threshold=0,
    ):
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
            axes[index].xaxis.set_major_locator(ticker.MaxNLocator(integer=True))
            index += 1
        plt.tight_layout()
        return fig, axes

    def _plot_size_v_runtime(self, dfs):
        fig, axes = plt.subplots(nrows=len(dfs), figsize=(16, 12))
        index = 0
        for type_, sub_df in dfs.items():
            for method, df in sub_df.items():
                yerr = list(df[("std", "wall_secs")])
                df.plot.line(
                    x="size",
                    y=("mean", "wall_secs"),
                    marker="o",
                    yerr=yerr,
                    title=type_.capitalize(),
                    ax=axes[index],
                    label=method,
                )
            index += 1
        plt.tight_layout()
        return fig, axes

    def _prompt_for_thing(self, thing, things):
        pprint(things)
        while picked := input(f"Please select a {thing}: ") not in things:
            print("Invalid size, please pick from the list printed above.")

        return picked

    def plot_threshold_v_runtime(self, interactive=False):
        """TODO."""
        standard_data = self.df.query("method in @self._standard_methods")
        threshold_data = self.df.query("method in @self._threshold_methods")

        min_threshold = threshold_data["threshold"].min()
        max_threshold = threshold_data["threshold"].max()

        sizes = threshold_data["size"].unique()
        if len(sizes) > 1:
            if interactive:
                size = self._prompt_for_thing("size", list(sizes))
            else:
                size = sizes[-1]
        else:
            size = sizes[0]

        standard_data = standard_data[standard_data["size"] == size]
        threshold_data = threshold_data[threshold_data["size"] == size]
        standard_dfs = self._gen_sub_dfs(standard_data)
        threshold_dfs = self._gen_sub_dfs(threshold_data)

        fig, axes = self._plot_threshold_v_runtime(
            threshold_dfs,
            standard_dfs,
            min_threshold=min_threshold,
            max_threshold=max_threshold,
        )
        fig.suptitle(
            f"Threshold vs. Runtime (size={size:,}, host={self.job_details['Node']}, arch={self.job_details['Machine']})",
            fontsize=16,
        )
        fig.tight_layout()

    def plot_size_v_runtime(self, interactive=False):
        """TODO."""
        df = self.df
        thresholds = self.df["threshold"].unique()
        if len(thresholds) > 1:
            if interactive:
                threshold = self._prompt_for_thing("threshold", list(thresholds))
            else:
                threshold = thresholds[-1]
            df = df[(df["threshold"] == threshold) | (df["threshold"] == 0)]
        else:
            threshold = thresholds[0]
            df = df[(df["threshold"] == threshold) | (df["threshold"] == 0)]

        dfs = self._gen_sub_dfs(df)

        fig, axes = self._plot_size_v_runtime(dfs)
        fig.suptitle("Size vs. Runtime", fontsize=16)
        fig.tight_layout()

    def plot_relative_difference(self, baseline_method, interactive=False):
        baseline_df = self.df[self.df["method"] == baseline_method]
        baseline_df = self.df[self.df["method"] != baseline_method]
        df = baseline_df.query("method in @self._threshold_methods").copy()

        other_baselines = baseline_df.query("method in @self._standard_methods").copy()
        other_baselines = self._gen_sub_dfs(other_baselines)

        min_threshold = df["threshold"].min()
        max_threshold = df["threshold"].max()

        sizes = df["size"].unique()
        if len(sizes) > 1:
            if interactive:
                size = self._prompt_for_thing("size", list(sizes))
            else:
                size = sizes[-1]
        else:
            size = sizes[0]
        df = df[df["size"] == size]
        dfs = self._gen_sub_dfs(df)

        for type_, sub_df in dfs.items():
            fig = plt.figure()
            ax = fig.subplots()
            # den = baseline_df["wall_nsecs"].reset_index(drop=True).mean()
            den = (
                baseline_df[baseline_df["description"] == type_]["wall_nsecs"]
                .reset_index(drop=True)
                .mean()
            )
            for method, df in sub_df.items():
                num = df[("mean", "wall_nsecs")].reset_index(drop=True)
                relative = num / den
                df["wall_nsecs_relative"] = relative.values

                df.plot.line(
                    x="threshold",
                    y="wall_nsecs_relative",
                    marker="o",
                    title=type_.title(),
                    ax=ax,
                    label=method,
                )
            ax.plot([0, max_threshold], [1, 1], "--", label=baseline_method)
            for v in other_baselines[type_].values():
                row = v.iloc[0]
                val = row[("mean", "wall_nsecs")]
                # I have no idea why this is a series, there's a bug somewhere
                label = row["method"].values[0]
                relative = val / den
                ax.plot([0, max_threshold], [relative, relative], "--", label=label)

            ax.legend(
                title="Method",
                bbox_to_anchor=(0.96, -0.03),
                bbox_transform=fig.transFigure,
                loc="lower right",
                ncol=1,
                fancybox=True,
            )
            fig.tight_layout(pad=5)

            t = f"""CPU: {self.job_details['CPU Info']['brand_raw'].split("@")[0]}
            CPU Count: {self.job_details['CPU Info']['count']}
            CPU Frequency: {self.job_details['CPU Info']['hz_actual_friendly']}
            Arch: {self.job_details['CPU Info']['arch_string_raw']}
            Compiler: gcc 12.2.1
            CFlags: -O3
            """
            fig.text(
                0.1,
                0,
                t,
                # transform=fig.transFigure,
            )
            ax.set_xlabel("Threshold")
            # Escape % since it is the comment char of latex
            ax.set_ylabel(f"\% of {baseline_method} runtime")
            ax.xaxis.set_major_locator(ticker.MaxNLocator(integer=True))

            fig.suptitle(
                f"""Threshold vs. Runtime Relative to GNU glibc's {baseline_method}
                (Input size=${size:,}$)""",
            )
            fig.tight_layout()


def get_fig_size(width=440, fraction=1):
    """Set figure dimensions to avoid scaling in LaTeX.

    Source: https://jwalton.info/Embed-Publication-Matplotlib-Latex/
    """
    # Width of figure (in pts)
    fig_width_pt = width * fraction

    # Convert from pt to inches
    inches_per_pt = 1 / 72.27

    # Golden ratio to set aesthetic figure height
    # https://disq.us/p/2940ij3
    golden_ratio = (5**0.5 - 1) / 2

    # Figure height/width in inches
    fig_width_in = fig_width_pt * inches_per_pt
    fig_height_in = fig_width_in * golden_ratio

    fig_dim = (fig_width_in, fig_height_in)

    return fig_dim


def gen_report_plots(result: Result):
    """Generate publication quality plots."""
    rename_methods_map = {
        "msort_heap_with_old_ins": "Mergesort with Primitive InsSort",
        "msort_heap_with_basic_ins": "Mergesort with Basic InsSort",
        "msort_heap_with_shell": "Mergesort with ShellSort",
        "msort_heap_with_fast_ins": "Mergesort with Fast InsSort",
        "msort_heap_with_network": "Mergesort with Fast InsSort and Network",
        "msort_with_network": "Mergesort with Fast InsSort and Network",
        "quicksort_with_ins": "Quicksort with InsSort",
        "quicksort_with_fast_ins": "Quicksort with Fast InsSort",
        "fast_ins": "Fast InsSort",
    }
    rename_standard_methods_map = {
        "msort_heap": "Mergesort",
    }
    rename_data_map = {
        "single_num": "Repeated Single Integer",
        "pipe_organ": "Pipe Organ",
    }
    result._threshold_methods.update(set(rename_methods_map.values()))
    result._standard_methods.update(set(rename_standard_methods_map.values()))
    result.df = result.df.replace(rename_methods_map)
    result.df = result.df.replace(rename_standard_methods_map)
    result.df = result.df.replace(rename_data_map)
    plots_dir = result.path / "plots"
    plots_dir.mkdir(exist_ok=True)

    # result.plot_threshold_v_runtime()
    # result.plot_size_v_runtime()
    result.plot_relative_difference("qsort")

    figs = [plt.figure(n) for n in plt.get_fignums()]
    for i, v in enumerate(figs):
        dst = plots_dir / f"{i}.gen.png"
        v.savefig(dst, bbox_inches="tight")


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

    if len(sys.argv) > 1:
        result = Result(Path(sys.argv[1]))
    else:
        base_results_dir = Path("./results")
        if not base_results_dir.is_dir():
            raise NotADirectoryError(f"'{base_results_dir}' is not a directory")

        last_result_path = get_latest_subdir(base_results_dir)
        result = Result(last_result_path)

    gen_report_plots(result)
    # result.plot_threshold_v_runtime()
    # result.plot_size_v_runtime()
    # result.plot_relative_difference("qsort")

    # plt.show()


if __name__ == "__main__":
    main()
