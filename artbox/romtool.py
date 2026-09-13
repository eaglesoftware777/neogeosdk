#!/usr/bin/env python3
"""
romtool.py - ROMWak-compatible utility (portable Python 3 port)

Supported switches:
  /b split alternating bytes
  /c concatenate files
  /f flip low/high bytes
  /h split equal halves
  /i write size/crc info text
  /m merge bytes from 2 files
  /q merge bytes from 4 files
  /s swap top/bottom halves
  /u update first N bytes from file1 into file2
  /w split alternating words
  /p pad file to KBytes with byte value
"""

from __future__ import annotations

import binascii
import os
import sys

VERSION = "0.4-py"


def usage() -> None:
    print("usage: romtool.py [--dir <full_path>] <option> <infile> <outfile> [outfile2] [psize] [pbyte]")


def read_file(path: str) -> bytes:
    with open(path, "rb") as f:
        return f.read()


def write_file(path: str, data: bytes) -> None:
    with open(path, "wb") as f:
        f.write(data)


def resolve_path(base_dir: str | None, path: str | None) -> str | None:
    if path is None:
        return None
    if os.path.isabs(path) or not base_dir:
        return path
    return os.path.join(base_dir, path)


def check_exists(path: str) -> bool:
    if os.path.exists(path):
        return True
    print(f"Error attempting to open file: {os.strerror(2)}", file=sys.stderr)
    return False


def split_equal(infile: str, out1: str, out2: str) -> int:
    if not check_exists(infile):
        return 1
    data = read_file(infile)
    half = len(data) // 2
    write_file(out1, data[:half])
    write_file(out2, data[half:half + half])
    print(f"'{out1}' saved successfully!")
    print(f"'{out2}' saved successfully!")
    return 0


def split_bytes(infile: str, out1: str, out2: str) -> int:
    if not check_exists(infile):
        return 1
    data = read_file(infile)
    write_file(out1, data[0::2])
    write_file(out2, data[1::2])
    print(f"'{out1}' saved successfully!")
    print(f"'{out2}' saved successfully!")
    return 0


def split_words(infile: str, out1: str, out2: str) -> int:
    if not check_exists(infile):
        return 1
    data = read_file(infile)
    if len(data) % 4:
        data = data[: len(data) - (len(data) % 4)]
    b1 = bytearray()
    b2 = bytearray()
    for i in range(0, len(data), 4):
        b1.extend(data[i:i + 2])
        b2.extend(data[i + 2:i + 4])
    write_file(out1, bytes(b1))
    write_file(out2, bytes(b2))
    print(f"'{out1}' saved successfully!")
    print(f"'{out2}' saved successfully!")
    return 0


def flip_bytes(infile: str, outfile: str | None) -> int:
    if not check_exists(infile):
        return 1
    out = outfile or infile
    data = bytearray(read_file(infile))
    if len(data) % 2:
        data.append(0)
    for i in range(0, len(data), 2):
        data[i], data[i + 1] = data[i + 1], data[i]
    write_file(out, bytes(data))
    print(f"'{out}' saved successfully!")
    return 0


def merge_bytes(in1: str, in2: str, out: str) -> int:
    if not check_exists(in1) or not check_exists(in2):
        return 1
    a = read_file(in1)
    b = read_file(in2)
    n = min(len(a), len(b))
    outbuf = bytearray()
    for i in range(n):
        outbuf.append(a[i])
        outbuf.append(b[i])
    write_file(out, bytes(outbuf))
    print(f"'{out}' saved successfully!")
    return 0


def merge_bytes_quad(in1: str, in2: str, in3: str, in4: str, out: str) -> int:
    if not all(check_exists(p) for p in (in1, in2, in3, in4)):
        return 1
    a = read_file(in1)
    b = read_file(in2)
    c = read_file(in3)
    d = read_file(in4)
    n = min(len(a), len(b), len(c), len(d))
    outbuf = bytearray()
    for i in range(n):
        outbuf.extend((a[i], b[i], c[i], d[i]))
    write_file(out, bytes(outbuf))
    print(f"'{out}' saved successfully!")
    return 0


def update_bytes(in1: str, in2: str, out: str, size_s: str) -> int:
    if not check_exists(in1) or not check_exists(in2):
        return 1
    size = int(size_s, 10)
    a = read_file(in1)
    b = bytearray(read_file(in2))
    if size > len(a):
        raise RuntimeError("update size larger than file 1")
    b[:size] = a[:size]
    write_file(out, bytes(b))
    print(f"'{out}' saved successfully!")
    return 0


