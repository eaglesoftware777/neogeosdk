"""Apply the V2 roster's costume, silhouette and movement design to fighter data."""
import json
from pathlib import Path
import numpy as np
from PIL import Image
import fighter_art

GAME=Path(__file__).resolve().parents[1]
OUT=GAME/'assets/authored'
ORDER='kenji somchai amara jack claire viktor linwei batu minjun malik idris arun ryuhei diego niran chenbao aisha kiran elena fahri maya aditya sokha lucas noah zara thabo maia tomas leila kaimana aleksei hayato mason lotus tororojo irina kofi xiaomei zero echo atlas viper mirage'.split()
# id, tailoring, legwear, primary, secondary, hair, footwear, discipline family
SPECS=[
('kenji','gi','trousers','ivory','red','black','bare','karate'),
('somchai','bare','shorts','red','gold','black','bare','thai'),
('amara','jersey','trousers','ivory','green','black','bare','flow'),
('jack','vest','shorts','red','navy','brown','boot','boxing'),
('claire','jacket','trousers','ivory','blue','auburn','boot','savate'),
('viktor','gi','shorts','red','navy','blond','boot','grapple'),
('linwei','mandarin','trousers','black','teal','black','slipper','palm'),
('batu','vest','shorts','brown','blue','black','boot','heavy'),
('minjun','gi','trousers','ivory','blue','black','bare','taekwondo'),
('malik','bare','shorts','gold','black','black','bare','heavy'),
('idris','vest','shorts','ivory','red','black','bare','kickbox'),
('arun','bare','trousers','gold','red','black','bare','low'),
('ryuhei','gi','trousers','navy','ivory','grey','bare','grapple'),
('diego','bare','tights','black','blue','blue','boot','lucha'),
('niran','bare','shorts','navy','ivory','black','bare','thai'),
('chenbao','mandarin','trousers','black','ivory','black','slipper','elbow'),
('aisha','jersey','shorts','ivory','rust','black','bare','boxing'),
('kiran','jacket','shorts','black','violet','black','bare','kickbox'),
('elena','singlet','shorts','blue','ivory','blond','boot','grapple'),
('fahri','bare','trousers','black','black','black','bare','heavy'),
('maya','vest','trousers','black','teal','black','trainer','elbow'),
('aditya','mandarin','trousers','black','gold','black','bare','low'),
('sokha','jersey','shorts','navy','orange','black','bare','thai'),
('lucas','gi','trousers','green','black','brown','bare','grapple'),
('noah','gi','trousers','navy','red','black','bare','karate'),
('zara','jersey','trousers','ivory','plum','black','bare','grapple'),
('thabo','vest','shorts','black','blue','black','boot','boxing'),
('maia','jacket','shorts','slate','orange','brown','boot','grapple'),
('tomas','gi','trousers','black','gold','brown','bare','karate'),
('leila','jacket','shorts','red','ivory','black','bare','kickbox'),
('kaimana','bare','shorts','teal','red','black','bare','heavy'),
('aleksei','gi','shorts','blue','slate','blond','boot','grapple'),
('hayato','gi','trousers','ivory','brown','grey','bare','palm'),
('mason','vest','trousers','slate','red','auburn','boot','boxing'),
('lotus','vest','trousers','black','green','black','bare','low'),
('tororojo','bare','tights','black','red','red','boot','lucha'),
('irina','jacket','trousers','slate','black','grey','boot','palm'),
('kofi','bare','shorts','violet','ivory','black','bare','boxing'),
('xiaomei','mandarin','trousers','black','red','black','slipper','palm'),
('zero','jacket','trousers','slate','violet','white','boot','unstable'),
('echo','armor','tights','slate','teal','white','boot','android'),
('atlas','armor','tights','slate','orange','slate','boot','heavy'),
('viper','armor','tights','violet','green','white','boot','taekwondo'),
('mirage','coat','tights','black','violet','black','boot','palm')]
COLORS={'ivory':(249,242,223),'red':(208,53,67),'rust':(191,91,46),'black':(65,75,94),
        'navy':(53,89,158),'gold':(239,188,66),'green':(62,168,116),'blue':(78,139,225),
        'teal':(45,167,179),'brown':(148,105,75),'auburn':(158,81,57),'blond':(236,204,118),
        'grey':(195,211,224),'white':(247,246,253),'violet':(146,97,206),'orange':(237,133,63),
        'plum':(153,71,143),'slate':(115,135,165)}
