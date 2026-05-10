#!/usr/bin/env python3

import os


def main():
    files = sorted(f for f in os.listdir("in") if f.lower().endswith(".png"))
    print(len(files))


if __name__ == "__main__":
    main()
