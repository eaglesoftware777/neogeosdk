"""Coins, Start and credits under each system ROM, in MAME (tests/boot_flow.lua).

    python3 games/centuryxi/tests/test_boot_flow.py --set dist/neo/centuryxi \\
        --elf out/game --platform mvs

--set is a folder with <platform>/roms/centuryxi and <platform>/hash (what
`make neo` writes to dist/neo/centuryxi, or an installed MAME test folder);
--elf the program the P1 was built from, for the match state's address. The
system ROMs come from roms/ (kept out of git) and EagleBIOS from
bios/test_roms; MAME gets private configuration and backup RAM, so nothing
installed is changed.

Checked on every system ROM given:
  boot     Start begins the game for one credit; three more Starts in the
           menu are free (the US arcade ROM write-protects its backup RAM:
           the Start flag lives in work RAM)
  session  after a Penalty Challenge, Start pays for and begins a second game
  versus   player 2 pays exactly one credit (from his own count where the
           system keeps one per chute), and the board does not restart
  join     a paying player 2 joins a match under way, for one credit
"""

import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

GAME = Path(__file__).resolve().parents[1]
ROOT = GAME.parents[1]
SCENARIOS = ('boot', 'session', 'versus', 'join')
SECONDS = {'boot': 26, 'session': 120, 'versus': 28, 'join': 46}
DEFAULT_BIOS = {'mvs': ['us', 'euro', 'japan', 'asia', 'unibios40', 'eagle'],
                'aes': ['asia', 'japan', 'eagle']}


def bcd(v):
    return (v >> 4) * 10 + (v & 15)


def parse(text):
    marks = {}
    for name, fields in re.findall(r'^MARK (\w+) (.*)$', text, re.MULTILINE):
        values = dict(re.findall(r'(\w+)=([0-9A-F/]+)', fields))
        c1, c2 = values.pop('credit').split('/')
        values = {k: int(v) for k, v in values.items()}
        values['credits'] = bcd(int(c1, 16)) + bcd(int(c2, 16))
        marks[name] = values
    return marks, 'RESET' in text


def verdict(scenario, mvs, marks, reset):
    if reset:
        return 'the board restarted'
    end = marks.get('end')
    if not end or end['mode'] != 2:
        return 'not in a game at the end'
    if scenario == 'boot':
        if end['p1'] != 1:
            return 'player 1 not playing'
        if mvs and not (marks['before']['credits'] - 1 == marks['paid']['credits'] == end['credits']):
            return 'credits %d -> %d -> %d, expected one taken' % (
                marks['before']['credits'], marks['paid']['credits'], end['credits'])
    elif scenario == 'session':
        if 'session_end' not in marks:
            return 'the session never ended'
        if mvs and end['credits'] != marks['before']['credits'] - 2:
            return 'credits %d -> %d over two games' % (marks['before']['credits'], end['credits'])
    elif scenario == 'versus':
        if mvs and (end['p2'] != 1 or end['credits'] != marks['before']['credits'] - 1):
            return 'player 2 mode %d, credits %d -> %d' % (end['p2'], marks['before']['credits'], end['credits'])
    elif scenario == 'join':
        if end['on'] != 3 or end['p2'] != 1:
            return 'player 2 did not join (match %d, mode %d)' % (end['on'], end['p2'])
        if mvs and end['credits'] != marks['before']['credits'] - 1:
            return 'credits %d -> %d' % (marks['before']['credits'], end['credits'])
    return None


def nm_tool(name):
    for tool in (name, os.path.expanduser('~/x-tools-v3/m68k-unknown-elf/bin/m68k-unknown-elf-nm')):
        if shutil.which(tool) or os.path.exists(tool):
            return tool
    raise SystemExit('m68k nm not found: pass --nm')


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--set', type=Path, required=True)
    parser.add_argument('--elf', type=Path, required=True)
    parser.add_argument('--platform', choices=('mvs', 'aes'), default='mvs')
    parser.add_argument('--bios', nargs='+')
    parser.add_argument('--scenario', nargs='+', choices=SCENARIOS, default=list(SCENARIOS))
    parser.add_argument('--mame', default='mame')
    parser.add_argument('--nm', default='m68k-unknown-elf-nm')
    parser.add_argument('--output', type=Path, help='keep results here (default: a temporary folder)')
    args = parser.parse_args()
    symbols = subprocess.check_output([nm_tool(args.nm), str(args.elf)], text=True)
    found = re.search(r'^([0-9a-f]+) [bBdD] cx_match_on$', symbols, re.MULTILINE)
    if not found:
        raise SystemExit('cx_match_on not in ' + str(args.elf))
    # MAME as a snap cannot read /tmp: a temporary folder goes in /dev/shm
    work = args.output or Path(tempfile.mkdtemp(prefix='cxboot.', dir='/dev/shm' if os.path.isdir('/dev/shm') else None))
    work.mkdir(parents=True, exist_ok=True)
    rom_set = args.set.resolve()
    failures = []
    for bios in args.bios or DEFAULT_BIOS[args.platform]:
        for scenario in args.scenario:
            out = (work / f'{args.platform}-{bios}-{scenario}').resolve()
            out.mkdir(parents=True, exist_ok=True)
            system = ROOT / 'bios/test_roms' if bios == 'eagle' else ROOT / 'roms'
            command = [args.mame, 'neogeo' if args.platform == 'mvs' else 'aes',
                       '-noreadconfig', '-skip_gameinfo',
                       '-rompath', ';'.join(map(str, (system, rom_set / args.platform / 'roms'))),
                       '-hashpath', str(rom_set / args.platform / 'hash'), '-cart1', 'centuryxi',
                       '-cfg_directory', str(out / 'cfg'), '-nvram_directory', str(out / 'nvram'),
                       '-noautosave', '-nonvram_save', '-video', 'none', '-sound', 'none', '-nothrottle',
                       '-seconds_to_run', str(SECONDS[scenario]), '-autoboot_delay', '0',
                       '-autoboot_script', str(GAME / 'tests/boot_flow.lua')]
            if bios != 'eagle':
                command += ['-bios', bios]
            env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy',
                       CX_SCENARIO=scenario, CX_PLATFORM=args.platform, CX_MATCH_ON=found.group(1),
                       CX_RESULT=str(out / 'result.txt'), CX_SNAPSHOT=str(out / 'end.png'))
            env.pop('DISPLAY', None)
            with (out / 'mame.log').open('w') as log:
                run = subprocess.run(command, env=env, stdout=log, stderr=subprocess.STDOUT, timeout=900)
            text = (out / 'result.txt').read_text() if (out / 'result.txt').exists() else ''
            marks, reset = parse(text)
            problem = 'MAME exit %d' % run.returncode if run.returncode else verdict(
                scenario, args.platform == 'mvs', marks, reset)
            print(f'{args.platform} {bios:10s} {scenario:8s} {"PASS" if not problem else "FAIL: " + problem}', flush=True)
            if problem:
                failures.append(f'{bios} {scenario}')
    if failures:
        raise SystemExit(f'Failed ({work}): ' + ', '.join(failures))


if __name__ == '__main__':
    main()
