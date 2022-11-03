#!/usr/bin/env python3
"""
A thin abstraction for easy comunication with the QST* subprocess.

Frequently, frontend specific information, such as supported methods, is
required during job generation or during analysis. This module aims to ease the
burden of manually updating all that specfic information, both in the frontend
and whatever other application by abstracting away those details to subprocess
calls. This all assumes that the subprocess supports introspection.
"""

import json
import subprocess
from functools import cached_property
from pathlib import Path


class QST:
    """
    A thin abstraction between the QST subprocess and Python.

    Grants cached and easy communication.
    """

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
        """Retrieve version information as JSON."""
        return json.loads(self._call("--version-json"))

    @cached_property
    def version(self) -> str:
        """Retrieve the major version."""
        return self.info["version"]

    @cached_property
    def threshold_methods(self) -> list[str]:
        """Retrieve a list of methods which support a threshold parameter."""
        return self._call("--show-methods=threshold").split()

    @cached_property
    def nonthreshold_methods(self) -> list[str]:
        """Retrieve a list of methods which do not support a threshold parameter."""
        return self._call("--show-methods=nonthreshold").split()

    @cached_property
    def methods(self) -> list[str]:
        """Retrieve a list of all supported methods."""
        return self.threshold_methods + self.nonthreshold_methods
