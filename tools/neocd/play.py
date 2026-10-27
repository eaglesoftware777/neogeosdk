"""Interactive NeoCD HLE simulator for source-built CD test images."""
import argparse
from pathlib import Path
import sys
import time

if __package__ in (None,''):
    sys.path.insert(0,str(Path(__file__).resolve().parents[2]))
from tools.neocd.libretro_host import NeoCD


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--core',type=Path,required=True);p.add_argument('--disc',type=Path,required=True)
    p.add_argument('--state',type=Path,required=True);p.add_argument('--scale',type=int,default=3)
    p.add_argument('--smoke-frames',type=int,default=0,help='Exit after this many frames for host smoke testing')
    a=p.parse_args()
    import pygame
    console=NeoCD(a.core,a.disc,a.state)
    pygame.mixer.pre_init(console.rate,-16,2,2048)
    pygame.init();pygame.joystick.init()
    joysticks=[pygame.joystick.Joystick(i) for i in range(pygame.joystick.get_count())]
    for joystick in joysticks:joystick.init()
    window=pygame.display.set_mode((320*a.scale,224*a.scale),pygame.RESIZABLE)
    pygame.display.set_caption('Sky Lance CD - NeoCD HLE simulator | arrows + A/S/D/F | Enter Start | Esc Exit')
    mapping={pygame.K_UP:4,pygame.K_DOWN:5,pygame.K_LEFT:6,pygame.K_RIGHT:7,
             pygame.K_a:0,pygame.K_s:8,pygame.K_d:1,pygame.K_f:9,
             pygame.K_RETURN:3,pygame.K_BACKSPACE:2}
    channel=pygame.mixer.Channel(0);pending=bytearray();sounds=[]
    clock=pygame.time.Clock();running=True;frames=0
    try:
        while running:
            for event in pygame.event.get():
                if event.type==pygame.QUIT:running=False
                if event.type==pygame.KEYDOWN and event.key==pygame.K_ESCAPE:running=False
            keys=pygame.key.get_pressed();buttons={value for key,value in mapping.items() if keys[key]}
            for joystick in joysticks[:1]:
                if joystick.get_numhats():
                    x,y=joystick.get_hat(0)
                    if x<0:buttons.add(6)
                    if x>0:buttons.add(7)
                    if y<0:buttons.add(5)
                    if y>0:buttons.add(4)
                if joystick.get_numaxes()>=2:
                    x,y=joystick.get_axis(0),joystick.get_axis(1)
                    if x<-.4:buttons.add(6)
                    if x>.4:buttons.add(7)
                    if y<-.4:buttons.add(4)
                    if y>.4:buttons.add(5)
                for index,value in enumerate((0,8,1,9,2,3)):
                    if index<joystick.get_numbuttons() and joystick.get_button(index):buttons.add(value)
            pending.extend(console.step(buttons));frames+=1
            # Queue 1/15-second PCM buffers to avoid a frame-sized mixer underrun.
            chunk=console.rate//15*4
            if len(pending)>=chunk and (not channel.get_busy() or channel.get_queue() is None):
                sound=pygame.mixer.Sound(buffer=bytes(pending[:chunk]));del pending[:chunk]
                if channel.get_busy():channel.queue(sound)
                else:channel.play(sound)
                sounds=(sounds+[sound])[-3:]
            if console.frame:
                raw,w,h,pitch=console.frame
                pixels=b''.join(raw[y*pitch:y*pitch+w*2] for y in range(h))
                surface=pygame.Surface((w,h),depth=16,masks=(0xF800,0x07E0,0x001F,0))
                surface.get_buffer().write(pixels)
                ww,wh=window.get_size();scale=min(ww/w,wh/h)
                dest=pygame.transform.scale(surface,(int(w*scale),int(h*scale)))
                window.fill((0,0,0));window.blit(dest,((ww-dest.get_width())//2,(wh-dest.get_height())//2))
                pygame.display.flip()
            if a.smoke_frames and frames>=a.smoke_frames:running=False
            clock.tick(console.fps)
    finally:
        a.state.mkdir(parents=True,exist_ok=True)
        (a.state/'last-core.log').write_text(''.join(console.messages))
        console.close();pygame.quit()
    print('NeoCD HLE simulator closed after',frames,'frames')


if __name__=='__main__':main()
