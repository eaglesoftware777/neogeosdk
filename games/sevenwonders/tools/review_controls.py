"""Check a matching private ROM through ordinary controls and read-only taps.

Requires GAME_OPTIMIZE='-O2 -g3 -gdwarf-2'. Keeps native snapshots and raw
measurements; never installs a build or writes emulated game memory.
"""
import argparse
import csv
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--workspace', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--platform', choices=('mvs', 'aes'), required=True)
    parser.add_argument('--route', choices=('directions', 'bubble'), default='directions')
    parser.add_argument('--level-fixture', action='store_true', help='Record that this private test build starts at level 2')
    parser.add_argument('--mame', default='/usr/games/mame')
    parser.add_argument('--m68k-prefix', default='/mnt/c/neogeo/x-tools-v3/m68k-unknown-elf/bin/m68k-unknown-elf-')
    args = parser.parse_args()
    root, out = args.workspace.resolve(), args.out.resolve()
    out.mkdir(parents=True, exist_ok=False)
    elf = root / 'out/game'
    expressions = {name: f'(unsigned long)&{name}' for name in
                   ('s_frames', 's_bound', 's_joy', 's_repeat')}
    expressions['puzzle'] = "(unsigned long)&'sw_game.c'::s_p"
    for field in ('mode', 'w', 'h', 'colors', 'color', 'kind', 'cover', 'tick', 'state', 'moves_used', 'angle',
                  'shot_x_q8', 'shot_y_q8', 'shot_vx_q8', 'shot_vy_q8'):
        expressions[field] = f'(unsigned long)&((NGPuzzle*)0)->{field}'
    cmd = [args.m68k_prefix + 'gdb', '-nx', '-batch', str(elf)]
    for key, expr in expressions.items():
        cmd += ['-ex', f'printf "{key}=%lu\\n", {expr}']
    layout = {k: int(v) for k, v in re.findall(r'^(\w+)=(\d+)$', subprocess.check_output(cmd, text=True), re.M)}
    if layout.keys() != expressions.keys():
        raise ValueError('Incomplete matching ELF layout')
    binary = out / 'program.bin'
    subprocess.run([args.m68k_prefix + 'objcopy', '-O', 'binary', str(elf), str(binary)], check=True)
    program = (root / 'roms/sevenwonders/782-p1.p1').read_bytes()
    native = bytearray(program)
    native[0::2], native[1::2] = program[1::2], program[0::2]
    if binary.read_bytes() != native[:binary.stat().st_size]:
        raise ValueError('ELF does not match ROM')
    binary.unlink()
    config = dict(layout=layout, output=str(out), route=args.route)
    assets = json.loads((root / 'games/sevenwonders/artbox/generated/assets.json').read_text())
    config['panel'] = next(a for a in assets['assets'] if a['name'] == 'panel')
    if args.route == 'bubble' and not args.level_fixture:
        helper = out / 'review-hint'
        subprocess.run(['cc', '-O2', '-std=c99', '-I', str(root / 'sdk'),
                        '-I', str(root / 'sdk/2d_engine'), str(Path(__file__).with_name('review_hint.c')),
                        *[str(root / 'sdk/2d_engine' / f'{name}.c')
                          for name in ('ng_puzzle', 'ng_trig', 'ng_trig_table')],
                        '-o', str(helper)], check=True)
        config['hint_helper'] = str(helper)

    def lua(value):
        if isinstance(value, dict):
            return '{' + ','.join('[' + lua(k) + ']=' + lua(v) for k, v in value.items()) + '}'
        return json.dumps(value)

    script = out / 'capture.lua'
    script.write_text('local q=' + lua(config) + '\n' + Path(__file__).with_name('controls_capture.lua').read_text())
    (out / 'manifest.json').write_text(json.dumps(dict(workspace=str(root), platform=args.platform,
        normal_inputs_only=True, level_start_fixture=args.level_fixture, route=args.route,
        layout=layout, elf_sha256=hashlib.sha256(elf.read_bytes()).hexdigest(),
        rom_sha256={p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                    for p in (root / 'roms/sevenwonders').glob('782-*')}), indent=2) + '\n')
    env = os.environ.copy()
    env.pop('DISPLAY', None); env.pop('WAYLAND_DISPLAY', None)
    env.update(SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy', QT_QPA_PLATFORM='offscreen')
    cmd = [args.mame, 'aes' if args.platform == 'aes' else 'neogeo', '-cart1', 'sevenwonders',
           '-bios', 'asia' if args.platform == 'aes' else 'euro', '-noreadconfig', '-skip_gameinfo',
           '-video', 'none', '-sound', 'none', '-nothrottle', '-nonvram_save', '-seconds_to_run', '90',
           '-rompath', str(root / 'bios/test_roms') + ';' + str(root / 'roms'),
           '-hashpath', str(root / 'hash_eagle/sevenwonders'), '-cfg_directory', str(out / 'cfg'),
           '-nvram_directory', str(out / 'nvram'), '-autoboot_delay', '0', '-autoboot_script', str(script)]
    with (out / 'mame.log').open('w') as log:
        subprocess.run(cmd, cwd=root, env=env, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=240)
    rows = list(csv.DictReader((out / 'controls.csv').open()))
    if len(rows) < (100 if args.route == 'bubble' else 250) or rows[-1]['phase'] != 'complete':
        raise ValueError('Ordinary-input control sequence did not complete')
    state = [[0, 0], [0, 0]]
    mismatches = []
    for row in rows:
        held, result = int(row['joy']), 0
        for axis, mask in enumerate((12, 3)):
            bits = held & mask
            if bits == mask:
                bits = 0
            old, wait = state[axis]
            if bits != old:
                emitted, wait = bits, 15
            elif bits and wait <= 1:
                emitted, wait = bits, 4
            else:
                emitted = 0
                if bits:
                    wait -= 1
            state[axis] = [bits, wait]
            result |= emitted
        # This script uses directions only; button edges stay separate.
        if result != (int(row['repeat']) & 15):
            mismatches.append(int(row['frame']))
    stores = json.loads((out / 'stores.json').read_text())
    ready = [i for i, r in enumerate(rows) if int(r['panel']) and int(r['score'])]
    if not ready:
        raise ValueError('No complete score panel was observed')
    first_ui = ready[0]
    ui_delay = float(rows[first_ui]['seconds']) - float(rows[0]['seconds'])
    missing_ui = [int(r['frame']) for r in rows[first_ui:]
                  if not int(r['panel']) or not int(r['score'])]
    if args.route == 'bubble' and not args.level_fixture and not stores['approach_won']:
        raise ValueError('Bubble replay did not clear the preceding board through normal inputs')
    steering = 0
    if args.route == 'bubble':
        if any(int(r['mode']) != 1 for r in rows):
            raise ValueError('Bubble input test did not reach an actual bubble board')
        for a, b in zip(rows, rows[1:]):
            if int(a['state']) == int(b['state']) == 4 and int(a['angle']) != int(b['angle']):
                if a['vx'] != b['vx'] or a['vy'] != b['vy']:
                    raise ValueError('Preparing the next aim changed the flying shot velocity')
                steering += 1
        if steering < 3:
            raise ValueError('No meaningful steering during actual flight was observed')
    result = dict(passed=not mismatches and bool(stores['vram']) and not stores['active_vram']
                  and not stores['active_palette'] and ui_delay <= 0.5 and not missing_ui,
                  samples=len(rows), repeat_mismatches=mismatches,
                  first_complete_ui_seconds=ui_delay, missing_ui_after_ready=missing_ui,
                  normal_inputs_only=True, platform=args.platform, **stores)
    if args.route == 'bubble':
        result.update(busy_aim_samples=steering, level_start_fixture=args.level_fixture)
    (out / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))
    if not result['passed']:
        raise ValueError('Control/video timing gate failed')


if __name__ == '__main__':
    main()
