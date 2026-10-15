"""Explicit native-grid study drawings. Run only to create a fresh study directory.

Editable indexed PNG exports, not this recipe, are the cartridge's source of truth.
Each contour and shading mark below is specified in whole pixel coordinates.
No skeleton, IK, capsule, interpolation, rotation, resampling or outline filter.
"""
import json
from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT/'assets/authored/rebirth'
LAYERS = ['far_arm','far_leg','torso','head_hair','near_leg','near_arm']
# Steel-cool shade, warm key light. Native five-bit hardware-compatible colors.
COLORS = {
 'kenji': ['000000','101020','784858','c88068','f0b898','505880','9898b0','e0d8d0','682838','b84050','f06058','181828','303048','585070','f8e8d0','e8b858'],
 'jack': ['000000','101020','603848','a86050','d89870','502038','a83848','e85058','182840','304868','5878a0','181828','403040','705048','f8e0b8','e8b858'],
 'amara': ['000000','181828','583048','a05848','d89868','485878','a0b0c0','e0e8e0','184850','287878','50b0a0','181828','382838','604048','f8e8c0','e8b858']}

def ink():
 im=Image.new('P',(160,160));return im,ImageDraw.Draw(im)
def poly(d,pts,c): d.polygon(pts,fill=c)
def line(d,pts,c,w=1):d.line(pts,fill=c,width=w)
def rect(d,b,c):d.rectangle(b,fill=c)
def shape(d,pts,fill):poly(d,pts,1); # outline is explicitly supplied separately below

def face(d,who):
 # Separate head drawings, deliberately distinct profile and cranial structure.
 if who=='kenji':
  poly(d,[(68,22),(75,18),(84,19),(89,24),(90,34),(94,38),(90,40),(89,47),(82,52),(73,48),(69,40)],1)
  poly(d,[(72,26),(85,24),(87,33),(91,38),(87,39),(87,45),(81,49),(75,46),(72,38)],3)
  poly(d,[(75,28),(84,26),(85,34),(88,37),(84,40),(84,45),(79,46),(77,37)],4)
  poly(d,[(67,24),(72,18),(80,16),(86,20),(89,25),(87,30),(82,26),(77,27),(74,32),(72,36),(68,32)],11)
  poly(d,[(69,24),(76,20),(82,19),(80,22),(74,24),(71,28)],13)
  line(d,[(70,29),(84,28),(89,30)],8,3);line(d,[(71,29),(84,29)],10)
  line(d,[(81,33),(86,34)],1);rect(d,(83,35,85,35),14);rect(d,(85,35,86,36),1)
  line(d,[(83,43),(87,42)],2);line(d,[(77,48),(80,49),(84,48)],2)
 elif who=='jack':
  poly(d,[(63,22),(69,18),(80,19),(86,25),(88,35),(92,40),(88,43),(86,51),(79,55),(67,51),(62,42)],1)
  poly(d,[(67,26),(79,24),(84,29),(84,37),(88,40),(84,43),(84,49),(78,52),(70,48),(66,40)],3)
  poly(d,[(72,28),(80,27),(81,37),(85,40),(80,43),(81,48),(76,49),(72,44)],4)
  poly(d,[(63,24),(66,19),(75,17),(82,20),(86,27),(81,29),(77,24),(70,26),(65,31)],11)
  line(d,[(67,22),(75,21),(80,23)],13,2)
  line(d,[(77,33),(83,34)],1,2);rect(d,(79,36,82,36),14);rect(d,(82,36,83,37),1)
  line(d,[(79,45),(85,44)],2,2);poly(d,[(69,47),(75,50),(83,48),(81,52),(75,53),(70,50)],12)
  line(d,[(66,36),(68,34),(69,39)],2)
 else:
  poly(d,[(67,22),(73,18),(82,18),(88,22),(91,31),(91,37),(94,40),(90,42),(88,49),(81,53),(72,49),(68,40)],1)
  poly(d,[(72,26),(85,24),(88,31),(87,38),(91,40),(86,42),(85,48),(80,50),(75,46),(72,38)],3)
  poly(d,[(77,27),(85,28),(86,36),(89,39),(83,42),(83,47),(78,47),(77,40)],4)
  poly(d,[(66,28),(67,22),(72,18),(82,16),(87,19),(90,25),(88,30),(84,27),(80,25),(75,29),(72,35),(68,35)],11)
  poly(d,[(68,25),(74,20),(82,19),(85,21),(78,22),(74,26),(69,29)],13)
  line(d,[(81,33),(87,33)],1);rect(d,(83,35,86,35),14);rect(d,(86,35,87,36),1)
  line(d,[(83,45),(87,44)],2);rect(d,(72,38,73,41),15)
  poly(d,[(67,29),(63,30),(61,38),(63,46),(58,53),(61,59),(66,53),(68,43),(69,37)],11)
  line(d,[(64,34),(64,43),(62,50)],13,2);line(d,[(62,51),(62,56)],15)

