"""Small host for an external NeoCD libretro core; no emulator/BIOS is embedded."""
import ctypes as c
from pathlib import Path


class Info(c.Structure):
    _fields_=[('path',c.c_char_p),('data',c.c_void_p),('size',c.c_size_t),('meta',c.c_char_p)]
class Variable(c.Structure):_fields_=[('key',c.c_char_p),('value',c.c_char_p)]
class Geometry(c.Structure):
    _fields_=[('base_width',c.c_uint),('base_height',c.c_uint),('max_width',c.c_uint),
              ('max_height',c.c_uint),('aspect_ratio',c.c_float)]
class Timing(c.Structure):_fields_=[('fps',c.c_double),('sample_rate',c.c_double)]
class AV(c.Structure):_fields_=[('geometry',Geometry),('timing',Timing)]


class NeoCD:
    """Callbacks are retained until close; caller owns video/audio consumption."""
    def __init__(self,core_path,disc,state):
        self.core_path=Path(core_path).resolve();self.disc=Path(disc).resolve()
        state=Path(state).resolve();(state/'system/neocd').mkdir(parents=True,exist_ok=True)
        (state/'save').mkdir(exist_ok=True)
        if any((state/'system/neocd').iterdir()):
            raise ValueError('HLE test needs an empty private system/neocd directory')
        self.frame=None;self.audio=bytearray();self.messages=[];self.errors=[];self.buttons=set()
        self.paths={9:str(state/'system').encode(),31:str(state/'save').encode()}
        self.options={b'neocd_overscan_h':b'0',b'neocd_cdspeedhack':b'Off',
                      b'neocd_loadskip':b'Off',b'neocd_cpu_overclock':b'100%'}
        self.core=c.CDLL(str(self.core_path));self.callbacks=[];self.loaded=False
        def callback(name,signature,function):
            cb=signature(function);self.callbacks.append(cb)
            getattr(self.core,'retro_set_'+name)(cb)
        self.logger=c.CFUNCTYPE(None,c.c_int,c.c_char_p)(lambda level,msg:self.messages.append(msg.decode('utf-8','replace')) if level>0 else None)
        callback('environment',c.CFUNCTYPE(c.c_bool,c.c_uint,c.c_void_p),self.environment)
        callback('video_refresh',c.CFUNCTYPE(None,c.c_void_p,c.c_uint,c.c_uint,c.c_size_t),self.video)
        callback('audio_sample',c.CFUNCTYPE(None,c.c_int16,c.c_int16),lambda left,right:None)
        callback('audio_sample_batch',c.CFUNCTYPE(c.c_size_t,c.c_void_p,c.c_size_t),self.audio_batch)
        callback('input_poll',c.CFUNCTYPE(None),lambda:None)
        callback('input_state',c.CFUNCTYPE(c.c_int16,c.c_uint,c.c_uint,c.c_uint,c.c_uint),
                 lambda port,device,index,button:int(port==0 and device==1 and button in self.buttons))
        self.core.retro_get_memory_data.argtypes=[c.c_uint];self.core.retro_get_memory_data.restype=c.c_void_p
        self.core.retro_init()
        self.core.retro_load_game.argtypes=[c.POINTER(Info)];self.core.retro_load_game.restype=c.c_bool
        info=Info(str(self.disc).encode(),None,0,None)
        self.loaded=self.core.retro_load_game(c.byref(info))
        if not self.loaded:
            self.core.retro_deinit();raise ValueError('NeoCD rejected disc: '+''.join(self.messages))
        self.av=AV();self.core.retro_get_system_av_info(c.byref(self.av))
        self.rate=int(self.av.timing.sample_rate);self.fps=self.av.timing.fps
        self.ram=self.core.retro_get_memory_data(2)
        self.vram=self.core.retro_get_memory_data(3)
        self.core.retro_serialize_size.restype=c.c_size_t

    def environment(self,command,data):
        command &= 0xffff
        if command==27:
            c.cast(data,c.POINTER(c.c_void_p))[0]=c.cast(self.logger,c.c_void_p).value;return True
        if command in self.paths:
            c.cast(data,c.POINTER(c.c_char_p))[0]=self.paths[command];return True
        if command==15:
            var=c.cast(data,c.POINTER(Variable)).contents
            if var.key in self.options:var.value=self.options[var.key];return True
            return False
        if command==10:return c.cast(data,c.POINTER(c.c_int))[0]==2
        if command==3:c.cast(data,c.POINTER(c.c_bool))[0]=True;return True
        return command in (11,16,18,35,36,44)

    def video(self,pointer,width,height,pitch):
        if not pointer:return
        if not 1<=width<=320 or height!=224 or pitch<width*2:
            self.errors.append('Unexpected video dimensions');return
        self.frame=(c.string_at(pointer,pitch*height),width,height,pitch)

    def audio_batch(self,pointer,n):
        if pointer:self.audio.extend(c.string_at(pointer,n*4))
        return n

    def step(self,buttons=()):
        self.buttons=set(buttons);self.audio.clear();self.core.retro_run()
        if self.errors:raise RuntimeError(';'.join(self.errors))
        return bytes(self.audio)

    def read(self,address,size=1):
        if not 0<=address<=0x200000-size:raise ValueError('RAM read outside CD program memory')
        return int.from_bytes(c.string_at(self.ram+address,size),'big')

    def write(self,address,value,size=1):
        if not 0<=address<=0x200000-size:raise ValueError('RAM write outside CD program memory')
        c.memmove(self.ram+address,value.to_bytes(size,'big'),size)

    def snapshot(self,path):
        from PIL import Image
        import sys
        if not self.frame:raise ValueError('No emulator frame received')
        raw,width,height,pitch=self.frame;rgb=bytearray()
        for y in range(height):
            for x in range(width):
                v=int.from_bytes(raw[y*pitch+x*2:y*pitch+x*2+2],sys.byteorder)
                rgb.extend((((v>>11)&31)*255//31,((v>>5)&63)*255//63,(v&31)*255//31))
        Image.frombytes('RGB',(width,height),bytes(rgb)).save(path)

    def close(self):
        if self.loaded:self.core.retro_unload_game();self.loaded=False
        self.core.retro_deinit()
