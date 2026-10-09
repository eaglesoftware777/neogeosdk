"""Verify installed Maiya graphics, capture gameplay and compare an old set."""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import subprocess
import wave
import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]


def windows(path):
    return subprocess.check_output(['wslpath', '-w', str(path)], text=True).strip().replace('\\', '/')


def layout(elf):
    expressions = {name: '&mg.' + name for name in
                   ('player', 'demo', 'state', 'stage', 'next_stage', 'state_timer')}
    expressions['camera'] = '&mg.camera.x'
    expressions.update({name: '&((NGCharacter*)0)->' + name[5:] for name in ('char_x', 'char_y')})
    gdb = ROOT.parent / 'x-tools-v3/m68k-unknown-elf/bin/m68k-unknown-elf-gdb'
    command = [str(gdb), '-batch', str(elf)]
    for name, expression in expressions.items():
        command += ['-ex', f'printf "{name}=%lu\\n", (unsigned long){expression}']
    output = subprocess.check_output(command, text=True)
    result = dict(line.split('=', 1) for line in output.splitlines() if '=' in line)
    if set(result) != set(expressions):
        raise ValueError('The ELF does not describe the installed game layout')
    return {name: int(value) for name, value in result.items()}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--target', type=Path, default=Path('/mnt/c/mame/neogeosdk'))
    parser.add_argument('--platform', choices=('aes', 'mvs'), default='mvs')
    parser.add_argument('--baseline', type=Path, help='Backup folder, for diagnostic captures without layout assertions')
    parser.add_argument('--stage', type=int, choices=range(12))
    parser.add_argument('--seconds', type=int, default=45)
    args = parser.parse_args()
    data = (args.baseline or args.target) / 'tests' / args.platform
    label = args.platform + ('-before' if args.baseline else '-rebuilt')
    if args.stage is not None:
        label += f'-stage-{args.stage+1}'
    output = args.target / 'tests/maiya-check' / label
    output.mkdir(parents=True, exist_ok=True)
    fields = {} if args.baseline else layout(data / 'game.elf')
    config = '{output=' + json.dumps(windows(output)) + ',fields={'
    config += ','.join(f'{name}={value}' for name, value in fields.items()) + '}'
    if args.baseline:
        config += ',baseline=true'
    if args.stage is not None:
        config += f',stage={args.stage}'
    config += '}'
    script = output / 'capture.lua'
    script.write_text(Path(__file__).with_suffix('.lua').read_text().replace('__MAIYA_CONFIG__', config), encoding='ascii')
    command = [str(args.target.parent / 'mame.exe'), 'aes' if args.platform == 'aes' else 'neogeo',
               '-noreadconfig', '-rompath', ';'.join(windows(p) for p in
                   (data / 'roms', args.target / 'roms', args.target.parent / 'roms')),
               '-hashpath', windows(data / 'hash'), '-bios', 'asia' if args.platform == 'aes' else 'euro',
               '-cart1', 'maiya', '-video', 'none', '-sound', 'none', '-nothrottle', '-nonvram_save',
               '-seconds_to_run', str(args.seconds), '-skip_gameinfo',
               '-cfg_directory', windows(output / 'cfg'), '-nvram_directory', windows(output / 'nvram'),
               '-autoboot_delay', '0', '-autoboot_script', windows(script), '-wavwrite', windows(output / 'audio.wav')]
    with (output / 'mame.log').open('w') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    with (output / 'state.csv').open() as stream:
        rows = list(csv.DictReader(stream))
    gameplay = [r for r in rows if int(r['mode']) == 2 and int(r['player'], 16)
                and r['demo'] == '0' and r['state'] == '1']
    if not args.baseline:
        if len(gameplay) < 3:
            raise ValueError('Maiya did not enter gameplay')
        if not all(r['protocol'] == '4e475331' and r['slot_wait'] == '0' and int(r['timer']) & 2 for r in gameplay):
            raise ValueError('The rebuilt sound driver is not active')
        if args.stage is not None and not any(int(r['stage']) == args.stage for r in gameplay):
            raise ValueError('Requested stage did not render')
    rendered = []
    for row in gameplay:
        path = output / f't{float(row["seconds"]):05.1f}.png'
        if not path.is_file():
            continue
        pixels = np.asarray(Image.open(path).convert('RGB'))
        if (pixels.max(axis=2) > 12).mean() > 0.40 and len(np.unique(pixels.reshape(-1, 3), axis=0)) > 50:
            rendered.append(path.name)
    if not args.baseline and len(rendered) < 3:
        raise ValueError('Gameplay images are blank or incomplete')
    with wave.open(str(output / 'audio.wav')) as wav:
        samples = np.frombuffer(wav.readframes(wav.getnframes()), dtype='<i2').astype(np.int32)
        peak = int(np.abs(samples).max())
    if not args.baseline and not peak:
        raise ValueError('Audio is silent')
    result = {'platform': args.platform, 'baseline': bool(args.baseline), 'gameplay_samples': len(gameplay),
              'rendered_frames': rendered, 'audio_peak': peak,
              'roms': {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in (data / 'roms/maiya').glob('780-*')}}
    (output / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
