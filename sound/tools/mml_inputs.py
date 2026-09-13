"""Stable track numbering for shell-expanded and literal Windows wildcards."""

import glob
from pathlib import Path
import re


def ordered_inputs(inputs):
    paths = set()
    for value in inputs:
        pattern = str(value)
        matches = glob.glob(pattern) if glob.has_magic(pattern) else [pattern]
        if not matches:
            raise ValueError(f"No music files match {pattern!r}")
        paths.update(Path(match) for match in matches)

    def key(path):
        match = re.match(r"^(\d+)(?:_|$)", path.stem)
        return (int(match[1]) if match else 65536, path.name.casefold(), str(path))

    return sorted(paths, key=key)