def torso(d,who):
 if who=='kenji':
  poly(d,[(69,48),(78,51),(86,48),(96,56),(100,71),(97,83),(92,92),(63,91),(57,75),(60,58)],1)
  poly(d,[(66,54),(76,56),(88,52),(94,60),(96,73),(92,87),(65,87),(61,72),(63,60)],6)
  poly(d,[(65,57),(73,55),(78,65),(89,55),(93,64),(90,73),(85,81),(66,80),(63,69)],7)
  poly(d,[(72,54),(78,60),(86,53),(88,56),(78,70)],5)
  line(d,[(73,56),(80,66),(89,56)],14);line(d,[(80,68),(74,81)],5,2)
  line(d,[(64,70),(68,74),(70,72)],5);line(d,[(86,76),(90,73),(93,69)],5,2)
  poly(d,[(64,82),(94,82),(94,88),(63,88)],11);line(d,[(64,83),(93,83)],13)
  poly(d,[(80,85),(85,86),(82,100),(78,96)],11);line(d,[(83,88),(80,95)],12)
 elif who=='jack':
  poly(d,[(65,51),(78,55),(88,51),(97,58),(102,70),(96,88),(65,91),(54,72),(56,60)],1)
  poly(d,[(63,55),(74,59),(85,55),(93,60),(97,70),(93,85),(65,87),(59,72),(59,61)],6)
  poly(d,[(66,58),(73,65),(84,59),(90,62),(93,73),(88,80),(67,79),(63,70)],7)
  poly(d,[(74,57),(78,60),(83,57),(84,64),(79,68),(75,65)],3)
  line(d,[(60,60),(64,72),(65,83)],15);line(d,[(89,58),(95,70),(92,82)],15)
  poly(d,[(65,82),(92,82),(96,92),(90,105),(62,102),(58,92)],8)
  rect(d,(64,82,92,87),14);rect(d,(72,83,83,86),15)
  line(d,[(64,89),(71,94),(68,102)],9,3);line(d,[(87,90),(90,96),(86,103)],10,3)
 else:
  poly(d,[(73,49),(83,51),(90,48),(97,57),(96,72),(88,84),(66,86),(63,73),(65,58)],1)
  poly(d,[(70,53),(78,56),(86,52),(92,56),(92,66),(86,71),(67,70),(67,59)],9)
  poly(d,[(70,56),(77,59),(86,54),(89,58),(88,63),(76,65),(69,63)],10)
  line(d,[(69,55),(77,58),(88,54)],15,2);line(d,[(68,68),(88,68)],8,2)
  poly(d,[(68,72),(86,73),(85,82),(67,83)],3)
  poly(d,[(71,73),(82,74),(80,79),(71,79)],4);line(d,[(78,76),(77,79)],2)
  poly(d,[(66,82),(88,81),(92,88),(69,94),(61,90)],6)
  line(d,[(65,85),(88,84)],15,3);line(d,[(77,86),(76,96),(72,100)],8,2)

