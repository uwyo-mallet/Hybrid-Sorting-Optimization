#!/usr/bin/env python3
"""
Write info about the system to a FOLDER/system_details.txt

Usage:
    info.py DIR [options]

Options:
    -h, --help               Show this help.
"""

import platform
from pathlib import Path

from docopt import docopt


def write_info(output_folder):
    info = {
        "Architecture": platform.architecture(),
        "Machine": platform.machine(),
        "Platform": platform.platform(),
        "Processor": platform.processor(),
        "Release": platform.release(),
        "System": platform.system(),
        "Uname": platform.uname(),
        "Version": platform.version(),
    }

    with open(Path(output_folder, "system_details.txt"), "w") as f:
        for i in info.keys():
            f.write(f"{i}: {str(info[i])}\n")


if __name__ == "__main__":
    args = docopt(__doc__)
    OUT_DIR = Path(args.get("DIR"))
    write_info(OUT_DIR)
