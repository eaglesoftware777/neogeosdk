"""Native No Signal CD port with resident fighters and lossless scenery overlays."""
import hashlib
import json
import math
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import wave

from .build_disc import build as build_disc
from .formats import cartridge_sprites_to_spr, validate_wav

ROOT = Path(__file__).resolve().parents[2]
CD = ROOT / 'sdk/platform/cd'
GAME = ROOT / 'games/nosignal'


def verify_v2():
    reference=json.loads((GAME/'cd/v2_reference.json').read_text())
    for name,expected in reference['files'].items():
        data = (ROOT/name).read_bytes()
        if name == 'games/nosignal/scenes/ns_draw.c':
            data = re.sub(rb'#ifdef NG_CD_NOSIGNAL\n.*?#endif\n', b'', data, flags=re.S)
        if hashlib.sha256(data).hexdigest()!=expected:
            raise ValueError('V2 source or artwork differs: '+name)
    return reference


BG_BASE = 0x7000
UTILITY_BASE = 0x7FFD


def descriptor(name, bank, offset):
    """An even-aligned BIOS filename list, kept in resident program memory."""
    from .formats import Entry
    Entry(name, bank, offset).validate(128)
    data = bytearray(name.encode('ascii') + b'\0' + bytes([bank]))
    if len(data) & 1:
        data.append(0)
    data.extend(struct.pack('>I', offset))
    return bytes(data + b'\0')


def convert_graphics(folder, include):
    import zlib
    verify_v2()
    art = GAME / 'artbox'
    assets = json.loads((art / 'generated/assets.json').read_text())['assets']
    lanes = [(art / f'784-{lane}.{lane}').read_bytes() for lane in ('c1', 'c2')]
    if any(len(lane) != 0x400000 for lane in lanes):
        raise ValueError('Unexpected No Signal cartridge graphics layout')
    resident = bytearray(BG_BASE * 128)
    next_tile = 0x100  # retain the system's reserved leading tiles
    mapping, backgrounds, rows = {}, [], []
    for asset in assets:
        count, first = asset['tiles'], asset['tile_base']
        native = cartridge_sprites_to_spr(lanes[0][first*64:(first+count)*64], lanes[1][first*64:(first+count)*64])
        if asset['kind'] == 'background':
            if BG_BASE + count > UTILITY_BASE:
                raise ValueError('Background exceeds CD sprite overlay slot')
            mapping[asset['id']] = BG_BASE
            name = f"BG{asset['id']:02}.SPR"
            (folder / name).write_bytes(native)
            backgrounds.append(dict(id=asset['id'], name=name, tiles=count, crc32=zlib.crc32(native),
                                    bank=3, offset=0x80000, size=len(native)))
        else:
            if next_tile + count > BG_BASE:
                raise ValueError('Resident fighters exceed the CD graphics budget')
            mapping[asset['id']] = next_tile
            resident[next_tile*128:(next_tile+count)*128] = native
            next_tile += count
    for bank in range(4):
        chunk = resident[bank*0x100000:(bank+1)*0x100000]
        (folder / f'SPR{bank}.SPR').write_bytes(chunk)
    utility = cartridge_sprites_to_spr(lanes[0][0xFFFD*64:], lanes[1][0xFFFD*64:])
    (folder / 'UTIL.SPR').write_bytes(utility)
    (folder / 'NS.FIX').write_bytes((art / '784-s1.s1').read_bytes())
    header = (art / 'generated/ns_assets.h').read_text()
    for asset in assets:
        kind = 2 if asset['kind'] == 'background' else 1
        old = f"{{{asset['id']}u,{kind}u,{asset['bank']}u,{asset['tile_base']}u,{asset['tile_base']+asset['tiles']-1}u,"
        new = f"{{{asset['id']}u,{kind}u,{asset['bank']}u,{mapping[asset['id']]}u,{mapping[asset['id']]+asset['tiles']-1}u,"
        if header.count(old) != 1:
            raise ValueError('Asset header disagrees with manifest: ' + asset['name'])
        header = header.replace(old, new)
    shadow = include / 'games/nosignal/artbox/generated/ns_assets.h'
    shadow.parent.mkdir(parents=True)
    shadow.write_text(header)
    for bg in backgrounds:
        data = descriptor(bg['name'], bg['bank'], bg['offset'])
        rows.append(f"static const uint8_t ns_cd_desc_{bg['id']}[] __attribute__((aligned(2))) = {{" + ','.join(str(x) for x in data) + '};')
    rows.append('typedef struct { uint8_t id; uint32_t bytes, crc; const uint8_t *descriptor; } NSCDBg;')
    rows.append('static const NSCDBg ns_cd_backgrounds[] = {')
    rows += [f"{{{b['id']}u,{b['size']}ul,0x{b['crc32']:08X}ul,ns_cd_desc_{b['id']}}}," for b in backgrounds]
    rows += ['};', '#define NS_CD_BG_OFFSET 0x80000ul']
    crc = []
    for n in range(256):
        v=n
        for _ in range(8): v=(v>>1)^ (0xEDB88320 if v&1 else 0)
        crc.append(v)
    rows.append('static const uint32_t ns_cd_crc_table[] = {' + ','.join(f'0x{x:08X}ul' for x in crc) + '};')
    (include / 'nosignal_cd_backgrounds.h').write_text('\n'.join(rows)+'\n')
    return dict(assets=len(assets), resident_tiles=next_tile, background_base=BG_BASE,
                utility_base=UTILITY_BASE, sprite_bytes=0x400000, backgrounds=backgrounds,
                tile_mapping=mapping, source_sha256=[hashlib.sha256(x).hexdigest() for x in lanes])


