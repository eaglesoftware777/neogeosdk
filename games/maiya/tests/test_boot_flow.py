"""Test a debug-enabled staged Maiya build against installed system ROMs.

Run tools/build.py followed by tools/build.py --quick for the selected platform.
This test uses private configuration/backup RAM and never changes system ROMs.
"""

import argparse
import os
from pathlib import Path
import re
import subprocess

GAME = Path(__file__).resolve().parents[1]
ROOT = GAME.parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--platform', choices=('mvs', 'aes'), default='mvs')
    parser.add_argument('--bios', nargs='+', default=['euro', 'us', 'unibios40'])
    parser.add_argument('--eagle', action='store_true')
    parser.add_argument('--automatic', action='store_true', help='Wait for the credited title timer')
    parser.add_argument('--trace', action='store_true', help='Trace the Start transition (12-14 seconds)')
    parser.add_argument('--gdb', required=True)
    parser.add_argument('--mame', default='mame')
    parser.add_argument('--output', type=Path, default=GAME / 'build/boot-flow')
    args = parser.parse_args()
    work = GAME / 'build/workspace'
    if (work / 'platform.txt').read_text().strip() != args.platform:
        parser.error('Rebuild the workspace for the requested platform first')
    fields = {name: f'&mg.{name}' for name in ('player', 'state', 'demo', 'lives', 'continues')}
    fields['latch'] = '&maiya_console_start'
    command = [args.gdb, '-nx', '-batch', str(work / 'out/game')]
    for name, expression in fields.items():
        command += ['-ex', f'printf "{name}=%lu\\n", (unsigned long){expression}']
    symbols = subprocess.check_output(command, text=True)
    addresses = dict(re.findall(r'^(\w+)=(\d+)$', symbols, re.MULTILINE))
    if addresses.keys() != fields.keys():
        raise SystemExit('Missing debug symbols; run tools/build.py --quick')
    args.output.mkdir(parents=True, exist_ok=True)
    layout = args.output.resolve() / 'layout.lua'
    layout.write_text('return {' + ','.join(f'{k}={v}' for k, v in addresses.items()) + '}\n')
    failures = []
    for bios in args.bios:
        out = (args.output / (args.platform + '-' + bios + ('-eagle' if args.eagle else '')
                             + ('-auto' if args.automatic else ''))).resolve()
        out.mkdir(parents=True, exist_ok=True)
        paths = [work / 'roms', ROOT / 'roms']
        if args.eagle:
            paths.insert(0, ROOT / 'bios/test_roms')
        command = [args.mame, 'neogeo' if args.platform == 'mvs' else 'aes',
                   '-noreadconfig', '-rompath', ';'.join(map(str, paths)),
                   '-hashpath', str(work / 'hash_eagle/maiya'), '-cart1', 'maiya',
                   '-bios', bios, '-video', 'none', '-sound', 'none', '-nothrottle',
                   '-skip_gameinfo', '-seconds_to_run', '45', '-nonvram_save',
                   '-cfg_directory', str(out / 'cfg'), '-nvram_directory', str(out / 'nvram'),
                   '-autoboot_delay', '0', '-autoboot_script', str(GAME / 'tests/boot_flow.lua')]
        if args.trace:
            command += ['-debug', '-debugger', 'none']
        env = dict(os.environ, DISPLAY='', SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy',
                   MAIYA_LAYOUT=str(layout), MAIYA_RESULT=str(out / 'result.txt'),
                   MAIYA_SNAPSHOT=str(out / 'play.png'), MAIYA_FINAL=str(out / 'final.png'))
        env['MAIYA_TRACE'] = str(out / 'start.trc') if args.trace else ''
        env['MAIYA_AUTO'] = '1' if args.automatic else '0'
        with (out / 'mame.log').open('w') as log:
            result = subprocess.run(command, env=env, stdout=log, stderr=subprocess.STDOUT, timeout=180)
        report = (out / 'result.txt').read_text() if (out / 'result.txt').exists() else ''
        passed = result.returncode == 0 and 'PLAY lives=3 continues=3 ' in report
        if args.platform == 'mvs':
            passed = passed and 'PLAY lives=3 continues=3 credit=1 ' in report
        print(f'{args.platform} {bios}: {"PASS" if passed else "FAIL"} ({out})', flush=True)
        if not passed:
            failures.append(bios)
    if failures:
        raise SystemExit('Failed: ' + ', '.join(failures))


if __name__ == '__main__':
    main()
