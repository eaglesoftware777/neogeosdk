"""Exercise the native Maiya CD disc in an explicitly labeled HLE emulator."""
import argparse
import array
import csv
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import wave

if __package__ in (None, ''):
    sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.neocd.libretro_host import NeoCD
from tools.neocd.audit_disc import audit_cue, read_iso
from tools.neocd.build import default_compiler


def run(core, build, out, nm, frames=2400):
    build, out = Path(build).resolve(), Path(out).resolve()
    audit = audit_cue(build / 'disc/maiya.cue')
    text = subprocess.check_output([nm, '-n', str(build / 'program.elf')], text=True)
    symbols = {name: int(addr, 16) for addr, name in re.findall(r'^([0-9a-fA-F]+)\s+\w\s+(\w+)$', text, re.M)}
    out.mkdir(parents=True, exist_ok=False)
    console = NeoCD(core, build / 'disc/maiya.cue', out / 'state')
    audio = bytearray()
    rows = []
    snapshots = []
    try:
        for frame in range(1, frames + 1):
            buttons = []
            # Press Start or A early to advance through eye-catcher and title
            if 60 < frame < 90 or 150 < frame < 180 or 240 < frame < 270:
                buttons.append(3) # START
                buttons.append(0) # A
            # Character select confirm
            if 320 < frame < 360:
                buttons.append(0) # A confirm hero
            # Gameplay inputs: walk right, jump, run
            if 400 < frame < 800:
                buttons.append(7) # right
                if frame % 40 < 15:
                    buttons.append(0) # A (jump)
            if 800 < frame < 1200:
                buttons.append(7) # right
                if frame % 60 < 20:
                    buttons.append(8) # B (throw / attack)
            if 1200 < frame < 1600:
                buttons.append(6) # left
                if frame % 50 < 15:
                    buttons.append(0) # A
            if 1600 < frame < 2400:
                buttons.append(7) # right
                if frame % 30 < 10:
                    buttons.append(0)

            audio.extend(console.step(buttons))
            row = {'frame': frame,
                   'vblanks': console.read(symbols['ng_cd_frames'], 4),
                   'console_start': console.read(symbols['maiya_console_start'], 1),
                   'sfx': console.read(symbols['maiya_cd_sfx_calls'], 4),
                   'music': console.read(symbols['maiya_cd_music_track'], 2)}
            rows.append(row)
            if frame in (90, 210, 350, 600, 900, 1200, 1800, 2400):
                name = f'frame-{frame:04}.png'
                console.snapshot(out / name)
                snapshots.append(name)
        native = read_iso((build / 'disc/maiya.iso').read_bytes())['MAIYA.PRG']
        import ctypes
        if ctypes.string_at(console.ram + 0x100, len(native) - 0x100) != native[0x100:]:
            raise ValueError('Native program bytes changed or a different image was loaded')
        if not any('high level BIOS' in message for message in console.messages):
            raise ValueError('Core did not explicitly identify HLE fallback')
        if rows[-1]['vblanks'] < frames - 150:
            raise ValueError('Game frame clock stalled or reset')
        if max(r['sfx'] for r in rows) < 2:
            raise ValueError('Sound events did not execute')
        peak = max(map(abs, array.array('h', audio)), default=0)
        if peak < 256:
            raise ValueError('Emulator audio capture is silent')
        report = dict(passed=True, scope='native Maiya: Super Nature Girl HLE simulator smoke',
                      stock_bios_verified=False, hardware_tested=False,
                      image_sha256=audit['sha256'], frames=frames,
                      vblanks=rows[-1]['vblanks'],
                      music_tracks=sorted({r['music'] for r in rows}),
                      sfx_calls=max(r['sfx'] for r in rows),
                      audio_peak=peak, screenshots=snapshots,
                      core_sha256=hashlib.sha256(Path(core).read_bytes()).hexdigest())
        (out / 'result.json').write_text(json.dumps(report, indent=2) + '\n')
        return report
    finally:
        (out / 'core.log').write_text(''.join(console.messages))
        if rows:
            with (out / 'state.csv').open('w', newline='') as f:
                writer = csv.DictWriter(f, fieldnames=list(rows[0]))
                writer.writeheader()
                writer.writerows(rows)
        with wave.open(str(out / 'audio.wav'), 'wb') as wav:
            wav.setparams((2, 2, console.rate, 0, 'NONE', ''))
            wav.writeframes(audio)
        console.close()


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--core', type=Path, required=True)
    p.add_argument('--build', type=Path, required=True)
    p.add_argument('--out', type=Path, required=True)
    p.add_argument('--frames', type=int, default=2400)
    cc = default_compiler()
    suffix = '.exe' if cc.endswith('.exe') else ''
    p.add_argument('--nm', default=cc[:-len('gcc' + suffix)] + 'nm' + suffix)
    a = p.parse_args()
    print(json.dumps(run(a.core, a.build, a.out, a.nm, a.frames), indent=2))


if __name__ == '__main__':
    main()
