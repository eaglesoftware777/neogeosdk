"""Native-grid cel drawing: shaped anatomy, tailored cloth and character heads."""
from functools import lru_cache
from pathlib import Path
import math
import numpy as np
from PIL import Image, ImageDraw

W, H = 160, 160
ROOT = Path(__file__).resolve().parents[1]
RAMP = {'skin': (2,3,4), 'A': (5,6,7), 'B': (8,9,10), 'C': (11,12,13),
        'white': (14,14,14), 'accent': (15,15,15)}


class Ink:
    def __init__(self):
        self.image = Image.new('P', (W,H), 0)
        self.draw = ImageDraw.Draw(self.image)

    def poly(self, pts, color, edge=None):
        pts = [tuple(int(round(v)) for v in p) for p in pts]
        self.draw.polygon(pts, fill=color)
        if edge is not None:
            self.draw.line(pts+[pts[0]], fill=edge, width=1)

    def line(self, a, b, color, width=1):
        self.draw.line([tuple(round(v) for v in a),tuple(round(v) for v in b)], fill=color, width=width)

    def dot(self,p,color):
        x,y=[round(v) for v in p]
        if 0<=x<W and 0<=y<H:self.image.putpixel((x,y),color)


def axes(a,b):
    u=np.asarray(b)-np.asarray(a);u=u/max(1,np.hypot(*u))
    n=np.array([u[1],-u[0]])
    if n[0]<0:n=-n
    return u,n


def limb(c,a,b,r0,r1,material,far=False,cloth=False):
    a,b=np.asarray(a),np.asarray(b);u,n=axes(a,b);d=b-a
    # Deltoid/thigh, taper, tendon: seven deliberate contour changes.
    pts=[a-u*1.5-n*r0*.78,a+u*2-n*r0,
         a+d*.36-n*r0*.92,b-u*2-n*r1,b+u*1.5,
         b+n*r1,a+d*.32+n*r0*.82,a+n*r0*.7]
    lo,mid,hi=RAMP[material]
    c.poly(pts,lo if far else mid,1)
    c.poly([a+u*2-n*r0*.7,a+d*.48-n*r0*.66,b-u*2-n*r1*.4,
            b-u*3+n*r1*.05,a+d*.32+n*r0*.12,a+u*3],mid if far else hi)
    c.poly([a+d*.48+n*r0*.5,b+n*r1*.73,b-u*1.5+n*r1*.12,a+d*.64+n*r0*.22],lo)
    if cloth:
        p=a+d*.63;c.line(p-n*r1*.75,p+n*r1*.30+u*2,lo)
        p=a+d*.79;c.line(p-n*r1*.34,p+n*r1*.70-u*1.5,lo)
    else:
        p=a+d*.44;c.line(p+n*r0*.45,b-u*3+n*r1*.25,lo)


def foot(c,b,j,side,m,far):
    a=j[side+'_ankle'];k=j[side+'_knee'];ground=a[1]>=141-b['ankle']-1.5
    u,n=axes(k,a);lo,mid,hi=RAMP[m['foot']]
    length=b['foot'];bare=m['foot']=='skin'
    if ground:
        sole=min(141,round(a[1]+b['ankle']));heel=a[0]-b['ankle_r']
        toe=a[0]+length
        c.poly([(heel,a[1]-2),(a[0]+2,a[1]-2),(a[0]+4,sole-3),
                (toe,sole-2),(toe+1,sole),(heel-1,sole)],lo if far else mid,1)
        c.line((heel,sole-1),(toe,sole-1),lo)
        c.line((a[0]+1,a[1]),(toe-2,sole-3),mid if far else hi,2)
        if bare:
            c.dot((toe-1,sole-2),lo);c.dot((toe-3,sole-2),lo)
        else:
            for dx in (2,4,6):c.line((a[0]+dx,a[1]),(a[0]+dx-1,a[1]+2),14)
    else:
        front=u*.48+n*.88;front=front/max(.1,np.hypot(*front))
        other=np.array([-front[1],front[0]]);toe=a+front*length
        if toe[1]>138:
            toe[1]=138
            front=(toe-a)/max(1,np.hypot(*(toe-a)));other=np.array([-front[1],front[0]])
        c.poly([a-u*2-other*3,a-u*2+other*3,a+front*3+other*2.8,
                toe+other*2.2,toe+front*1.2-other*2.1,a-other*2.6],lo if far else mid,1)
        c.line(a+front*2-other,toe-other,mid if far else hi,2)
        c.line(a+other*2.5,toe+other*2,lo)