FEMALE=set('amara claire aisha kiran elena maya zara maia leila lotus irina xiaomei viper mirage'.split())
HEAVY=set('batu malik fahri kaimana tororojo atlas'.split())


def ramp(name):
    rgb=COLORS[name]
    return [[round(c*s) for c in rgb] for s in (.43,.73,1)]


def palette_png(a,p,path):
    im=Image.fromarray(a.astype('uint8'));im=im.convert('P')
    im.putpalette(np.asarray(p,dtype='uint8').flatten().tolist()+[0]*720)
    im.save(path,transparency=0)


def motion(d,family,number):
    poses=d['poses'];anims=d['anims'];seed=(number%5)-2
    stance=dict(fighter_art.resolve(poses,'stance'))
    if family=='boxing':stance.update(torso=13,head=-11,far_fist=[14,-10],near_fist=[8,-3],far_foot=[13,0],near_foot=[-14,0])
    elif family in ('thai','kickbox'):stance.update(torso=5,head=-5,far_fist=[15,-8],near_fist=[6,-4],far_foot=[14,0],near_foot=[-15,0])
    elif family=='palm':stance.update(torso=2,head=-2,far_fist=[22,-2],near_fist=[12,5],far_foot=[13,0],near_foot=[-10,0],fist='open')
    elif family=='grapple':stance.update(torso=17,head=-15,hip=[0,9],far_fist=[22,5],near_fist=[15,11],far_foot=[17,0],near_foot=[-18,0],fist='open')
    elif family in ('heavy','lucha'):stance.update(torso=10,head=-9,hip=[0,7],far_fist=[23,9],near_fist=[13,15],far_foot=[18,0],near_foot=[-18,0],fist='open')
    elif family in ('low','flow'):stance.update(torso=20,head=-18,hip=[0,13],far_fist=[25,4],near_fist=[4,15],far_foot=[20,0],near_foot=[-22,0],fist='knife' if family=='low' else 'open')
    elif family in ('taekwondo','savate'):stance.update(torso=-4,head=4,far_fist=[20,0],near_fist=[4,10],far_foot=[12,0],near_foot=[-16,0])
    elif family=='elbow':stance.update(torso=11,head=-9,far_fist=[14,-5],near_fist=[8,3],far_foot=[13,0],near_foot=[-14,0])
    elif family=='unstable':stance.update(torso=seed*4,head=-seed*4,far_fist=[22,-4],near_fist=[0,13])
    if family=='karate':stance.update(torso=9,head=-8,hip=[0,8],far_foot=[22,0],near_foot=[-23,0],far_fist=[20,-3],near_fist=[8,8])
    if family in ('thai','kickbox','boxing'):stance.update(hip=[0,6],far_foot=[18,0],near_foot=[-19,0])
    if family in ('grapple','heavy','lucha'):stance.update(hip=[0,13],far_foot=[24,0],near_foot=[-24,0])
    if family in ('taekwondo','savate'):stance.update(hip=[0,6],far_foot=[23,0],near_foot=[-23,0])
    if family=='palm':stance.update(hip=[0,7],far_foot=[18,0],near_foot=[-17,0])
    if d['id']=='atlas':stance['fist']='closed'
    stance['hip'][0]+=seed*.6;stance['far_fist'][0]+=seed;stance['near_fist'][1]+=number%3
    poses['stance']=stance
    if 'down' in poses:poses['down']['hip']=[-4,poses['down'].get('hip',[-10,42])[1]]
    poses['v2_breathe']={'base':'stance','hip':[stance['hip'][0],stance['hip'][1]+1],
                         'far_fist':[stance['far_fist'][0]-1,stance['far_fist'][1]+1]}
    anims['idle']=[['stance',14+number%3],['v2_breathe',14+number%3]]
    # Six genuine foot phases. B is the opposite planted side of A.
    fw=stance['far_foot'][0];bw=stance['near_foot'][0]
    feet=[([fw+2,0],[bw+3,-3]),([fw-8,0],[-2,-6]),([fw-16,0],[fw-4,-3]),
          ([bw+3,-3],[fw+2,0]),([-2,-6],[fw-8,0]),([fw-4,-3],[fw-16,0])]
    for k,(ff,nf) in enumerate(feet):
        poses[f'v2_step{k}']={'base':'stance','far_foot':ff,'near_foot':nf,
            'hip':[stance['hip'][0],stance['hip'][1]+(1 if k in (1,4) else 0)],
            'far_fist':[stance['far_fist'][0]+(1 if k<3 else -1),stance['far_fist'][1]],
            'near_fist':[stance['near_fist'][0]+(-1 if k<3 else 1),stance['near_fist'][1]]}
    holds=[6,4,5,6,4,5] if family not in ('heavy','grapple') else [8,5,6,8,5,6]
    anims['walk_f']=[[f'v2_step{k}',hold] for k,hold in enumerate(holds)]
    anims['walk_b']=[[f'v2_step{k}',holds[k]] for k in (3,2,1,0,5,4)]
    # Preserve every move's startup, active flags, displacement and recovery.
    # Give previously held idle/crouch startup and recovery their own drawings.
    for name,seq in list(anims.items()):
        if not any(len(fr)>2 and 'active' in str(fr[2]) for fr in seq):continue
        active=next(fr for fr in seq if len(fr)>2 and 'active' in str(fr[2]))
        act=fighter_art.resolve(poses,active[0]);base=act
        wind=f'v2_{name}_wind';recover=f'v2_{name}_recover'
        poses[wind]=dict(act);poses[recover]=dict(act)
        for key in ('near_fist','far_fist'):
            normal=stance[key];hit=act[key]
            poses[wind][key]=[round(normal[i]+(hit[i]-normal[i])*.24,1) for i in (0,1)]
            poses[recover][key]=[round(normal[i]+(hit[i]-normal[i])*.47,1) for i in (0,1)]
        poses[wind]['torso']=stance['torso']+(act['torso']-stance['torso'])*.22
        poses[recover]['torso']=stance['torso']+(act['torso']-stance['torso'])*.5
        for key in ('near_foot','far_foot'):
            normal=stance[key];target=act[key]
            poses[wind][key]=[round(normal[i]+(target[i]-normal[i])*.22,1) for i in (0,1)]
            poses[recover][key]=[round(normal[i]+(target[i]-normal[i])*.54,1) for i in (0,1)]
            if target[1]<-18 and target[0]>25:
                poses[wind][key]=[8,-24]
                poses[recover][key]=[12,-18]
        if seq and len(seq[0])<3 and seq[0][0] in ('stance','crouch','jump'):
            seq[0][0]=wind
        if len(seq)>1 and len(seq[-1])<3 and seq[-1][0] in ('stance','crouch'):
            seq[-1][0]=recover
    if family=='boxing':
        # Four buttons are distinct punches; these fighters do not acquire kicks.
        for m in d['moves']:
            if m['kind']=='normal':
                name=m['anim'];seq=anims[name]
                for k,fr in enumerate(seq):
                    if len(fr)>2 and 'active' in str(fr[2]):
                        key=f'v2_{name}_punch';heavy=m['buttons'] in ('C','D','CD')
                        p={'base':'stance','torso':16 if heavy else 10,'hip':[2,5],
                           'near_fist':[30,-9 if m['buttons']=='D' else 2],
                           'far_fist':[10,-10],'near_foot':[-16,0],'far_foot':[13,0]}
                        if m.get('stance')=='crouch':p.update(base='crouch',torso=20,near_fist=[30,6])
                        if m.get('stance')=='air':p.update(base='jump',torso=18,near_fist=[28,13],far_fist=[10,-3],near_foot=[-6,-20],far_foot=[10,-25])
                        poses[key]=p;fr[0]=key;m['limb']='near_fist'
    # Open-hand grapplers and centre-line specialists have recognisable guards.
    if family in ('palm','grapple','heavy','low'):
        for name in ('guard','guard_low'):
            if name in poses:poses[name]['fist']='knife' if family=='low' else 'open'


