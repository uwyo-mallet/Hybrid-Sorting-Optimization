#!/usr/bin/env python3
"""A small helper to automate job submission on ARCC."""

import argparse
import json
import subprocess
from dataclasses import dataclass

JOB_SBATCH = "jobs.sbatch"


@dataclass
class Partition:
    """Represent a single partition on ARCC."""

    name: str
    constraint: list[str]

    def cmds(self, args) -> list[list[str]]:
        """Convert a set of arguments to a list of sbatch commands."""
        result = []
        for c in self.constraint:
            partition = json.dumps({"partition": self.name, "constraint": c})
            cmd = [
                "sbatch",
                f"--partition={self.name}",
                f"--constraint={c}",
                JOB_SBATCH,
                *args,
                f"--partition='{partition}'",
            ]

            result.append(cmd)

        return result


partitions = (
    Partition("beartooth", ["icelake"]),
    Partition("moran", ["sandy", "ivy"]),
    Partition("moran-bigmem", ["haswell"]),
    Partition("teton", ["broadwell"]),
    Partition("teton-cascade", ["cascade"]),
    Partition("teton-massmem", ["epyc"]),
    Partition("teton-knl", ["teton-knl"]),
)
partitions = {p.name: p for p in partitions}
partition_names = list(partitions.keys())


def build_parser():
    """Generate the argument parser."""
    partition_opts = partition_names + ["all"]
    parser = argparse.ArgumentParser(
        description=__doc__,
    )
    parser.add_argument(
        "-p",
        "--partition",
        nargs="+",
        choices=partition_opts,
        default=["all"],
    )
    parser.add_argument(
        "-n",
        "--dry-run",
        help="Just print commands instead of running them.",
        action="store_true",
    )
    parser.add_argument(
        "PASSTHROUGH",
        nargs=argparse.REMAINDER,
        help="Pass through arguments to jobs.py",
    )

    return parser


if __name__ == "__main__":

    parser = build_parser()
    args = parser.parse_args()
    args = vars(args)

    selected_partitions = args["partition"]
    if "all" in selected_partitions and len(selected_partitions) > 1:
        raise ValueError("Cannot specify multiple partitions with 'all'")

    if "all" not in selected_partitions:
        for i in selected_partitions:
            if i not in partition_names:
                raise ValueError(f"Invalid partition selected: '{i}'")
    else:
        selected_partitions = partition_names

    for i in selected_partitions:
        obj = partitions[i]
        cmds = obj.cmds(args["PASSTHROUGH"])
        if args["dry_run"]:
            for c in cmds:
                print(" ".join(c))
        else:
            for c in cmds:
                subprocess.run(c, check=True)
