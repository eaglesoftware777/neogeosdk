#!/usr/bin/env python3
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from asset_rules import build_asset_specs


def main():
    specs = build_asset_specs("in")
    print(len(specs))


if __name__ == "__main__":
    main()
