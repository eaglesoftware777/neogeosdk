local root=assert(app.params.root);local P=dofile(assert(app.params.scripts)..'/pixel_tools.lua')
local pal={{0,0,0},{15,23,33},{37,45,56},{63,78,90},{94,113,125},{142,159,168},{212,222,218},{242,241,218},{112,33,37},{204,58,49},{249,149,60},{20,58,74},{70,167,188},{56,83,61},{121,153,83},{251,218,122}}
local dot,line,rect,poly,panel=P.dot,P.line,P.rect,P.poly,P.panel
local assets={};local defs={}
local function warden(im,phase)
 -- Two full engine nacelles, transverse armored bridge and an intact tail.
 panel(im,9,16,15,34,3);panel(im,56,16,15,34,3);panel(im,22,27,37,13,13)
 poly(im,{{40,9},{47,19},{49,42},{45,51},{35,51},{31,42},{33,20}},1)
 poly(im,{{40,12},{44,20},{46,41},{43,48},{37,48},{34,41},{36,20}},4)
 panel(im,36,18,8,12,11);line(im,37,19,42,19,12);line(im,37,31,37,44,6)
 for _,x in ipairs({16,63})do
  if phase==1 then line(im,x-10,6,x+9,26,5);line(im,x+10,6,x-9,26,3)
  elseif phase==2 then line(im,x,3,x,30,5);line(im,x-13,16,x+13,16,3)
  else line(im,x-13,16,x+13,16,5);line(im,x,3,x,30,3)end
  panel(im,x-2,14,5,5,6);dot(im,x,16,1);panel(im,x-5,34,11,10,2);line(im,x-4,49,x+4,49,10)
 end
 for _,x in ipairs({25,49})do panel(im,x,39,5,17,3);line(im,x+1,40,x+1,53,6);dot(im,x+2,54,10)end
 panel(im,36,44,8,9,8);line(im,37,45,42,45,9);dot(im,40,47,15)
end
local function keep(im,phase)
 -- Airborne fortress with a complete central hull and four articulated bays.
 poly(im,{{56,6},{72,16},{76,27},{91,35},{91,87},{77,101},{35,101},{21,87},{21,35},{36,27},{40,16}},1)
 poly(im,{{56,9},{69,18},{73,30},{87,37},{87,84},{74,97},{38,97},{25,84},{25,37},{39,30},{43,18}},3)
 panel(im,40,27,32,59,4);poly(im,{{56,14},{66,25},{66,37},{46,37},{46,25}},8);line(im,49,25,60,20,9)
 panel(im,47,41,18,17,11);line(im,49,43,62,43,12);line(im,49,46,60,46,12)
 for _,x in ipairs({9,78})do
  poly(im,{{x+8,17},{x+22,24},{x+24,44},{x+24,87},{x+18,103},{x+5,101},{x,86},{x,38}},1)
  poly(im,{{x+9,20},{x+19,26},{x+21,43},{x+21,84},{x+16,99},{x+7,97},{x+3,84},{x+3,39}},8)
  line(im,x+5,34,x+5,84,9);line(im,x+4,33,x+18,29,6)
  for y=42,86,11 do panel(im,x+6,y,13,6,3);line(im,x+7,y+2,x+17,y+2,5)end
  line(im,x+7,98,x+16,98,phase==1 and 15 or 10)
 end
 for _,x in ipairs({32,73})do
  panel(im,x-2,59,11,15,4);panel(im,x,70,7,22,2);line(im,x+1,71,x+1,88,6);dot(im,x+3,89,phase==2 and 15 or 10)
 end
 panel(im,43,72,26,16,8);panel(im,48,76,16,8,10);line(im,50,77,61,77,15)
 panel(im,45,88,22,8,2);for x=48,61,3 do line(im,x,89,x,93,4)end
end
local function turret(im,phase)
 poly(im,{{8,8},{23,8},{28,14},{26,23},{22,27},{10,27},{5,22},{4,14}},1)
 poly(im,{{9,10},{22,10},{25,15},{23,22},{20,24},{11,24},{7,20},{7,15}},4)
 panel(im,12,14,9,8,8);panel(im,14+phase,3,4,16,3);line(im,15+phase,4,15+phase,15,6);dot(im,17,18,15)
