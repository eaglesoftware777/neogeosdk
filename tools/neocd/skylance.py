"""Sky Lance-only native CD port. Reads cartridge assets without rebuilding them."""
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
from .formats import cartridge_sprites_to_spr,validate_wav

ROOT=Path(__file__).resolve().parents[2]
CD=ROOT/'sdk/platform/cd'
GAME=ROOT/'games/skylance'


def convert_graphics(folder):
    art=GAME/'artbox';assets=json.loads((art/'generated/assets.json').read_text())
    end=max(row['tile_last'] for row in assets)+1
    if end>0x7FFD:raise ValueError('Sky Lance art exceeds CD sprite RAM including utility tiles')
    lanes=[(art/'generated'/f'779-{lane}.{lane}').read_bytes() for lane in ('c1','c2')]
    if any(len(lane)!=0x400000 for lane in lanes):raise ValueError('Unexpected Sky Lance cartridge graphics layout')
    # Assets retain every tile ID. Only the cartridge utility tail relocates.
    native=bytearray(0x400000)
    native[:end*128]=cartridge_sprites_to_spr(lanes[0][:end*64],lanes[1][:end*64])
    native[0x7FFD*128:]=cartridge_sprites_to_spr(lanes[0][0xFFFD*64:],lanes[1][0xFFFD*64:])
    for bank in range(4): (folder/f'SPR{bank}.SPR').write_bytes(native[bank*0x100000:(bank+1)*0x100000])
    fix=(art/'779-s1.s1').read_bytes()
    if len(fix)!=0x20000:raise ValueError('Sky Lance FIX must be 128 KiB')
    (folder/'SKY.FIX').write_bytes(fix)
    return dict(assets=len(assets),last_asset_tile=end-1,sprite_bytes=len(native),
                blank_tile=0x7FFF,source_sha256=[hashlib.sha256(lane).hexdigest() for lane in lanes])


def sound_payload(folder,include,obj,wlaz80,wlalink,cdda):
    sound=GAME/'sound/samples';pcm=bytearray();table=[]
    paths=[sound/'out_a'/f'{i}.adpcma' for i in range(1,17)]
    words=('game_over','go','ready','score','stage','start')
    paths += [sound/'out_a_voice'/('word_'+word+'.adpcma') for word in words]
    for path in paths:
        data=path.read_bytes()
        if not data or len(data)%256:raise ValueError('Unaligned ADPCM-A sample '+str(path))
        first=len(pcm)//256;pcm.extend(data);last=len(pcm)//256-1
        table.extend(struct.pack('<HH',first,last))
    if len(pcm)>0x80000:raise ValueError('CD sound bank exceeds 512 KiB')
    (folder/'SFX.PCM').write_bytes(pcm)
    generated=(GAME/'cd/sound.s').read_text()+f'\n.DEFINE sample_count {len(paths)}\nsamples:\n.DB '+','.join(str(v) for v in table)+'\n'
    source=obj/'sound.s';source.write_text(generated)
    subprocess.run([wlaz80,'-o',str(obj/'sound.o'),str(source)],check=True)
    link=obj/'sound.link';link.write_text('[objects]\n'+str(obj/'sound.o')+'\n')
    subprocess.run([wlalink,'-r',str(link),str(folder/'SOUND.Z80')],check=True)
    header=include/'sdk/sound_voice_ids.h';header.parent.mkdir(parents=True)
    header.write_text('#ifndef SOUND_VOICE_IDS_H\n#define SOUND_VOICE_IDS_H\n'+''.join(
        f'#define SOUND_VOICE_WORD_{word.upper()} {i+16}u\n' for i,word in enumerate(words))+'#endif\n')
    audio=[];durations=[]
    if cdda:
        for i in range(1,10):
            source=sound/'out_16el_b'/f'{i}.wav'
            metadata=json.loads(source.with_suffix('.wav.json').read_text())
            target=folder/f'MUSIC{i}.WAV'
            subprocess.run(['ffmpeg','-v','error','-f','s16le','-ar',str(metadata['sample_rate']),
                            '-ac','1','-i',str(source),'-ar','44100','-ac','2','-c:a','pcm_s16le',str(target)],check=True)
            # Pad with silence to the CD audio sector; never truncate music.
            with wave.open(str(target),'rb') as wav:raw=wav.readframes(wav.getnframes())
            raw+=bytes((-len(raw))%2352)
            with wave.open(str(target),'wb') as wav:
                wav.setparams((2,2,44100,0,'NONE',''));wav.writeframes(raw)
            info=validate_wav(target);durations.append(math.ceil(info['frames']*60/44100))
            audio.append('payload/'+target.name)
    (include/'sky_cd_audio.h').write_text('#ifndef SKY_CD_AUDIO_H\n#define SKY_CD_AUDIO_H\n'+
        f'#define SKY_CD_SAMPLE_COUNT {len(paths)}u\n#define SKY_CD_TRACK_COUNT {len(audio)}u\n'+
        'static const uint32_t sky_cd_track_frames[]={'+','.join(str(n)+'ul' for n in (durations or [1]))+'};\n#endif\n')
    return audio,dict(pcm_bytes=len(pcm),samples=len(paths),tracks=len(audio))


