"""Build ORIGINAL 512 KiB CD firmware; never install into cartridge BIOS paths."""
import argparse
import datetime
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[3]
CD = ROOT / 'bios/cd'


def compiler_default():
    versions=('x-tools-v3-win','x-tools-v2-win','x-tools-v3','x-tools-v2','x-tools') if os.name=='nt' else (
        'x-tools-v3','x-tools-v2','x-tools','x-tools-v3-win','x-tools-v2-win')
    for version in versions:
        for suffix in (('.exe',) if os.name=='nt' else ('','.exe')):
            p = ROOT.parent / version / 'm68k-unknown-elf/bin' / ('m68k-unknown-elf-gcc' + suffix)
            if p.is_file():
                return str(p)
    return 'm68k-unknown-elf-gcc'


def build(out, cc, stock_rom=None):
    if out.exists() and any(out.iterdir()):
        raise ValueError('Use a new or empty firmware output directory; previous builds are preserved')
    out.mkdir(parents=True, exist_ok=True)
    cc = shutil.which(cc) or cc
    suffix = '.exe' if cc.endswith('.exe') else ''
    ending = 'gcc' + suffix
    if not cc.endswith(ending):
        raise ValueError('--cc must select a 68000 GCC compiler')
    prefix = cc[:-len(ending)]
    spec = importlib.util.spec_from_file_location('eagle_original_font', ROOT / 'bios/tools/gen_sfix.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    font = module.build_rom()[:4096]
    font_source = out / 'font.c'
    font_source.write_text('#include <stdint.h>\nconst uint8_t eagle_cd_font[]={\n' +
                           ','.join(str(b) for b in font) + '\n};\n')
    result = {'version': '1.7.7', 'status': 'EagleBIOS-CD bootable firmware', 'profiles': {}}
    flags = ['-m68000', '-O2', '-ffreestanding', '-fno-builtin', '-fno-common',
             '-Wall', '-Wextra', '-Werror', '-I' + str(CD / 'include')]
    for ident, model in enumerate(('front', 'top', 'cdz')):
        folder = out / model
        folder.mkdir(exist_ok=True)
        objects = []
        for source in [CD / 'src/cd_vectors.S', *sorted((CD / 'src').glob('*.c')), font_source]:
            obj = folder / (source.stem + '.o')
            subprocess.run([cc, *flags, '-DCD_MODEL=' + str(ident), '-c', str(source), '-o', str(obj)], check=True)
            objects.append(str(obj))
        elf, raw = folder / 'eagle_cd.elf', folder / 'raw.bin'
        subprocess.run([prefix + 'ld' + suffix, '-T', str(CD / 'cd_bios.ld'),
                        '-Map=' + str(folder / 'eagle_cd.map'), *objects, '-o', str(elf)], check=True)
        subprocess.run([prefix + 'objcopy' + suffix, '-O', 'binary', str(elf), str(raw)], check=True)
        data = raw.read_bytes()
        if len(data) > 0x80000 or data[:4] != bytes.fromhex('0010f300'):
            raise ValueError('Invalid CD firmware vectors or ROM size')
        pc = int.from_bytes(data[4:8], 'big')
        if not 0xC01000 <= pc < 0xC80000 or pc & 1 or data[0x700:0x708] != b'EAGLECD\0':
            raise ValueError('Invalid CD reset address or identification')
        rom = out / ('eagle_cd_' + model + '.rom')
        if stock_rom is not None and Path(stock_rom).is_file():
            base = bytearray(Path(stock_rom).read_bytes())
            if len(base) == 0x80000:
                if base[:4] == bytes.fromhex('100000f3'):
                    unswapped = bytearray(len(base))
                    for i in range(0, len(base), 2):
                        unswapped[i] = base[i+1]
                        unswapped[i+1] = base[i]
                    base = unswapped
                base[:len(data)] = data
                rom.write_bytes(base)
                # Also emit word-swapped MAME ROM for emulators that expect swapped words
                mame_rom = out / ('mame_eagle_cd_' + model + '.rom')
                swapped = bytearray(len(base))
                for i in range(0, len(base), 2):
                    swapped[i] = base[i+1]
                    swapped[i+1] = base[i]
                mame_rom.write_bytes(swapped)
            else:
                rom.write_bytes(data.ljust(0x80000, b'\xff'))
        else:
            rom.write_bytes(data.ljust(0x80000, b'\xff'))
        result['profiles'][model] = dict(file=rom.name, size=rom.stat().st_size,
                                        sha256=hashlib.sha256(rom.read_bytes()).hexdigest(), reset=hex(pc))
    (out / 'firmware.json').write_text(json.dumps(result, indent=2) + '\n')
    return result


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--out', type=Path, help='New/empty output directory; defaults to timestamped out/neocd/bios/')
    p.add_argument('--cc', default=compiler_default())
    a = p.parse_args()
    if a.out is None:
        stamp=datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S%fZ')
        a.out=ROOT/'out/neocd/bios'/stamp
    print(json.dumps(build(a.out.resolve(), a.cc), indent=2))


if __name__ == '__main__':
    main()
