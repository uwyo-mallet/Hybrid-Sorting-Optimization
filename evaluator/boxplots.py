#!/usr/bin/env python3

import argparse
import sys
from pathlib import Path

import matplotlib as mpl
import matplotlib.pyplot as plt
import pandas as pd

# Yes this is ugly, fix it later.
sys.path.insert(0, "../evaluator")

from mpl import Result, get_avg_df


def build_parser():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "RESULT_DIRS",
        metavar="DIR",
        action="store",
        nargs="+",
        type=Path,
    )
    return parser


def generate_boxplots(paths):
    results = [Result(i) for i in paths]

    cols = [
        "method",
        "description",
        # "threshold",
        # "size",
        "wall_nsecs_relative",
        # "host",
    ]
    most_improved = pd.DataFrame(columns=cols)

    for r in results:
        host = r.job_details.get("Node")
        df = r.df
        df = df[df["method"] != "quicksort_with_ins"]
        df = df[df["method"] != "quicksort_with_fast_ins"]

        baseline_df = df[df["method"] == "qsort"]
        df = df[df["method"] != "qsort"]

        baseline_df = get_avg_df(baseline_df)
        # df = get_avg_df(df)

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

    types = sorted(most_improved["description"].unique())
    # methods = sorted(most_improved["method"].unique())
    # for i in types:
    #     type_df = most_improved[most_improved["description"] == i]
    # most_improved[]
    # print(most_improved.dtypes)
    print(most_improved)
    for i in types:
        type_df = most_improved[most_improved["description"] == i]
        type_df.boxplot(by=["method", "description"])
    # most_improved.boxplot(column=["method", "description"], by="wall_nsecs_relative")

    plt.show()

    # for m in methods:
    #     method_df = type_df[type_df["method"] == m]


if __name__ == "__main__":
    parser = build_parser()
    args = parser.parse_args()
    args = vars(args)

    generate_boxplots(args["RESULT_DIRS"])
