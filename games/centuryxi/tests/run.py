#!/usr/bin/env python3
"""Run deterministic match and tournament checks on the host."""
from pathlib import Path
import subprocess
import tempfile
root = Path(__file__).resolve().parents[1]
sdk = root.parents[1] / 'sdk' / '2d_engine'
# the game's ball, offside and championship are the SDK's football engine
SIM = ['cx_match.c', 'cx_ball.c', 'cx_player.c', 'cx_keeper.c', 'cx_ai.c', 'cx_rules.c', 'cx_nations.c']
with tempfile.TemporaryDirectory(prefix='centuryxi-tests-') as folder:
    for name, sources in [('sim', SIM), ('tournament', SIM + ['cx_tournament.c'])]:
        exe = str(Path(folder) / name)
        subprocess.run(['cc', '-std=c99', '-O2', '-g', '-DCX_CODE=', '-DNG_TUNE_CODE=', '-fsanitize=address,undefined',
                        '-I', str(root/'scenes'), '-I', str(root/'tests'), '-I', str(sdk), '-I', str(sdk.parent), str(sdk/'ng_tune.c'), str(sdk/'ng_soccer.c'),
                        *[str(root/'scenes'/s) for s in sources],
                        str(root/'tests'/f'{name}_test.c'), '-o', exe], check=True)
        subprocess.run([exe], check=True)
