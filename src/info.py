#!/usr/bin/env python3
"""
Write info about the system to a FOLDER/system_details.pickle

Usage:
    info.py DIR [options]

Options:
    -h, --help               Show this help.
"""

import platform
from pathlib import Path
import pickle
from docopt import docopt


def write_info(output_folder, concurrent="slurm"):
    info = {
        "Architecture": platform.architecture(),
        "Machine": platform.machine(),
        "Platform": platform.platform(),
        "Processor": platform.processor(),
        "Release": platform.release(),
        "System": platform.system(),
        "Uname": platform.uname(),
        "Version": platform.version(),
        "Number of concurrent jobs": concurrent,
    }

    with open(Path(output_folder, "system_details.pickle"), "wb") as f:
        pickle.dump(info, f, protocol=pickle.HIGHEST_PROTOCOL)


if __name__ == "__main__":
    args = docopt(__doc__)
    OUT_DIR = Path(args.get("DIR"))
    write_info(OUT_DIR)