# Complete leg contours and their fold/sole marks. Coordinates are authored,
# not derived from a skeleton. Each half cycle switches near/far support.
LEG = {
 'kenji': {
  'back': ([(65,86),(79,89),(75,106),(65,122),(60,135),(52,139),(50,141),(69,141),(71,135),(75,125),(87,104),(83,91)], [(66,93),(72,94),(68,109),(59,132),(64,133),(73,114),(79,101)], [(54,137),(61,135),(65,138),(68,139)]),
  'front': ([(80,87),(92,87),(97,101),(102,116),(106,133),(118,137),(120,141),(101,141),(97,133),(90,120),(79,108),(75,96)], [(84,92),(90,91),(94,103),(96,117),(101,132),(98,134),(92,124),(85,112),(80,101)], [(102,137),(108,135),(116,139)]),
  'pass': ([(71,88),(86,88),(84,105),(90,117),(86,132),(78,135),(74,133),(76,128),(79,116),(69,108),(66,97)], [(73,94),(79,93),(77,105),(84,117),(82,125),(79,125),(82,117),(72,108)], [(77,130),(82,129),(84,132)]),
  'plant': ([(76,89),(89,90),(88,110),(85,130),(87,136),(96,139),(97,141),(75,141),(72,136),(74,119),(72,101)], [(78,95),(84,96),(82,112),(79,130),(76,131),(78,115)], [(77,136),(83,135),(92,139)])},
 'jack': {
  'back': ([(64,98),(78,101),(70,117),(65,128),(65,135),(69,141),(47,141),(48,136),(52,132),(55,119),(55,105)], [(64,104),(70,104),(64,115),(59,127),(54,127),(59,114)], [(53,128),(63,129),(62,137),(65,139),(51,139)]),
  'front': ([(79,100),(93,100),(100,110),(106,125),(109,132),(118,137),(119,141),(99,141),(97,132),(89,120),(80,116),(76,107)], [(84,104),(91,104),(96,114),(102,125),(99,128),(92,119),(83,111)], [(100,129),(106,128),(108,136),(115,139),(101,139)]),
  'pass': ([(69,100),(81,100),(85,113),(80,125),(77,134),(72,136),(68,133),(70,125),(75,117),(68,112)], [(72,104),(78,104),(79,113),(74,123),(71,124),(76,114)], [(70,127),(77,128),(76,133),(72,134)]),
  'plant': ([(76,100),(90,100),(88,118),(86,131),(90,136),(97,139),(97,141),(76,141),(73,137),(76,124),(73,111)], [(80,104),(85,104),(83,117),(81,129),(77,129),(80,117)], [(77,131),(85,131),(85,136),(93,139),(77,139)])},
 'amara': {
  'back': ([(65,88),(80,89),(75,106),(65,121),(58,133),(48,139),(47,141),(65,141),(68,135),(72,126),(84,110),(84,94)], [(69,94),(74,94),(69,108),(61,121),(55,134),(60,134),(68,118),(77,104)], [(51,138),(58,135),(62,137),(64,139)]),
  'front': ([(79,88),(91,87),(98,101),(109,115),(113,133),(124,137),(126,141),(108,141),(104,135),(99,123),(86,113),(76,102)], [(85,92),(90,92),(93,101),(103,116),(109,132),(106,133),(101,122),(88,110)], [(109,136),(115,134),(123,138)]),
  'pass': ([(69,88),(82,89),(88,103),(98,112),(96,128),(89,130),(85,127),(86,119),(79,111),(68,102)], [(73,94),(79,94),(82,103),(92,113),(90,122),(87,122),(88,115),(76,105)], [(88,125),(93,125),(92,128)]),
  'plant': ([(76,89),(90,89),(91,111),(86,131),(88,136),(97,139),(98,141),(76,141),(72,137),(75,121),(72,105)], [(80,95),(86,95),(85,109),(80,129),(77,130),(79,114)], [(78,136),(84,135),(93,139)])}}

def leg(d,who,key,far):
 contour,fold,foot=LEG[who][key];poly(d,contour,1)
 # Skin visible below the boxer's trunks; full cloth on gi and abada.
 fill=2 if far else (3 if who=='jack' else 6)
 # Inset fill is a manually authored fold mass, not a capsule fill.
 poly(d,contour,fill);line(d,contour+[contour[0]],1)
 poly(d,fold,3 if far and who=='jack' else (4 if who=='jack' else (6 if far else 7)))
 if who=='jack':
  poly(d,foot,9 if far else 10);line(d,foot+[foot[0]],8)
  line(d,[(foot[0][0]+2,foot[0][1]+3),(foot[1][0]-1,foot[0][1]+4)],14)
 else:
  poly(d,foot,3 if far else 4);line(d,foot,2)
 # Deliberate knee break / cloth fold, anchored to this specific drawing.
 k={'back':[(63,119),(66,122),(70,120)],'front':[(91,117),(97,119),(101,117)],'pass':[(79,112),(83,115),(86,113)],'plant':[(75,115),(80,117),(85,114)]}[key]
 line(d,k,2 if who=='jack' else 5)