def leg(c,b,j,side,m,far):
    h,k,a=[j[side+'_'+part] for part in ('hip','knee','ankle')]
    thighs=b['thigh_r']*m.get('thigh_scale',1);shin=b['knee_r']*m.get('shin_scale',1)
    if m.get('trunks'):
        end=h+(k-h)*m['trunks'];limb(c,h,k,thighs,b['knee_r'],'skin',far)
        limb(c,h,end,thighs*1.14,thighs*.95,m.get('shorts',m['thigh']),far,True)
        _,n=axes(h,k);c.line(end-n*thighs*.7,end+n*thighs*.6,15)
    else:
        limb(c,h,k,thighs*1.13,b['knee_r'],m['thigh'],far,True)
    limb(c,k,a,shin,b['ankle_r'], 'skin' if m.get('trunks') else m['shin'],far,not m.get('trunks'))
    u,n=axes(k,a)
    c.line(k-n*b['knee_r']*.66,k+n*b['knee_r']*.55+u*2,RAMP['skin' if m.get('trunks') else m['thigh']][0])
    if m.get('shin_split'):
        start=k+(a-k)*m['shin_split'];limb(c,start,a,b['ankle_r']*1.65,b['ankle_r']*1.1,m['boot'],far,True)
        for t in (.3,.52,.75):
            p=start+(a-start)*t;c.line(p-n*1.5,p+n*1.5+u,14)
    if m.get('abada'):
        for t in (.2,.43,.68):
            p=k+(a-k)*t;c.line(p-n*shin*.60,p+n*shin*.52+u*2,RAMP[m['shin']][0])
    foot(c,b,j,side,m,far)


def hand(c,b,a,f,material,kind,far,glove=False):
    u,n=axes(a,f);r=b['fist_r'];lo,mid,hi=RAMP[material]
    if kind in ('open','knife','claw') and not glove:
        tip=f+u*(r*1.55)
        c.poly([f-u*2-n*r*.65,f+u*r*.8-n*r*.7,tip-n*r*.36,
                tip+n*r*.35,f+u*r*.4+n*r*.65,f-u*2+n*r*.66],lo if far else mid,1)
        c.line(f-n*r*.38,tip-n*r*.10,mid if far else hi)
        c.poly([f+n*r*.45,f-u*r*.5+n*r,f-u*2+n*r*.65],mid)
        c.dot(f+u*r*.1+n*r*.45,lo)
    else:
        c.poly([f-u*r*.70-n*r*.62,f+u*r*.45-n*r*.95,
                f+u*r-n*r*.40,f+u*r+n*r*.60,f+u*.1+n*r,
                f-u*r*.70+n*r*.57],lo if far else mid,1)
        c.line(f-u*r*.30-n*r*.53,f+u*r*.5-n*r*.54,mid if far else hi,2)
        c.line(f+u*r*.66-n*r*.25,f+u*r*.66+n*r*.50,lo)
        c.line(f-u*r*.33+n*r*.3,f+u*r*.33+n*r*.3,lo)
        c.dot(f-u*r*.42+n*r*.60,mid if far else hi)


def arm(c,b,j,side,m,kind,far):
    s,e,f=[j[side+'_'+part] for part in ('shoulder','elbow','fist')]
    limb(c,s,e,b['arm_r']*1.1,b['elbow_r'],m['upper_arm'],far,m['upper_arm']!='skin')
    limb(c,e,f,b['elbow_r']*1.06,b['wrist_r'],m['forearm'],far,m['forearm']!='skin')
    u,n=axes(e,f)
    if m.get('sleeve'):
        end=e+(f-e)*m['sleeve'];limb(c,e,end,b['elbow_r']*1.38,b['wrist_r']*1.45,m['upper_arm'],far,True)
        c.line(end-n*b['wrist_r']*1.2,end+n*b['wrist_r']*1.2,14)
    if m.get('wrist_wrap'):
        for step in range(3):
            p=f-u*(3+step*2);c.line(p-n*b['wrist_r'],p+n*b['wrist_r'],14 if step%2==0 else RAMP[m['wrist_wrap']][0])
    hand(c,b,e,f,m['fist'],kind,far,m.get('gloves',False))


