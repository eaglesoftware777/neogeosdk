#!/usr/bin/env python3
"""Reject overlapping WLA writes even on versions that return success."""

import subprocess
import sys


def main():
    result = subprocess.run(sys.argv[1:], stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, text=True,
                            errors="replace", check=False)
    sys.stdout.write(result.stdout)
    if "MEM_INSERT:" in result.stdout and "write into" in result.stdout:
        sys.exit("Overlapping ROM data: refusing to publish M1.")
    sys.exit(result.returncode)


if __name__ == "__main__":
    main()
