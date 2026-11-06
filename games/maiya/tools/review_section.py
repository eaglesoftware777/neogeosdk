"""Replay forest routes or movement through ordinary controls and a matching ELF.

Does not install ROMs or write game memory. Requires debug type information in
the private build (GAME_OPTIMIZE='-O2 -g3 -gdwarf-2' preserves normal behavior).
"""
import argparse
import csv
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import zipfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--workspace', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--hero', choices=('maiya', 'luna'), default='maiya')
    parser.add_argument('--route', choices=('ground', 'canopy', 'motion', 'forest', 'camera', 'bricks'), default='ground')
    parser.add_argument('--platform', choices=('mvs', 'aes'), default='mvs')
    parser.add_argument('--frames', action='store_true', help='Retain every native motion frame')
    parser.add_argument('--observe-video', action='store_true', help='Check actual gameplay video/palette stores')
    parser.add_argument('--observe-queues', action='store_true', help='Read pending group/age diagnostics')
    parser.add_argument('--observe-publication', action='store_true', help='Trace completed batch ownership and visible pixels')
    parser.add_argument('--use-art', action='store_true', help='Spend one available Secret Art in the later forest encounter')
    parser.add_argument('--mame', default='/usr/games/mame')
    parser.add_argument('--m68k-prefix', default='/mnt/c/neogeo/x-tools-v3/m68k-unknown-elf/bin/m68k-unknown-elf-')
    args = parser.parse_args()
    if args.use_art and args.route != 'forest':
        parser.error('--use-art belongs to the full forest replay')
    root, out = args.workspace.resolve(), args.out.resolve()
    out.mkdir(parents=True, exist_ok=False)
    elf = root / 'out/game'
    expressions = {'base': '(unsigned long)&mg', 'pick': '(unsigned long)&mg_chooser_pick'}
    if args.route in ('camera', 'bricks'):
        for name in ('road', 'far'):
            expressions[name + '_first'] = f'(unsigned long)&mg.{name}.firstSprite'
        expressions['ledge_first'] = '(unsigned long)&mg.ledges[0].firstSprite'
        expressions['ledge_owner'] = '(unsigned long)&mg_ledge_owner[0]'
    for name in ('player', 'state', 'demo', 'tick', 'stage', 'lives', 'has_key',
                 'climbing', 'on_ledge', 'hero_choice', 'hurt', 'gait', 'walk_distance', 'pick_mask', 'cam_y',
                 'gait_stop', 'run_hold', 'facing', 'plant', 'cast', 'attack', 'turning',
                 'sq_t', 'flash', 'veil', 'gait_clock', 'art'):
        expressions[name] = f'(unsigned long)&mg.{name}-(unsigned long)&mg'
    for name in ('boss', 'boss_active', 'boss_timer', 'gate_unlocked', 'state_timer', 'rescue_mask'):
        expressions[name] = f'(unsigned long)&mg.{name}-(unsigned long)&mg'
    for name in ('x', 'y', 'x_fp', 'vx_fp', 'vy_fp', 'sprite_first', 'sprite_tile', 'hp', 'scale_x', 'scale_y', 'sprite_offset_y', 'visible'):
        expressions[name] = f'(unsigned long)&((NGCharacter*)0)->{name}'
    if args.observe_queues or args.observe_publication:
        for name in ('ngsg_high', 'ngsg_norm', 'ngsg_low', 'ngsg_high_n', 'ngsg_norm_n',
                     'ngsg_low_n', 'ngsg_head_age'):
            expressions[name] = f'(unsigned long)&{name}'
        for name in ('firstSprite', 'strips', 'heightTiles', 'dirty', 'visible', 'mapRows', 'tileBase', 'hflip'):
            expressions['group_' + name] = f'(unsigned long)&((NGSpriteGroup*)0)->{name}'
    if args.observe_publication:
        for name in ('mg_video_sync', 'ng_chars', 'ng_char_groups', 'ng_char_uploaded_first'):
            expressions[name] = f'(unsigned long)&{name}'
        expressions['character_size'] = 'sizeof(NGCharacter)'
        expressions['group_size'] = 'sizeof(NGSpriteGroup)'
    cmd = [args.m68k_prefix + 'gdb', '-nx', '-batch', str(elf)]
    for name, expr in expressions.items():
        cmd += ['-ex', f'printf "{name}=%lu\\n", {expr}']
    layout = {k: int(v) for k, v in re.findall(r'^(\w+)=(\d+)$',
              subprocess.check_output(cmd, text=True), re.M)}
    if layout.keys() != expressions.keys():
        raise ValueError('Incomplete matching ELF layout')
    parts = {p.name: p.read_bytes() for p in (root / 'roms/maiya').glob('780-*')}
    p1 = parts['780-p1.p1']
    raw = out / 'program.bin'
    subprocess.run([args.m68k_prefix + 'objcopy', '-O', 'binary', str(elf), str(raw)], check=True)
    native = bytearray(p1); native[0::2], native[1::2] = p1[1::2], p1[0::2]
    if raw.read_bytes() != native[:raw.stat().st_size]:
        raise ValueError('ELF does not match program ROM')
    raw.unlink()
    shutil.copy2(elf, out / 'game')
    (out / 'roms').mkdir(); (out / 'hash').mkdir()
    with zipfile.ZipFile(out / 'roms/maiya.zip', 'x', zipfile.ZIP_DEFLATED) as rom:
        for name, data in parts.items(): rom.writestr(name, data)
    shutil.copy2(root / 'hash_eagle/maiya/neogeo.xml', out / 'hash/neogeo.xml')
    level = json.loads((root / 'games/maiya/levels/01_emerald_forest.json').read_text())
    config = dict(layout=layout, output=str(out), hero=int(args.hero == 'luna'),
                  route=args.route, level=level, frames=args.frames, use_art=args.use_art)
    if args.frames:
        (out / 'frames').mkdir()
    if args.route != 'motion':
        config['canopy_end'] = next(i for i, p in enumerate(level['platforms'], 1)
                                   if p['x'] <= level['key']['x'] <= p['x'] + p['w'] and
                                   p['y'] - 40 <= level['key']['y'] <= p['y'] - 16)
        bough = level['platforms'][2]
        reward = next(i for i, p in enumerate(level['pickups'])
                      if bough['x'] <= p['x'] and p['x'] + 32 <= bough['x'] + bough['w'] and
                      p['y'] + 32 <= bough['y'])

    def lua(v):
        if isinstance(v, dict): return '{' + ','.join('[' + lua(k) + ']=' + lua(x) for k, x in v.items()) + '}'
        if isinstance(v, list): return '{' + ','.join(lua(x) for x in v) + '}'
        if isinstance(v, bool): return 'true' if v else 'false'
        return json.dumps(v)

    script = out / 'capture.lua'
    capture = {'motion': 'motion_capture.lua', 'forest': 'forest_capture.lua',
               'bricks': 'bricks_capture.lua'}.get(
        args.route, 'section_capture.lua')
    script.write_text('local q=' + lua(config) + '\n' +
                      Path(__file__).with_name(capture).read_text())
    if args.observe_video:
        with script.open('a') as stream:
            stream.write('\n' + Path(__file__).with_name('video_observer.lua').read_text())
    if args.observe_queues:
        with script.open('a') as stream:
            stream.write('\n' + Path(__file__).with_name('queue_observer.lua').read_text())
    if args.observe_publication:
        with script.open('a') as stream:
            stream.write('\n' + Path(__file__).with_name('publication_observer.lua').read_text())
    manifest = dict(workspace=str(root), platform=args.platform, hero=args.hero,
                    route=args.route, normal_inputs_only=True, native_frames=args.frames,
                    video_observer=args.observe_video, queue_observer=args.observe_queues, layout=layout,
                    publication_observer=args.observe_publication,
                    use_art=args.use_art,
                    elf_sha256=hashlib.sha256(elf.read_bytes()).hexdigest(),
                    rom_sha256={k: hashlib.sha256(v).hexdigest() for k, v in parts.items()},
                    level_sha256=hashlib.sha256((root / 'games/maiya/levels/01_emerald_forest.json').read_bytes()).hexdigest())
    manifest['source_sha256'] = {name: hashlib.sha256((root / name).read_bytes()).hexdigest()
        for name in ('games/maiya/scenes/maiya_game.c', 'games/maiya/user.c',
                     'games/maiya/scenes/maiya_levels_data.h', 'games/maiya/scenes/maiya_presentation.h',
                     'sdk/2d_engine/ng_chars.c', 'sdk/2d_engine/ng_chars.h',
                     'sdk/2d_engine/ng_sprite_group.c', 'sdk/2d_engine/ng_sprite_group.h',
                     'sdk/2d_engine/ng_frame_sync.h') if (root / name).is_file()}
    (out / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    cmd = [args.mame, 'aes' if args.platform == 'aes' else 'neogeo', '-cart1', 'maiya',
           '-bios', 'asia' if args.platform == 'aes' else 'euro', '-noreadconfig',
           '-skip_gameinfo', '-video', 'none', '-sound', 'none', '-nothrottle',
           '-nonvram_save', '-seconds_to_run', '180' if args.route == 'forest' else '105', '-rompath',
           str(out / 'roms') + ';' + str(root / 'bios/test_roms'), '-hashpath', str(out / 'hash'),
           '-cfg_directory', str(out / 'cfg'), '-nvram_directory', str(out / 'nvram'),
           '-autoboot_delay', '0', '-autoboot_script', str(script)]
    env = os.environ.copy()
    env.pop('DISPLAY', None)
    env.pop('WAYLAND_DISPLAY', None)
    env.update(SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy', QT_QPA_PLATFORM='offscreen')
    with (out / 'mame.log').open('w') as log:
        subprocess.run(cmd, env=env, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=240)
    rows = list(csv.DictReader((out / 'section.csv').open()))
    video = None
    if args.observe_video:
        samples = list(csv.DictReader((out / 'video-stores.csv').open()))
        if not samples:
            raise ValueError('Video observer recorded no samples')
        video = {k: int(v) for k, v in samples[-1].items() if k != 'seconds'}
        (out / 'video-result.json').write_text(json.dumps(video, indent=2) + '\n')
        if not video['game_vram'] or not video['game_palette']:
            raise ValueError('Video taps were not validated by actual gameplay stores')
        if video['active_vram'] or video['active_palette']:
            raise ValueError('Gameplay wrote video/palette RAM during active display')
        if video.get('scanline_peak', 0) > 96:
            raise ValueError('Conservative scanline strip bound exceeds96; inspect before hardware acceptance')
    if args.observe_publication:
        from publication_review import validate as validate_publication
        publication = validate_publication(list(csv.DictReader((out / 'publications.csv').open())))
        (out / 'publication-result.json').write_text(json.dumps(publication, indent=2) + '\n')
        if not publication['passed']:
            raise ValueError('Completed publication did not match visible body; see publication-result.json')
    if args.route == 'forest':
        from forest_review import validate
        from review_gait import array
        header = (root / 'games/maiya/artbox/generated/maiya_assets.h').read_text()
        result = validate(rows, array(header, 'mg_hero_tiles'), config['hero'])
        result.update(hero=args.hero, platform=args.platform, normal_inputs_only=True)
        (out / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
        print(json.dumps(result, indent=2))
        if args.use_art and result['art_spent'] != 1:
            raise ValueError('Forest replay did not spend exactly one available Secret Art')
        if not result['passed']:
            raise ValueError('Forest rendering gate failed; see result.json for mixed/late samples')
        return
    if args.route == 'motion':
        from motion_review import validate
        from review_gait import array
        header = (root / 'games/maiya/artbox/generated/maiya_assets.h').read_text()
        if any(int(r['hero']) != config['hero'] for r in rows):
            raise ValueError('Actual chooser selected the wrong heroine')
        result = validate(rows, array(header, 'mg_hero_tiles'),
                          array(header, 'mg_hero_alt_pal' if args.hero == 'luna' else 'mg_hero_pal'))
        result.update(hero=args.hero, platform=args.platform, normal_inputs_only=True)
        (out / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
        print(json.dumps(result, indent=2))
        return
    if args.route in ('camera', 'bricks'):
        if args.route == 'bricks':
            from camera_review import validate_bricks as validate
        else:
            from camera_review import validate
        result = validate(rows)
        result.update(hero=args.hero, platform=args.platform, normal_inputs_only=True)
        (out / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
        print(json.dumps(result, indent=2))
        return
    if not rows or rows[-1]['phase'] != 'complete':
        raise ValueError('Section incomplete; inspect section.csv and native snapshots')
    if any(int(r['hero']) != config['hero'] or int(r['stage']) != 0 for r in rows):
        raise ValueError('Wrong heroine or stage')
    if len({int(r['lives']) for r in rows}) != 1:
        raise ValueError('Section lost a life')
    if any(int(r['hp']) < int(rows[0]['hp']) for r in rows):
        raise ValueError('Opening route took damage')
    if any(int(r['scb_rows']) == 0 for r in rows):
        raise ValueError('Active heroine lost her rendered rows')
    if not int(rows[-1]['key']) or int(rows[-1]['x']) < 1408:
        raise ValueError('Did not obtain key and reach the section exit')
    result = dict(passed=True, route=args.route, hero=args.hero, platform=args.platform,
                  normal_inputs_only=True, hardware_samples=len(rows),
                  key_obtained=True, end_x=int(rows[-1]['x']),
                  hp_start=int(rows[0]['hp']), hp_end=int(rows[-1]['hp']),
                  life_lost=False, seconds=float(rows[-1]['seconds']) - float(rows[0]['seconds']))
    result['canopy_reward_collected'] = bool(int(rows[-1]['pick_mask']) & (1 << reward))
    if args.route == 'canopy':
        if not any(r['phase'] == 'canopy' and int(r['on_ledge']) and int(r['y']) == bough['y']
                   and bough['x'] - 4 <= int(r['x']) <= bough['x'] + bough['w'] + 4 for r in rows):
            raise ValueError('Canopy route was not traversed')
        if not result['canopy_reward_collected']:
            raise ValueError('Canopy reward was not collected')
        if any(int(r['canvas_top']) < 32 or int(r['canvas_bottom']) > 224 for r in rows):
            raise ValueError('Hero left safe vertical framing')
    (out / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