ARMS = {
 'kenji': {
  'guard_far': [(88,56),(97,57),(104,65),(111,60),(114,48),(119,45),(126,49),(124,56),(119,69),(110,77),(100,76),(92,69)],
  'guard_near': [(62,57),(69,59),(73,70),(75,76),(83,69),(86,60),(92,57),(98,61),(98,68),(90,77),(79,86),(69,83),(62,74),(57,64)],
  'straight': [(62,57),(72,58),(85,62),(101,58),(121,54),(129,52),(135,55),(135,62),(128,66),(112,65),(91,70),(74,73),(64,69)],
  'chamber': [(62,57),(69,59),(68,69),(64,79),(76,78),(82,74),(88,76),(88,83),(81,87),(63,89),(57,83),(55,69)],
  'rise': [(62,57),(73,57),(81,48),(88,31),(88,21),(93,17),(99,20),(102,27),(96,39),(93,57),(83,69),(71,72),(62,66)]},
 'jack': {
  'guard_far': [(91,57),(98,61),(104,72),(109,70),(108,54),(106,45),(109,39),(119,38),(124,43),(124,51),(119,59),(122,74),(116,82),(106,85),(99,79),(92,69)],
  'guard_near': [(58,57),(67,59),(67,71),(64,82),(72,85),(79,73),(79,58),(78,52),(82,47),(93,47),(97,52),(95,61),(91,65),(91,81),(85,93),(75,98),(62,93),(53,81),(52,66)],
  'straight': [(58,59),(67,58),(82,63),(95,58),(114,49),(123,44),(134,46),(138,52),(137,61),(132,66),(123,66),(118,61),(101,69),(83,76),(64,75),(57,68)],
  'chamber': [(58,58),(67,60),(62,77),(70,80),(81,73),(84,68),(92,68),(98,73),(98,81),(91,86),(82,84),(73,92),(60,90),(51,82),(51,70)],
  'rise': [(58,58),(68,58),(79,50),(85,37),(83,26),(85,17),(95,15),(102,19),(104,27),(100,33),(97,34),(99,47),(90,65),(78,75),(64,73),(56,67)]},
 'amara': {
  'guard_far': [(88,55),(95,58),(99,69),(106,68),(117,60),(118,55),(122,51),(129,52),(133,56),(132,60),(126,61),(121,65),(113,75),(103,79),(93,76),(88,66)],
  'guard_near': [(65,55),(70,59),(67,70),(70,76),(79,73),(88,65),(91,59),(96,57),(101,60),(101,65),(97,69),(91,70),(83,80),(73,85),(64,82),(59,74),(60,62)],
  'straight': [(65,55),(72,59),(80,62),(97,63),(114,58),(122,52),(127,53),(132,58),(131,62),(125,64),(117,63),(101,72),(82,75),(68,72),(62,64)],
  'chamber': [(64,55),(72,57),(70,68),(66,77),(76,83),(84,83),(90,80),(95,84),(96,89),(91,92),(83,89),(76,91),(63,87),(57,80),(58,66)],
  'rise': [(65,55),(74,55),(79,45),(84,29),(85,22),(89,17),(94,17),(98,23),(97,28),(93,31),(91,46),(86,60),(75,67),(66,64)]}}

def arm(d,who,key,far):
 contour=ARMS[who][key];poly(d,contour,2 if far else 3);line(d,contour+[contour[0]],1)
 # Separate small muscle/cloth marks, no automatically shaded geometrical limb.
 if key=='guard_near':
  poly(d,[(62,61),(66,62),(63,71),(61,77),(65,85),(61,83),(58,76)],3 if far else 4)
  line(d,[(64,79),(67,82),(72,83)],2)
 elif key=='straight':
  poly(d,[(65,61),(73,62),(87,66),(101,62),(116,56),(118,59),(101,66),(87,69),(72,66)],3 if far else 4)
  line(d,[(78,70),(86,71),(98,68)],2)
 elif key=='chamber':line(d,[(59,65),(58,77),(64,84)],4,2)
 elif key=='rise':line(d,[(70,60),(79,51),(85,36)],4,2)
 else:line(d,[(94,61),(98,69),(105,74)],3,2)
 if who=='kenji':
  # Wide sleeves terminate above the skin wrist, explicit cuff folds.
  poly(d,[(61,57),(68,57),(72,67),(69,75),(62,76),(58,68)],5 if far else 7)
  line(d,[(61,71),(68,72)],5)
 if who=='jack':
  glove={'guard_far':[(109,40),(119,40),(122,44),(122,51),(116,55),(108,51)],'guard_near':[(82,49),(91,49),(95,53),(93,61),(85,63),(80,57)],'straight':[(123,46),(133,48),(136,53),(135,60),(130,64),(122,64),(118,58)],'chamber':[(86,70),(93,71),(96,75),(95,81),(90,84),(83,80)],'rise':[(87,19),(95,17),(101,21),(102,27),(97,31),(87,28)]}[key]
  poly(d,glove,5 if far else 6);line(d,glove+[glove[0]],1);line(d,[glove[0],glove[1],glove[2]],7,2)
 else:
  marks={'guard_far':[(121,50),(124,52)],'guard_near':[(90,62),(94,63)],'straight':[(127,56),(132,58)],'chamber':[(83,79),(86,81)],'rise':[(92,22),(96,23)]}[key]
  line(d,marks,4,2)


