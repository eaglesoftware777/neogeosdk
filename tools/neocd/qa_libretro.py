"""Optional HLE-only startup diagnostic. NEVER counts as stock/Eagle BIOS QA.

Requires a separately built NeoCD libretro core with its own HLE fallback.
No core implementation or firmware is bundled. All emulator state is private.
"""
import argparse
import array
import ctypes as c
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import wave

if __package__ in (None,''):
    sys.path.insert(0,str(Path(__file__).resolve().parents[2]))
from tools.neocd.audit_disc import audit_cue
from tools.neocd.build import default_compiler


def run(core_path,build,out,nm):
    from PIL import Image
    core_path,build,out=Path(core_path).resolve(),Path(build).resolve(),Path(out).resolve()
    audit=audit_cue(build/'disc/helloworld.cue')
    symbols_text=subprocess.check_output([nm,'-n',str(build/'program.elf')],text=True)
    symbols={n:int(a,16) for a,n in re.findall(r'^([0-9a-fA-F]+)\s+\w\s+(\w+)$',symbols_text,re.M)}
    out.mkdir(parents=True,exist_ok=False)
    system,save=out/'system',out/'save'
    (system/'neocd').mkdir(parents=True);save.mkdir()
    core=c.CDLL(str(core_path))
    env_type=c.CFUNCTYPE(c.c_bool,c.c_uint,c.c_void_p)
    video_type=c.CFUNCTYPE(None,c.c_void_p,c.c_uint,c.c_uint,c.c_size_t)
    audio_type=c.CFUNCTYPE(None,c.c_int16,c.c_int16)
    batch_type=c.CFUNCTYPE(c.c_size_t,c.c_void_p,c.c_size_t)
    poll_type=c.CFUNCTYPE(None)
    input_type=c.CFUNCTYPE(c.c_int16,c.c_uint,c.c_uint,c.c_uint,c.c_uint)
    log_type=c.CFUNCTYPE(None,c.c_int,c.c_char_p)
    class Info(c.Structure):
        _fields_=[('path',c.c_char_p),('data',c.c_void_p),('size',c.c_size_t),('meta',c.c_char_p)]
    class Variable(c.Structure):
        _fields_=[('key',c.c_char_p),('value',c.c_char_p)]
    class Geometry(c.Structure):
        _fields_=[('base_width',c.c_uint),('base_height',c.c_uint),('max_width',c.c_uint),
                  ('max_height',c.c_uint),('aspect_ratio',c.c_float)]
    class Timing(c.Structure):_fields_=[('fps',c.c_double),('sample_rate',c.c_double)]
    class AV(c.Structure):_fields_=[('geometry',Geometry),('timing',Timing)]
    paths={9:str(system).encode(),31:str(save).encode()}
    variables={b'neocd_overscan_h':b'0',b'neocd_cdspeedhack':b'Off',b'neocd_loadskip':b'Off'}
    messages,errors,audio=[],[],bytearray()
    current_frame=0
    @log_type
    def logger(level,message):
        if level>0:messages.append(message.decode('utf-8','replace'))
    @env_type
    def environment(command,data):
        command &= 0xffff
        if command==27:
            c.cast(data,c.POINTER(c.c_void_p))[0]=c.cast(logger,c.c_void_p).value;return True
        if command in paths:
            c.cast(data,c.POINTER(c.c_char_p))[0]=paths[command];return True
        if command==15:
            var=c.cast(data,c.POINTER(Variable)).contents
            if var.key in variables:var.value=variables[var.key];return True
            return False
        if command==10:return c.cast(data,c.POINTER(c.c_int))[0]==2 # RGB565
        if command==3:c.cast(data,c.POINTER(c.c_bool))[0]=True;return True
        return command in (11,16,18,35,36,44)
    @video_type
    def video(ptr,w,h,pitch):
        if not ptr or current_frame not in (120,480,600,900):return
        try:
            if w>320 or h!=224 or pitch<w*2:raise ValueError('Unexpected native video dimensions')
            raw=c.string_at(ptr,pitch*h);rgb=bytearray()
            for y in range(h):
                for x in range(w):
                    value=int.from_bytes(raw[y*pitch+x*2:y*pitch+x*2+2],sys.byteorder)
                    rgb.extend((((value>>11)&31)*255//31,((value>>5)&63)*255//63,(value&31)*255//31))
            Image.frombytes('RGB',(w,h),bytes(rgb)).save(out/f'frame-{current_frame}.png')
        except Exception as e:errors.append(str(e))
    @audio_type
    def sample(left,right):pass
    @batch_type
    def batch(ptr,n):
        if ptr:audio.extend(c.string_at(ptr,n*4))
        return n
    @poll_type
    def poll():pass
    @input_type
    def input_state(port,device,index,button):
        if port!=0 or device!=1:return 0
        return int((button==7 and 500<current_frame<530) or (button==0 and 650<current_frame<662))
    # Keep all callbacks alive through retro_deinit.
    callbacks=[('environment',environment),('video_refresh',video),('audio_sample',sample),
               ('audio_sample_batch',batch),('input_poll',poll),('input_state',input_state)]
    for name,callback in callbacks:getattr(core,'retro_set_'+name)(callback)
    core.retro_init()
    core.retro_load_game.argtypes=[c.POINTER(Info)];core.retro_load_game.restype=c.c_bool
    core.retro_get_memory_data.argtypes=[c.c_uint];core.retro_get_memory_data.restype=c.c_void_p
    info=Info(str(build/'disc/helloworld.cue').encode(),None,0,None)
    loaded=core.retro_load_game(c.byref(info))
    try:
        if not loaded:raise ValueError('Core rejected disc')
        av=AV();core.retro_get_system_av_info(c.byref(av))
        rate=int(av.timing.sample_rate)
        if rate<=0 or rate!=av.timing.sample_rate:raise ValueError('Unsupported core audio rate')
        positions={}
        audio_markers={}
        for current_frame in range(1,901):
            core.retro_run()
            if current_frame in (600,640,650,720):audio_markers[current_frame]=len(audio)
            if current_frame in (480,600):
                memory=core.retro_get_memory_data(2)
                positions[current_frame]=int.from_bytes(c.string_at(memory+symbols['object_x'],2),'big')
        ram=c.string_at(core.retro_get_memory_data(2),0x200000)
    finally:
        if loaded:core.retro_unload_game()
        core.retro_deinit()
    (out/'core.log').write_text(''.join(messages))
    if errors:raise ValueError('; '.join(errors))
    if not any('high level BIOS' in message for message in messages):
        raise ValueError('Core did not explicitly identify HLE fallback; cannot classify this test')
    frames=int.from_bytes(ram[symbols['ng_cd_frames']:symbols['ng_cd_frames']+4],'big')
    pad=int.from_bytes(ram[symbols['ng_cd_input_seen']:symbols['ng_cd_input_seen']+2],'big')
    if frames<300 or pad&0x18!=0x18 or positions[600]<=positions[480]:
        raise ValueError('HLE diagnostic did not run the native program and accept inputs')
    image=Image.open(out/'frame-900.png').convert('RGB')
    if len(image.getcolors(image.width*image.height))<3:raise ValueError('Native image is blank')
    prg=(build/'payload/HELLO.PRG').read_bytes()
    if ram[0x100:len(prg)]!=prg[0x100:]:raise ValueError('Loaded native program differs from source-built PRG')
    with wave.open(str(out/'audio.wav'),'wb') as wav:
        wav.setparams((2,2,rate,0,'NONE',''));wav.writeframes(audio)
    def peak(first,last):
        samples=array.array('h',audio[audio_markers[first]:audio_markers[last]])
        return max(map(abs,samples),default=0)
    quiet_peak,sfx_peak=peak(600,640),peak(650,720)
    if quiet_peak>64 or sfx_peak<512:
        raise ValueError('ADPCM-A cue was missing or pre-button audio was unexpectedly noisy')
    report=dict(diagnostic_passed=True,firmware='NeoCD built-in HLE',stock_gate_passed=False,
                eagle_gate_passed=False,core_sha256=hashlib.sha256(core_path.read_bytes()).hexdigest(),
                image_sha256=audit['sha256'],native_frames=frames,pad_seen=pad,
                sprite_x_before=positions[480],sprite_x_after=positions[600],
                audio_rate=rate,fps=av.timing.fps,
                adpcm_a_hle_capture=True,adpcm_a_peak=sfx_peak,pre_button_peak=quiet_peak,
                cdda_playback=False,save_persistence=False,hardware_tested=False)
    (out/'result.json').write_text(json.dumps(report,indent=2)+'\n')
    return report


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--hle-debug',action='store_true',required=True)
    p.add_argument('--core',type=Path,required=True);p.add_argument('--build',type=Path,required=True)
    p.add_argument('--out',type=Path,required=True)
    cc=default_compiler();suffix='.exe' if cc.endswith('.exe') else ''
    p.add_argument('--nm',default=cc[:-len('gcc'+suffix)]+'nm'+suffix)
    a=p.parse_args();print(json.dumps(run(a.core,a.build,a.out,a.nm),indent=2))


if __name__=='__main__':main()