def sound_payload(folder, include, obj, wlaz80, wlalink, cdda):
    sound = GAME / 'sound/samples'
    pcm = bytearray()
    table = []
    paths = [sound / 'out_a' / f'{i}.adpcma' for i in range(1, 48)]
    for path in paths:
        data = path.read_bytes()
        if not data or len(data) % 256:
            raise ValueError('Unaligned ADPCM-A sample ' + str(path))
        first = len(pcm) // 256
        pcm.extend(data)
        last = len(pcm) // 256 - 1
        table.extend(struct.pack('<HH', first, last))
    if len(pcm) > 0x80000:
        raise ValueError('CD sound bank exceeds 512 KiB')
    (folder / 'SFX.PCM').write_bytes(pcm)
    generated = (GAME / 'cd/sound.s').read_text() + f'\n.DEFINE sample_count {len(paths)}\nsamples:\n.DB ' + ','.join(str(v) for v in table) + '\n'
    source = obj / 'sound.s'
    source.write_text(generated)
    subprocess.run([wlaz80, '-o', str(obj / 'sound.o'), str(source)], check=True)
    link = obj / 'sound.link'
    link.write_text('[objects]\n' + str(obj / 'sound.o') + '\n')
    subprocess.run([wlalink, '-r', str(link), str(folder / 'SOUND.Z80')], check=True)
    audio = []
    durations = []
    if cdda:
        for i in range(1, 19):
            src_file = sound / 'out_16el_b' / f'{i}.wav'
            meta = json.loads(src_file.with_suffix('.wav.json').read_text())
            target = folder / f'MUSIC{i}.WAV'
            subprocess.run(['ffmpeg', '-v', 'error', '-f', 's16le', '-ar', str(meta['sample_rate']),
                            '-ac', '1', '-i', str(src_file), '-ar', '44100', '-ac', '2', '-c:a', 'pcm_s16le', str(target)], check=True)
            with wave.open(str(target), 'rb') as wav:
                raw = wav.readframes(wav.getnframes())
            raw += bytes((-len(raw)) % 2352)
            with wave.open(str(target), 'wb') as wav:
                wav.setparams((2, 2, 44100, 0, 'NONE', ''))
                wav.writeframes(raw)
            info = validate_wav(target)
            durations.append(math.ceil(info['frames'] * 60 / 44100))
            audio.append('payload/' + target.name)
    (include / 'nosignal_cd_audio.h').write_text('#ifndef NS_CD_AUDIO_H\n#define NS_CD_AUDIO_H\n' +
        f'#define NS_CD_SAMPLE_COUNT {len(paths)}u\n#define NS_CD_TRACK_COUNT {len(audio)}u\n' +
        'static const uint32_t ns_cd_track_frames[]={' + ','.join(str(n) + 'ul' for n in (durations or [1])) + '};\n#endif\n')
    return audio, dict(pcm_bytes=len(pcm), samples=len(paths), tracks=len(audio))