end
local function blast(im,phase)
 local r=({4,7,11,14,12,8})[phase+1];local cx,cy=16,16
 poly(im,{{cx,cy-r},{cx+3,cy-4},{cx+r,cy-2},{cx+5,cy+3},{cx+2,cy+r},{cx-4,cy+5},{cx-r,cy+2},{cx-5,cy-4}},8)
 poly(im,{{cx,cy-r+2},{cx+2,cy-2},{cx+r-3,cy-1},{cx+3,cy+2},{cx+1,cy+r-3},{cx-2,cy+3},{cx-r+3,cy+1},{cx-3,cy-2}},10)
 if phase<4 then poly(im,{{cx,cy-r+4},{cx+r-5,cy},{cx,cy+r-4},{cx-r+5,cy}},15)end
end
local function cloud(im,phase)
 local outer={{4,25},{9,25},{9,19},{16,19},{16,13},{27,13},{27,8},{35,8},{35,5},{47,5},{47,8},{56,8},{56,14},{66,14},{66,18},{75,18},{75,22},{84,22},{84,27},{91,27},{91,34},{86,34},{86,39},{74,39},{74,43},{61,43},{61,45},{41,45},{41,42},{26,42},{26,38},{14,38},{14,33},{4,33}}
 poly(im,outer,2)
 for _,v in ipairs({{12,22,24,12},{28,11,27,20},{49,21,31,13},{30,28,34,12}})do
  local x,y,w,h=table.unpack(v)
  poly(im,{{x+4,y},{x+w-6,y},{x+w-6,y+3},{x+w,y+3},{x+w,y+h-4},{x+w-5,y+h-4},{x+w-5,y+h},{x+4,y+h},{x+4,y+h-3},{x,y+h-3},{x,y+4},{x+4,y+4}},4)
  poly(im,{{x+5,y+2},{x+w-8,y+2},{x+w-8,y+5},{x+w-3,y+5},{x+w-5,y+h-5},{x+5,y+h-4},{x+3,y+5}},5)
  line(im,x+6,y+3,x+w-10,y+3,6)
 end
 line(im,32,39,45,39,3);line(im,64,34,76,34,3)
 -- Open holes between lobes keep bullets and the player's hitbox readable.
