#!/usr/bin/env python3

import argparse
import sys
from pathlib import Path

import matplotlib as mpl
import matplotlib.pyplot as plt
import pandas as pd
import scienceplots

# Yes this is ugly, fix it later.
sys.path.insert(0, "../evaluator")

from mpl import Result, get_avg_df

pd.set_option("display.max_rows", None)
pd.set_option("display.max_columns", None)
pd.set_option("display.width", None)

plt.style.use(["science", "ieee"])
plt.style.use("seaborn-v0_8-paper")
mpl.rcParams["font.size"] = 14
mpl.rcParams["figure.titlesize"] = 14
mpl.rcParams["axes.titlesize"] = 14
mpl.rcParams["axes.labelsize"] = 14
mpl.rcParams["xtick.labelsize"] = 6
mpl.rcParams["ytick.labelsize"] = 12
mpl.rcParams["legend.fontsize"] = 10

mpl.rcParams["errorbar.capsize"] = 3
mpl.rcParams["lines.linewidth"] = 1
mpl.rcParams["lines.markersize"] = 5


def build_parser():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "RESULT_DIRS",
        metavar="DIR",
        action="store",
        nargs="+",
        type=Path,
    )
    parser.add_argument(
        "-o",
        "--output",
        metavar="DIR",
        action="store",
        required=True,
        type=Path,
    )
    return parser


def generate_boxplots(paths: list[Path], output_dir: Path):
    results = [Result(i) for i in paths]

    cols = [
        "method",
        "description",
        # "threshold",
        # "size",
        "wall_nsecs_relative",
        # "host",
    ]

    rename_methods_map = {
        "msort_heap_with_old_ins": "Mergesort w/ Primitive InsSort",
        "msort_heap_with_basic_ins": "Mergesort w/ Basic InsSort",
        "msort_heap_with_shell": "Mergesort w/ ShellSort",
        "msort_heap_with_fast_ins": "Mergesort w/ Fast InsSort",
        "msort_heap_with_network": "Mergesort w/ Fast InsSort \& Network",
        "msort_with_network": "Mergesort w/ Fast InsSort \& Network",
        "quicksort_with_ins": "Quicksort w/ InsSort",
        "quicksort_with_fast_ins": "Quicksort w/ Fast InsSort",
        "fast_ins": "Fast InsSort",
    }
    rename_standard_methods_map = {
        "msort_heap": "Mergesort",
    }
    rename_data_map = {
        "single_num": "Repeated Single Integer",
        "pipe_organ": "Pipe Organ",
    }

    most_improved = pd.DataFrame(columns=cols)

    for r in results:

        r._threshold_methods.update(set(rename_methods_map.values()))
        r._standard_methods.update(set(rename_standard_methods_map.values()))
        r.df = r.df.replace(rename_methods_map)
        r.df = r.df.replace(rename_standard_methods_map)
        r.df = r.df.replace(rename_data_map)

        host = r.job_details.get("Node")
        df = r.df
        df = df[df["method"] != "Quicksort w/ Fast InsSort"]
        df = df[df["method"] != "Quicksort w/ InsSort"]

        baseline_df = df[df["method"] == "qsort"]
        df = df[df["method"] != "qsort"]

        baseline_df = get_avg_df(baseline_df)
        dfs_by_type = r.gen_sub_dfs(df)

        for type_, dfs in dfs_by_type.items():
            den = (
                baseline_df[baseline_df["description"] == type_][("mean", "wall_nsecs")]
                .reset_index(drop=True)
                .mean()
            )
            for method, sub_df in dfs.items():
                num = sub_df[("mean", "wall_nsecs")].reset_index(drop=True)
                relative = num / den
                sub_df["wall_nsecs_relative"] = relative.values * 100

        for type_, dfs in dfs_by_type.items():
            for method, sub_df in dfs.items():
                min_val = sub_df["wall_nsecs_relative"].min()
                min_row = sub_df[sub_df["wall_nsecs_relative"] == min_val].copy()
                min_row["host"] = host
                min_row = min_row[cols]
                min_row.columns = min_row.columns.droplevel(1)
                most_improved = pd.concat([most_improved, min_row], axis=0)

    size = df["size"].unique()[0]
    types = sorted(most_improved["description"].unique())
    for i in types:
        type_df = most_improved[most_improved["description"] == i]
        type_ = i.title()

        title = f"""Runtime Relative to GNU glibc's qsort() Across Various Platforms
        Input size = {size:,}
        {type_}"""

        ax = type_df.boxplot(
            by=["method"],
            grid=False,
        )
        ax.axhline(100, color="red", linestyle="--")

        # Remove default stuff from pandas
        ax.set_xlabel("")
        ax.get_figure().suptitle("")

        # Seet rest of deatils
        ax.set_ylabel("\% of qsort runtime")
        ax.set_title(title)
        ax.get_figure().tight_layout()

    figs = [plt.figure(n) for n in plt.get_fignums()]
    for i, v in enumerate(figs):
        dst = output_dir / f"{i}.gen.png"
        v.savefig(dst, bbox_inches="tight")


if __name__ == "__main__":
    parser = build_parser()
    args = parser.parse_args()
    args = vars(args)

    args["output"].mkdir(exist_ok=True)

    generate_boxplots(args["RESULT_DIRS"], args["output"])