def torso(c,b,j,m):
    h=j['hip'];up=j['up'];n=np.array([-up[1],up[0]])
    neck=j['neck'];ch=h+up*(b['torso']*.78);wa=b['waist_r'];cr=b['chest_r'];lo,mid,hi=RAMP[m['torso']]
    c.poly([h-n*wa,h+up*8-n*(wa+1),ch-n*cr,neck-n*4,
            neck+n*4,ch+n*cr,h+up*8+n*(wa+1),h+n*wa],mid,1)
    c.poly([h-up-n*wa*.63,h+up*12-n*wa*.66,ch-n*cr*.68,neck-n*3,
            neck+up*2,ch-n*cr*.1,h+up*7-n*wa*.17],hi)
    c.poly([h+n*wa*.5,ch+n*cr*.48,neck+n*3,ch+n*cr*.87,h+n*wa*.83],lo)
    if m.get('muscle') or m['torso']=='skin':
        center=h+up*b['torso']*.68
        c.line(center-n*cr*.50,center+n*cr*.32-up*2,2)
        c.line(center+up*5-n*cr*.44,center+up*4+n*cr*.32,4)
        for t in (8,12,16):c.line(h+up*t-n*2,h+up*t+n*4,2)
    if m.get('gi_lapel'):
        apex=h+up*12
        c.poly([neck-n*5,ch-n*cr*.30,apex,neck+n*3],3,1)
        c.line(neck-n*5,apex+n*1,RAMP[m['gi_lapel']][2],2)
        c.line(neck+n*5,apex-n*2,RAMP[m['gi_lapel']][0],2)
    elif m.get('collar'):
        c.poly([neck-n*5-up*2,neck+n*5-up*2,neck+n*6-up*7,neck-n*6-up*7],RAMP[m.get('trim','B')][1],1)
        c.line(neck-up*9,h+up*7,14)
    if m.get('vest'):
        vl,vm,vh=RAMP[m['vest']]
        for sign in (-1,1):
            c.poly([neck+n*sign*4,ch+n*sign*(cr+.6),h+n*sign*(wa+.6),h+n*sign*(wa*.3),ch+n*sign*cr*.28],vm,1)
            c.line(ch+n*sign*cr*.70,h+n*sign*wa*.67,vh)
    if m.get('armor'):
        c.poly([ch-up*7-n*(cr-1),ch-up*7+n*(cr-1),ch-up*17+n*(wa+1),ch-up*17-n*(wa+1)],RAMP[m['armor']][1],1)
        c.line(ch-up*8-n*(cr-3),ch-up*8+n*(cr-3),RAMP[m['armor']][2])
        c.poly([ch-up*9-n*2,ch-up*9+n*2,ch-up*13+n*2,ch-up*13-n*2],15,1)
        c.dot(ch-up*10,14)
    if m['torso']!='skin':
        for t,dx in ((6,-3),(11,2),(20,-4)):
            p=h+up*t+n*dx;c.line(p-n*3,p+n*2-up*2,lo)
        if m.get('piping'):
            c.line(ch-n*(cr-1),h+up*5-n*(wa-1),15)
        if m.get('buttons'):
            for t in (12,18,24):
                p=h+up*t;c.line(p-n*2,p+n*3,14);c.dot(p+n*3,1)
    belt=m.get('waistband',m.get('belt'))
    if belt:
        bl,bm,bh=RAMP[belt]
        c.poly([h-n*(wa+1),h+n*(wa+1),h-up*3+n*(wa+1),h-up*3-n*(wa+1)],bm,1)
        c.line(h-n*wa,h+n*wa,bh)
        if m.get('belt') or m.get('abada_cord'):
            c.line(h+n*2,h-up*10+n*4,bl,2);c.line(h+n*3,h-up*8+n*7,bh)


@lru_cache(maxsize=64)
def head_image(name):
    p=ROOT/'assets/authored/heads'/f'{name}.png'
    assert p.exists(),f'Missing authored head: {name}'
    return Image.open(p).copy()


def head(c,b,j,m,pose):
    center=j['head'];n=j['neck'];limb(c,n,center,b['neck_r'],b['neck_r']*.78,'skin')
    im=head_image(m['portrait'])
    angle=pose['torso']+pose['head']
    if abs(pose['torso'])>65:
        im=im.transpose(Image.Transpose.ROTATE_90 if pose['torso']<0 else Image.Transpose.ROTATE_270)
    x,y=round(center[0]-im.width/2),round(center[1]-im.height/2)
    a=np.array(im);mask=Image.fromarray(np.where(a>0,255,0).astype('uint8'))
    c.image.paste(im,(x,y),mask)
    if m.get('band') and abs(pose['torso'])<65:
        c.line((x+7,y+8),(x-4,y+11),15,2)
        c.line((x+7,y+9),(x-3,y+14),15)


def render(build,mat,pose,j):
    c=Ink();layers={};composite=np.zeros((H,W),dtype=np.uint8)
    for name,draw in [('far_leg',lambda:leg(c,build,j,'far',mat,True)),
                      ('far_arm',lambda:arm(c,build,j,'far',mat,pose.get('fist','closed'),True)),
                      ('torso',lambda:torso(c,build,j,mat)),
                      ('near_leg',lambda:leg(c,build,j,'near',mat,False)),
                      ('head',lambda:head(c,build,j,mat,pose)),
                      ('near_arm',lambda:arm(c,build,j,'near',mat,pose.get('fist','closed'),False))]:
        c=Ink();draw();after=np.array(c.image)
        layers[name]=after>0
        composite[after>0]=after[after>0]
    return composite,layers