def swap_halves(infile: str, outfile: str | None) -> int:
    if not check_exists(infile):
        return 1
    out = outfile or infile
    data = read_file(infile)
    half = len(data) // 2
    write_file(out, data[half:half + half] + data[:half])
    print(f"'{out}' saved successfully!")
    return 0


def pad_file(infile: str, outfile: str, psize_k: str, pbyte: str) -> int:
    if not check_exists(infile):
        return 1
    size = int(psize_k, 10) * 1024
    pb = int(pbyte, 10) & 0xFF
    data = read_file(infile)
    if len(data) > size:
        raise RuntimeError(f"input file larger than pad size ({len(data)} > {size})")
    if len(data) < size:
        data += bytes([pb]) * (size - len(data))
    write_file(outfile, data)
    print(f"'{outfile}' saved successfully!")
    return 0


def concat_files(a: str, b: str, out: str) -> int:
    if not check_exists(a) or not check_exists(b):
        return 1
    write_file(out, read_file(a) + read_file(b))
    print(f"'{a}' + '{b}' concatained into '{out}' successfully!")
    return 0


def info_file(infile: str, outfile: str) -> int:
    if not check_exists(infile):
        return 1
    data = read_file(infile)
    crc = binascii.crc32(data) & 0xFFFFFFFF
    text = f"{infile} size:{len(data)} crc32:{crc}"
    with open(outfile, "wt", encoding="utf-8") as f:
        f.write(text)
    print(text)
    print(f"'{outfile}' saved successfully!")
    return 0


def main(argv: list[str]) -> int:
    print(f"ROMWak {VERSION}")
    if len(argv) < 2:
        usage()
        return 1
    base_dir: str | None = None
    i = 1
    if len(argv) >= 4 and argv[1] == "--dir":
        base_dir = argv[2]
        i = 3
    if len(argv) <= i:
        usage()
        return 1
    opt = argv[i]
    if len(opt) < 2 or opt[0] not in ("/", "-"):
        usage()
        return 1
    k = opt[1].lower()
    raw = argv[i + 1 :]
    try:
        if k == "b":
            return split_bytes(resolve_path(base_dir, raw[0]), resolve_path(base_dir, raw[1]), resolve_path(base_dir, raw[2]))
        if k == "c":
            return concat_files(resolve_path(base_dir, raw[0]), resolve_path(base_dir, raw[1]), resolve_path(base_dir, raw[2]))
        if k == "f":
            return flip_bytes(resolve_path(base_dir, raw[0]), resolve_path(base_dir, raw[1]) if len(raw) > 1 else None)
        if k == "h":
            return split_equal(resolve_path(base_dir, raw[0]), resolve_path(base_dir, raw[1]), resolve_path(base_dir, raw[2]))
        if k == "i":
            return info_file(resolve_path(base_dir, raw[0]), resolve_path(base_dir, raw[1]))
        if k == "m":
            return merge_bytes(resolve_path(base_dir, raw[0]), resolve_path(base_dir, raw[1]), resolve_path(base_dir, raw[2]))
        if k == "q":
            return merge_bytes_quad(
                resolve_path(base_dir, raw[0]),
                resolve_path(base_dir, raw[1]),
                resolve_path(base_dir, raw[2]),
                resolve_path(base_dir, raw[3]),
                resolve_path(base_dir, raw[4]),
            )
        if k == "s":
            return swap_halves(resolve_path(base_dir, raw[0]), resolve_path(base_dir, raw[1]) if len(raw) > 1 else None)
        if k == "u":
            return update_bytes(
                resolve_path(base_dir, raw[0]),
                resolve_path(base_dir, raw[1]),
                resolve_path(base_dir, raw[2]),
                raw[3],
            )
        if k == "w":
            return split_words(resolve_path(base_dir, raw[0]), resolve_path(base_dir, raw[1]), resolve_path(base_dir, raw[2]))
        if k == "p":
            return pad_file(resolve_path(base_dir, raw[0]), resolve_path(base_dir, raw[1]), raw[2], raw[3])
    except (IndexError, ValueError, OSError, RuntimeError) as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1
    print(f"ERROR: Option '{opt}' doesn't exist.")
    return 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