def main():
    portrait=Image.open(OUT/'portraits-master.png').convert('RGBA')
    (OUT/'heads').mkdir(exist_ok=True);(OUT/'portraits').mkdir(exist_ok=True)
    profiles=[]
    for number,(fid,cut,legwear,primary,secondary,hair,shoe,family) in enumerate(SPECS):
        path=GAME/'fighters'/f'{fid}.json';d=json.loads((OUT/'roster-baseline.json').read_text())[fid];b=d['build']
        # Retain body scale and limb lengths. Remove the mannequin's swollen head.
        b['head_r']=7.7 if fid not in HEAVY else 8.6
        for key in ('thigh_r','arm_r','elbow_r','wrist_r'):b[key]=round(b[key]*(1.10 if fid in FEMALE else 1.15),2)
        b['chest_r']=round(b['chest_r']*(1.06 if fid in FEMALE else 1.14),2)
        if fid in FEMALE:b['waist_r']=min(b['waist_r'],8.2);b['chest_r']=min(b['chest_r'],10.8)
        m={'portrait':fid,'costume':cut,'thigh':'A','shin':'A','foot':'skin','torso':'B',
           'upper_arm':'skin','forearm':'skin','fist':'skin','hair':'C','waistband':'B',
           'piping':cut in ('jersey','jacket','mandarin')}
        if cut=='bare':m.update(torso='skin',muscle=True)
        if cut=='gi':m.update(torso='A',upper_arm='A',sleeve=.50,gi_lapel='A',belt='B')
        if cut=='vest':m.update(torso='skin' if family in ('heavy','boxing') and fid!='jack' else 'B',vest='B')
        if cut=='mandarin':m.update(upper_arm='B',sleeve=.55,buttons=True,belt='B')
        if cut in ('jacket','coat'):m.update(upper_arm='B',sleeve=.72,collar=True,trim='A')
        if cut=='armor':m.update(torso='A',upper_arm='A',forearm='A',fist='A',armor='A')
        if legwear=='shorts':m.update(trunks=.56,shorts='A',shin='skin')
        if fid=='amara':m.update(abada=True,abada_cord='accent',thigh_scale=1.16,shin_scale=1.08)
        if shoe=='boot':m.update(shin_split=.52,boot='B',foot='B')
        elif shoe in ('slipper','trainer'):m['foot']='B'
        if family in ('boxing','thai','kickbox'):
            m.update(wrist_wrap='white')
            if fid in ('jack','thabo'):m.update(fist='A' if fid=='jack' else 'white',gloves=True)
        if fid in ('kenji','niran','somchai'):m['band']='accent'
        if fid in ('diego','tororojo'):m['fist']='B'
        if fid=='atlas':m.update(foot='A',boot='A',fist='A',waistband='B')
        d['materials']=m
        d['palette'].update(A=ramp(primary),B=ramp(secondary),C=ramp(hair),outline=[17,21,31],white=[250,247,235],accent=COLORS['gold' if fid not in ('kenji','viktor','zero') else 'red'])
        # Retain the original skin ramp and alternate colour functionality.
        d['visual_design']={'costume':cut,'legwear':legwear,'family':family,'footwear':shoe,
                            'personality':f"{d['style'].title()}: {cut}, {primary} and {secondary}, {family} guard"}
        motion(d,family,number)
        path.write_text(json.dumps(d,indent=2)+'\n')
        colors=fighter_art.ramp_colors(d['palette']);x=number%8;y=number//8
        box=(round(x*portrait.width/8),round(y*portrait.height/6),round((x+1)*portrait.width/8),round((y+1)*portrait.height/6))
        source=portrait.crop(box)
        # Hardware conversion uses nearest neighbour and explicit existing indices.
        def indexed(im):
            a=np.asarray(im);distance=((a[...,:3].astype('int32')[...,None,:]-colors.astype('int32')[None,None,:,:])**2).sum(-1)
            distance[...,0]=100000000
            out=distance.argmin(-1).astype('uint8');out[a[...,3]<128]=0
            from scipy import ndimage
            lab,_=ndimage.label(out>0,structure=np.ones((3,3)))
            size=np.bincount(lab.ravel());small=(size<3);small[0]=False
            out[small[lab]]=0
            return out
        pi=indexed(source.resize((32,32),Image.Resampling.NEAREST))
        palette_png(pi,colors,OUT/'portraits'/f'{fid}.png')
        cw,ch=source.size
        hi=indexed(source.crop((round(cw*.24),0,round(cw*.96),round(ch*.74))).resize((21,23),Image.Resampling.NEAREST))
        palette_png(hi,colors,OUT/'heads'/f'{fid}.png')
        profiles.append({'id':fid,'name':d['name'],'origin':d['origin'],'style':d['style'],**d['visual_design']})
    (OUT/'profiles.json').write_text(json.dumps(profiles,indent=2)+'\n')
    print('44 costumes, heads, portraits and individual motion models authored')


if __name__=='__main__':main()