end
local function author(name,w,h,bank,draw,kind,append,banks,unit)
 local count=kind=='effect' and 6 or (kind=='cloud' or kind=='title') and 1 or 8
 local document_pal=pal
 if unit then document_pal={};for _,c in ipairs(unit.palette)do document_pal[#document_pal+1]={c[1],c[2],c[3]}end end
 if kind=='cloud' and name:find('cloud')then document_pal={{0,0,0},{76,94,112},{104,126,138},{140,160,166},{169,190,189},{203,214,207},{232,235,217},{255,248,226},{112,33,37},{204,58,49},{249,149,60},{20,58,74},{70,167,188},{56,83,61},{121,153,83},{251,218,122}}end
 if kind=='title'then
  local f=assert(io.open(root..'/aircraft_models.json','r'));local models=json.decode(f:read('*a'));f:close()
  document_pal={};for _,c in ipairs(models[1].palette)do document_pal[#document_pal+1]={c[1],c[2],c[3]}end
 end
 local s=P.create(w,h,document_pal);local layer=s.layers[1];layer.name='complete_hull';local shadow=s:newLayer();shadow.name='shadow';shadow.isVisible=false
 for i=0,15 do s.palettes[1]:setColor(16+i,Color{r=i==0 and 0 or 20+(i-1)*15,g=i==0 and 0 or 20+(i-1)*15,b=i==0 and 0 or 20+(i-1)*15,a=i==0 and 0 or 255})end
 for f=1,count do
  if f>1 then s:newFrame()end
  local im=Image(w,h,ColorMode.INDEXED);local phase=f-1
  if unit then
   -- tools/unit_frames.py builds every frame from the unit's base drawing
   for y=0,h-1 do local row=unit.frames[f][y+1];for x=0,w-1 do local v=tonumber(row:sub(x+1,x+1),16);if v~=0 then im:putPixel(x,y,v)end end end
  elseif kind=='effect' then draw(im,phase)
  elseif kind=='cloud' or kind=='title' then draw(im,0)
  elseif phase<4 then draw(im,phase%3);if phase==3 then panel(im,math.floor(w/2)-3,math.floor(h/2),7,6,2);line(im,math.floor(w/2)-2,math.floor(h/2)+1,math.floor(w/2)+1,math.floor(h/2)+2,10)end
  elseif phase<6 then draw(im,0);local cx,cy=math.floor(w/2),math.floor(h/2);local r=phase==4 and 6 or 12;poly(im,{{cx,cy-r},{cx+r,cy},{cx,cy+r},{cx-r,cy}},10);poly(im,{{cx,cy-r+2},{cx+r-2,cy},{cx,cy+r-2},{cx-r+2,cy}},15)
  else
   local cx,cy=math.floor(w/2),math.floor(h/2);local r=math.min(cx-4,cy-4);poly(im,{{cx,cy-r},{cx+5,cy-5},{cx+r,cy},{cx+5,cy+5},{cx,cy+r},{cx-5,cy+5},{cx-r,cy},{cx-5,cy-5}},10)
   poly(im,{{cx,cy-r+3},{cx+r-3,cy},{cx,cy+r-3},{cx-r+3,cy}},phase==6 and 15 or 8)
   panel(im,4,math.floor(h/2)-5,6,8,3);panel(im,w-11,math.floor(h/2)+3,7,6,3)
  end
  s:newCel(layer,f,im,Point(0,0));local sh=Image(w,h,ColorMode.INDEXED)
  for y=0,h-1 do for x=0,w-1 do if im:getPixel(x,y)~=0 then sh:putPixel(x,y,18)end end end;s:newCel(shadow,f,sh,Point(0,0));s.frames[f].duration=phase<4 and .12 or .08
  local label=f==1 and name or name..'_'..phase
  assets[#assets+1]={name=label,png='frames/'..label..'.png',bank=bank,banks=banks or 1,pivot={math.floor(w/2),math.floor(h/2)},append=(f~=1 or append==true),source=name..'.aseprite',frame=f,kind=kind}
 end
 local t=s:newTag(1,math.min(count,kind=='effect' and 6 or 3));t.name=kind=='effect' and 'impact' or 'idle'
 if count==8 then t=s:newTag(4,4);t.name='damage';t=s:newTag(5,8);t.name='destruction' end
 s:saveAs(root..'/'..name..'.aseprite');s:close();defs[#defs+1]={name=name,width=w,height=h,frames=count,palette=document_pal,kind=kind}
end
-- Stage 1 units: each has its own palette and the strip unit_frames.py built.
for _,name in ipairs({'drone','fighter','heli','tank','bomber','sam','radar_site'})do
 local f=assert(io.open(root..'/units/'..name..'.json','r'));local u=json.decode(f:read('*a'));f:close()
 author(name,u.width,u.height,u.bank,nil,'enemy',false,u.banks,u)
end
-- Existing ID 167 now contains an original complete twin-rotor miniboss.
-- The drone's bank holds the drone's own colours now, so it moves next door.
author('miniboss_0',80,64,76,warden,'miniboss',false,1)
author('red_fortress',112,112,56,keep,'boss',false,16)
author('native_keep_turret',32,32,56,turret,'module',true,16)
author('native_far_cloud',96,48,100,cloud,'cloud',true)
author('native_overhead_cloud',96,48,100,cloud,'cloud',true)
for i=0,5 do author('explosion_'..i,32,32,99,function(im)blast(im,i)end,'cloud',false)end
author('title',320,224,16,function(im)
 local f=assert(io.open(root..'/title-native_indices.bin','rb'));for y=0,223 do local row=assert(f:read(320));for x=0,319 do im:putPixel(x,y,row:byte(x+1))end end;f:close()
end,'title',false,16)
-- Shadow files use the existing smoke ramp, so live effects cannot recolour them.
local f=assert(io.open(root..'/production_assets.json','w'));f:write(json.encode(assets));f:close()
f=assert(io.open(root..'/production_models.json','w'));f:write(json.encode(defs));f:close()
print('Authored native Stage 1 enemy, miniboss, complete fortress, module and atmosphere sources')