def build_skylance(out,cc,plus,cdda,wlaz80,wlalink):
    if plus:raise ValueError('Sky Lance CD currently uses the original C engine; C++ game port is not enabled')
    out=Path(out).resolve()
    if out.exists() and any(out.iterdir()):raise ValueError('Use a new CD output directory')
    out.mkdir(parents=True,exist_ok=True)
    obj,payload,include=out/'obj',out/'payload',out/'include'
    for p in (obj,payload,include):p.mkdir()
    graphics=convert_graphics(payload)
    audio,sound=sound_payload(payload,include,obj,wlaz80,wlalink,cdda)
    cc=shutil.which(cc) or cc;suffix='.exe' if cc.endswith('.exe') else ''
    prefix=cc[:-len('gcc'+suffix)]
    flags=['-m68000','-O2','-ffreestanding','-fno-builtin','-fno-common',
           '-ffunction-sections','-fdata-sections','-Wall','-Wextra',
           '-I'+str(include),'-I'+str(ROOT),'-I'+str(ROOT/'sdk'),'-I'+str(ROOT/'sdk/2d_engine'),
           '-I'+str(GAME/'artbox'),'-I'+str(CD),'-DNG_PLATFORM_CD=1','-DNG_CD_SKYLANCE=1',
           '-DNG_CD_GAME_ID=0x0779','-DNG_PALFX_SCREEN=1','-DNG_VRAM_DEFER=1','-DNG_SHOOTER=1',
           '-DNG_SPRITE_BLANK_TILE=0x7FFFu']
    modules=('ng_defs ng_properties ng_game_time ng_timers ng_progress ng_status ng_game_events '
             'ng_level ng_vram ng_sprite_window ng_art_asset ng_palette_assets ng_bg ng_fix '
             'ng_sprite_group ng_actions ng_chars ng_npcs ng_physics ng_border_constraints '
             'ng_game_interupt ng_scene ng_depthfx ng_render_queue ng_fixed ng_camera '
             'ng_palette_fx ng_particles_none ng_feedback ng_debug ng_joystick ng_rand '
             'ng_shooter ng_trig ng_trig_table ng_pause ng_move').split()
    sources=[CD/'ng_cd_boot.S',CD/'ng_cd_bios.S',CD/'ng_cd_input.c',CD/'ng_cd_system.c',
             GAME/'cd/platform.c',GAME/'cd/audio.c',GAME/'main.c',
             *[GAME/'scenes'/(name+'.c') for name in ('sky_draw','sky_stage','sky_sortie','sky_combat')],
             ROOT/'sdk/ng_fix/ng_fix.c',*[ROOT/'sdk/2d_engine'/(name+'.c') for name in modules]]
    objects=[];hashes={}
    for index,source in enumerate(sources):
        path=obj/(f'{index:02}_'+source.stem+'.o')
        hashes[source.relative_to(ROOT).as_posix()]=hashlib.sha256(source.read_bytes()).hexdigest()
        subprocess.run([cc,*flags,*(['-std=gnu99'] if source.suffix=='.c' else []),'-c',str(source),'-o',str(path)],check=True)
        objects.append(str(path))
    elf=out/'program.elf'
    subprocess.run([cc,'-m68000','-nostdlib','-Wl,--gc-sections','-Wl,-T,'+str(CD/'neocd.ld'),
                    '-Wl,-Map,'+str(out/'program.map'),*objects,'-lgcc','-o',str(elf)],check=True)
    subprocess.run([prefix+'objcopy'+suffix,'-O','binary',str(elf),str(payload/'SKY.PRG')],check=True)
    program=(payload/'SKY.PRG').read_bytes()
    if len(program)&1: (payload/'SKY.PRG').write_bytes(program+b'\0')
    for name,digest in hashes.items():
        if hashlib.sha256((ROOT/name).read_bytes()).hexdigest()!=digest:raise ValueError('Source changed during CD build: '+name)
    records=[dict(name=name,source='payload/'+name,group='boot') for name in ('SKY.FIX','SFX.PCM','SOUND.Z80','SKY.PRG')]
    records += [dict(name=f'SPR{bank}.SPR',source=f'payload/SPR{bank}.SPR',group='boot',bank=bank) for bank in range(4)]
    manifest=dict(format='neogeosdk-cd-1',game='skylance',volume='SKY_LANCE_CD',
                  groups={'boot':{'depends':[]}},files=records,audio=audio,
                  port_status='emulator development candidate; stock BIOS/hardware unverified',
                  graphics=graphics,sound=sound,source_sha256=hashes)
    source=out/'source-manifest.json';source.write_text(json.dumps(manifest,indent=2)+'\n')
    return build_disc(source,out/'disc',int(os.environ.get('SOURCE_DATE_EPOCH','946684800')))