def move_image(im,dx=0,dy=0):
 out=Image.new('P',im.size);out.paste(im,(dx,dy));return out

def drawing(who,name,leg_keys=('back','front'),arm_key='guard_near',bob=0,far_arm='guard_far'):
 layers={}
 for part in LAYERS:
  im,d=ink()
  if part=='head_hair':face(d,who)
  elif part=='torso':torso(d,who)
  elif part.endswith('leg'):leg(d,who,leg_keys[0 if part=='far_leg' else 1],part=='far_leg')
  else:arm(d,who,far_arm if part=='far_arm' else arm_key,part=='far_arm')
  # Bob affects upper mass only, support soles remain on the fixed ground row.
  if bob and part in ('head_hair','torso','far_arm','near_arm'):im=move_image(im,0,bob)
  layers[part]=im
 return layers

def write_png(im,path,colors):
 im.putpalette(sum(colors,[])+[0]*(768-len(colors)*3));im.save(path,transparency=0)

def flatten(layers):
 out=Image.new('P',(160,160))
 for im in layers.values():out.paste(im,(0,0),Image.fromarray((np.asarray(im)>0).astype(np.uint8)*255,'L'))
 return out

def maps(im,path):
 a=np.asarray(im);path.write_text('\n'.join(''.join('.' if not v else format(v,'X') for v in row) for row in a)+'\n')

def main():
 for who in COLORS:
  folder=OUT/who
  if (folder/'project.json').exists():raise SystemExit('Refusing to overwrite editable study: '+str(folder))
  folder.mkdir(parents=True,exist_ok=True);(folder/'maps').mkdir(exist_ok=True);(folder/'layers').mkdir(exist_ok=True)
  colors=[list(bytes.fromhex(c)) for c in COLORS[who]]
  frames=[]
  specs=[('idle0',('back','front'),'guard_near',0,10),('idle1',('back','front'),'guard_near',1,12),('idle2',('back','front'),'guard_near',0,10),('idle3',('back','front'),'guard_near',-1,12),
         ('walk0',('back','front'),'guard_near',0,6),('walk1',('pass','plant'),'guard_near',1,4),('walk2',('front','back'),'guard_near',0,6),('walk3',('plant','pass'),'guard_near',-1,4),
         ('chamber',('back','front'),'chamber',1,4),('strike',('back','front'),'straight',0,3),('rise',('back','front'),'rise',-1,4)]
  for i,(name,legs,arms,bob,duration) in enumerate(specs):
   layers=drawing(who,name,legs,arms,bob);flat=flatten(layers)
   for layer,im in layers.items():maps(im,folder/'maps'/f'{name}__{layer}.txt');write_png(im,folder/'layers'/f'{name}__{layer}.png',colors)
   write_png(flat,folder/(name+'.png'),colors)
   coords=np.argwhere(np.asarray(flat)>0);bounds=[int(coords[:,1].min()),int(coords[:,0].min()),int(coords[:,1].max()),int(coords[:,0].max())]
   frames.append({'name':name,'duration':duration,'duration_ms':round(duration*1000/60),'bounds':bounds,'anchor':[80,141],'file':name+'.png','layers':{l:f'layers/{name}__{l}.png' for l in LAYERS}})
  project={'width':160,'height':160,'colors':colors,'layers':LAYERS,'frames':frames,'tags':[{'name':'idle','first':0,'last':3},{'name':'walk','first':4,'last':7},{'name':'punch-key-study','first':8,'last':10}]}
  (folder/'project.json').write_text(json.dumps(project,indent=2)+'\n')
  cols=4;sheet=Image.new('RGB',(160*cols,160*3),(24,32,48))
  for i,fr in enumerate(frames):
   rgba=Image.open(folder/fr['file']).convert('RGBA');sheet.paste(rgba,(i%cols*160,i//cols*160),rgba)
  sheet.save(folder/'contact-native.png');sheet.resize((1280,960),Image.Resampling.NEAREST).save(folder/'contact-2x.png')
  for tag in project['tags'][:2]:
   seq=[];dur=[]
   for fr in frames[tag['first']:tag['last']+1]:
    im=Image.new('RGB',(160,160),(24,32,48));rgba=Image.open(folder/fr['file']).convert('RGBA');im.paste(rgba,(0,0),rgba);seq.append(im.resize((320,320),Image.Resampling.NEAREST));dur.append(fr['duration_ms'])
   seq[0].save(folder/(tag['name']+'.gif'),save_all=True,append_images=seq[1:],duration=dur,loop=0)
 print('Created three native key studies; these require visual review before roster production.')
if __name__=='__main__':main()
