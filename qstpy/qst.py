#!/usr/bin/env python3

import json
import subprocess
from functools import cached_property
from pathlib import Path


class QST:
    """TODO."""

    def __init__(self, qst_path: str = "build/QST"):
        """TODO."""
        qst_path = Path(qst_path)
        if not qst_path.is_file():
            raise FileNotFoundError(qst_path)

        self._exec_path = qst_path

    def _call(self, *args):
        return (
            subprocess.run([self._exec_path, *args], check=True, capture_output=True)
            .stdout.decode()
            .strip()
        )

    @cached_property
    def info(self) -> dict:
        """TODO."""
        return json.loads(self._call("--version-json"))

    @cached_property
    def version(self) -> str:
        """TODO."""
        return self.info["version"]

    @cached_property
    def threshold_methods(self) -> list[str]:
        """TODO."""
        return self._call("--show-methods=threshold").split()

    @cached_property
    def nonthreshold_methods(self) -> list[str]:
        """TODO."""
        return self._call("--show-methods=nonthreshold").split()
