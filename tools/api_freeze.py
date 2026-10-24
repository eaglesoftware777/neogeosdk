#!/usr/bin/env python3
"""
Framework v1: the C engine's public calls are frozen. From v1.0 on they
change by additions only.

    python3 tools/api_freeze.py          check (make api-check, make test)
    python3 tools/api_freeze.py --write  record the list (only when a new
                                         framework version is declared)

The list is docs/api/framework_v1.txt: every prototype in sdk/2d_engine/*.h
that carries NEOGEO_USER, read the way tools/gen_api_reference.py reads
them, one per line as "header: prototype". The check fails when a listed
prototype is missing or its signature changed; new ones are reported and
allowed.
"""

import argparse
import glob
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'tools'))
from gen_api_reference import protos_c  # noqa: E402

FROZEN = os.path.join(ROOT, 'docs', 'api', 'framework_v1.txt')


def current():
    out = set()
    for path in sorted(glob.glob(os.path.join(ROOT, 'sdk', '2d_engine', '*.h'))):
        name = os.path.basename(path)
        for proto in protos_c(path):
            out.add(f'{name}: {proto}')
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--write', action='store_true', help='record the current list as frozen')
    args = ap.parse_args()
    now = current()
    if args.write:
        os.makedirs(os.path.dirname(FROZEN), exist_ok=True)
        with open(FROZEN, 'w', encoding='utf-8', newline='\n') as fh:
            fh.write('# Framework v1: the C engine API (sdk/2d_engine), frozen.\n')
            fh.write('# Written by tools/api_freeze.py --write; checked by make api-check.\n')
            for line in sorted(now):
                fh.write(line + '\n')
        print(f'api-freeze: recorded {len(now)} prototypes in {os.path.relpath(FROZEN, ROOT)}')
        return 0
    if not os.path.isfile(FROZEN):
        print(f'api-freeze: {os.path.relpath(FROZEN, ROOT)} is missing', file=sys.stderr)
        return 1
    with open(FROZEN, encoding='utf-8') as fh:
        frozen = {l.rstrip('\n') for l in fh if l.strip() and not l.startswith('#')}
    gone = sorted(frozen - now)
    added = sorted(now - frozen)
    for line in gone:
        print(f'api-freeze: missing or changed: {line}', file=sys.stderr)
    if gone:
        print(f'api-freeze: {len(gone)} frozen prototype(s) missing or changed; '
              'Framework v1 changes by additions only', file=sys.stderr)
        return 1
    print(f'api-freeze: {len(frozen)} frozen prototypes present'
          + (f', {len(added)} added since' if added else ''))
    return 0


if __name__ == '__main__':
    sys.exit(main())
