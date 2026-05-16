#!/usr/bin/env python3
import argparse
import os
import sys


def parse_cfg(path: str):
    data = {}
    with open(path, 'r', encoding='utf-8') as f:
        for raw in f:
            line = raw.strip()
            if not line or line.startswith('#'):
                continue
            if '=' not in line:
                continue
            k, v = line.split('=', 1)
            data[k.strip()] = v.strip()
    return data


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--cfg', default='game.cfg')
    args = ap.parse_args()

    if not os.path.exists(args.cfg):
        print(f'ERROR: missing {args.cfg}. Create it with CURRENT_GAME=<game>.', file=sys.stderr)
        return 1

    cfg = parse_cfg(args.cfg)
    current = cfg.get('CURRENT_GAME') or cfg.get('GAME')
    if not current:
        print(f'ERROR: {args.cfg} must define CURRENT_GAME=<game>.', file=sys.stderr)
        return 1

    game = os.environ.get('GAME', '').strip()
    if not game:
        print('ERROR: GAME is empty.', file=sys.stderr)
        return 1

    if game != current:
        print(
            f'ERROR: GAME mismatch. make is using GAME={game}, but {args.cfg} says CURRENT_GAME={current}.\n'
            f'Fix by either:\n'
            f'  1) update {args.cfg} to CURRENT_GAME={game}\n'
            f'  2) run with GAME={current}',
            file=sys.stderr,
        )
        return 1

    print(f'[game-check] GAME={game} (from {args.cfg})')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