def build_nosignal(out, cc, plus, cdda, wlaz80, wlalink, media_audit=False):
    if plus:
        raise ValueError('No Signal Z Alpha V2 uses its original C beat-em-up engine')
    out = Path(out).resolve()
    if out.exists() and any(out.iterdir()):
        raise ValueError('Use a new CD output directory')
    out.mkdir(parents=True, exist_ok=True)
    obj, payload, include = out / 'obj', out / 'payload', out / 'include'
    for p in (obj, payload, include):
        p.mkdir()
    graphics = convert_graphics(payload, include)
    audio, sound = sound_payload(payload, include, obj, wlaz80, wlalink, cdda)
    cc = shutil.which(cc) or cc
    suffix = '.exe' if cc.endswith('.exe') else ''
    prefix = cc[:-len('gcc' + suffix)]
    flags = ['-m68000', '-O2', '-g3', '-gdwarf-2', '-ffreestanding', '-fno-builtin', '-fno-common',
             '-ffunction-sections', '-fdata-sections', '-Wall', '-Wextra',
             '-I' + str(include), '-I' + str(ROOT), '-I' + str(ROOT / 'sdk'), '-I' + str(ROOT / 'sdk/2d_engine'),
             '-I' + str(GAME), '-I' + str(GAME / 'scenes'), '-I' + str(GAME / 'artbox'), '-I' + str(GAME / 'artbox/generated'),
             '-I' + str(CD), '-DNG_PLATFORM_CD=1', '-DNG_CD_NOSIGNAL=1',
             '-DNG_CD_GAME_ID=0x0784', '-DNG_PALFX_SCREEN=1', '-DNG_VRAM_DEFER=1',
             '-DNG_SOUND_QUEUE=1', '-DNG_CD_SAVE_BYTES=32', '-DNG_SPRITE_BLANK_TILE=0x7FFFu']
    modules = ('ng_defs ng_properties ng_game_time ng_timers ng_progress ng_status ng_game_events '
               'ng_level ng_vram ng_sprite_window ng_art_asset ng_palette_assets ng_bg ng_fix '
               'ng_sprite_group ng_actions ng_chars ng_npcs ng_physics ng_border_constraints '
               'ng_game_interupt ng_scene ng_depthfx ng_render_queue ng_fixed ng_camera '
               'ng_palette_fx ng_particles_none ng_feedback ng_debug ng_joystick ng_rand '
               'ng_demo_advanced ng_trig ng_trig_table ng_pause ng_move ng_beatemup').split()
    sources = [CD / 'ng_cd_boot.S', CD / 'ng_cd_bios.S', CD / 'ng_cd_input.c', CD / 'ng_cd_system.c',
               GAME / 'cd/platform.c', GAME / 'cd/audio.c', GAME / 'main.c',
               GAME / 'scenes/ns_game.c', GAME / 'scenes/ns_draw.c', GAME / 'scenes/ns_data.c',
               ROOT / 'sdk/ng_fix/ng_fix.c', *[ROOT / 'sdk/2d_engine' / (name + '.c') for name in modules]]
    objects = []
    hashes = {}
    if media_audit:
        sources.append(GAME / 'cd/media_audit.c')
    for index, source in enumerate(sources):
        path = obj / (f'{index:02}_' + source.stem + '.o')
        hashes[source.relative_to(ROOT).as_posix()] = hashlib.sha256(source.read_bytes()).hexdigest()
        special = ['-Dng_cd_start=ns_cd_game_start'] if media_audit and source == GAME/'cd/platform.c' else []
        subprocess.run([cc, *flags, *special, *(['-std=gnu99'] if source.suffix == '.c' else []), '-c', str(source), '-o', str(path)], check=True)
        objects.append(str(path))
    elf = out / 'program.elf'
    subprocess.run([cc, '-m68000', '-nostdlib', '-Wl,--gc-sections', '-Wl,-T,' + str(CD / 'neocd.ld'),
                    '-Wl,-Map,' + str(out / 'program.map'), *objects, '-lgcc', '-o', str(elf)], check=True)
    subprocess.run([prefix + 'objcopy' + suffix, '-O', 'binary', str(elf), str(payload / 'NS.PRG')], check=True)
    program = (payload / 'NS.PRG').read_bytes()
    if len(program) & 1:
        (payload / 'NS.PRG').write_bytes(program + b'\0')
    for name, digest in hashes.items():
        if hashlib.sha256((ROOT / name).read_bytes()).hexdigest() != digest:
            raise ValueError('Source changed during CD build: ' + name)
    verify_v2()
    records = [dict(name=name, source='payload/' + name, group='boot') for name in ('NS.FIX', 'SFX.PCM', 'SOUND.Z80', 'NS.PRG')]
    records += [dict(name=f'SPR{bank}.SPR', source=f'payload/SPR{bank}.SPR', group='boot', bank=bank) for bank in range(4)]
    records.append(dict(name='UTIL.SPR', source='payload/UTIL.SPR', group='boot', bank=3, offset=0xFFE80))
    groups = {'boot': {'depends': []}}
    for bg in graphics['backgrounds']:
        group = f"scene_{bg['id']}"
        groups[group] = {'depends': ['boot']}
        records.append(dict(name=bg['name'], source='payload/'+bg['name'], group=group, bank=bg['bank'], offset=bg['offset']))
    manifest = dict(format='neogeosdk-cd-1', game='nosignal', volume='NOSIGNAL_V2_CD',
                    groups=groups, files=records, audio=audio,
                    port_status='media diagnostic; not a playable release' if media_audit else 'native CD port of V2 game and artwork',
                    media_audit=bool(media_audit),
                    graphics=graphics, sound=sound, source_sha256=hashes)
    source = out / 'source-manifest.json'
    source.write_text(json.dumps(manifest, indent=2) + '\n')
    return build_disc(source, out / 'disc', int(os.environ.get('SOURCE_DATE_EPOCH', '946684800')))
