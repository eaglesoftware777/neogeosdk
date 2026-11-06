-- Native original aircraft. All contours and material clusters are specified
-- here in whole pixels and written directly to indexed Aseprite Images.
-- No imported painting, rotate, scale, interpolation or antialiased tool.
local root=assert(app.params.root)
local layer_names={'guides','shadow','far_wing','main_hull','near_wing','mechanical_details','weapons','engine','damage','effects'}
local common={{0,0,0},{12,20,32},{32,48,65},{57,77,97},{89,113,130},{138,158,170},{195,207,211},{227,235,227},
 {28,77,146},{48,142,203},{130,211,239},{16,40,65},{56,130,162},{247,150,52},{251,235,178},{73,48,44}}
local accents={{{28,77,146},{48,142,203},{130,211,239}},{{28,93,61},{61,174,109},{154,223,174}},{{146,57,32},{223,101,48},{247,178,97}}}
local types={'f16','f35','f15'}
local function dot(im,x,y,c) assert(x>=0 and x<48 and y>=0 and y<48,'clipped authored pixel');im:putPixel(x,y,c) end
local function line(im,x0,y0,x1,y1,c)
 local dx=math.abs(x1-x0);local sx=x0<x1 and 1 or -1;local dy=-math.abs(y1-y0);local sy=y0<y1 and 1 or -1;local err=dx+dy
 while true do dot(im,x0,y0,c);if x0==x1 and y0==y1 then break end;local e=2*err;if e>=dy then err=err+dy;x0=x0+sx end;if e<=dx then err=err+dx;y0=y0+sy end end
end
local function rect(im,x0,y0,x1,y1,c) for y=y0,y1 do for x=x0,x1 do dot(im,x,y,c) end end end
local function poly(im,p,c)
 local lo,hi=47,0;for _,v in ipairs(p) do lo=math.min(lo,v[2]);hi=math.max(hi,v[2]) end
 for y=lo,hi do
  local xs={}
  for i,a in ipairs(p) do local b=p[i%#p+1];if (a[2]<=y and b[2]>y) or (b[2]<=y and a[2]>y) then xs[#xs+1]=a[1]+(y-a[2])*(b[1]-a[1])/(b[2]-a[2]) end end
  table.sort(xs);for i=1,#xs-1,2 do for x=math.ceil(xs[i]),math.floor(xs[i+1]) do dot(im,x,y,c) end end
 end
 for i,a in ipairs(p) do local b=p[i%#p+1];line(im,a[1],a[2],b[1],b[2],c) end
end
local function wing(im,left,model,bank)
 local strength=math.abs(bank);local span=({12,13,14})[model]-({0,1,2,4})[strength+1]
 local sign=left and -1 or 1;local raised=(sign*bank<0);local dz=bank==0 and 0 or (raised and -strength or strength)
 local rootx=24+sign*(model==3 and 3 or 2);local tip=24+sign*span
 local front=model==2 and 17 or model==3 and 18 or 20;local tipy=model==2 and 27 or 31
 poly(im,{{rootx,front},{tip,tipy+dz},{tip+sign,tipy+2+dz},{rootx+sign,31},{rootx,28}},1)
 poly(im,{{rootx,front+1},{tip,tipy+dz},{tip,tipy+1+dz},{rootx+sign,29}},raised and 5 or 3)
 line(im,rootx,front+2,tip,tipy+dz,raised and 7 or 5)
 line(im,rootx+sign,26,tip-sign*2,tipy+1+dz,2)
 line(im,rootx+sign,27,tip-sign*3,tipy+1+dz,4)
 -- Paint is confined to a deliberate panel, never a noisy colour fringe.
 line(im,tip-sign*3,tipy+dz,tip-sign,tipy+dz,9)
 line(im,tip-sign*2,tipy+dz,tip-sign,tipy+dz,10)
 return tip,tipy+dz
end
local function hull(im,model,bank)
 if model==1 then
  poly(im,{{24,5},{26,10},{27,18},{27,25},{29,33},{28,39},{26,42},{22,42},{20,39},{20,34},{21,27},{21,17},{22,11}},1)
  poly(im,{{24,7},{25,11},{26,19},{26,28},{27,35},{26,40},{22,40},{21,35},{22,27},{22,17},{23,11}},4)
 elseif model==2 then
  poly(im,{{24,3},{27,9},{29,14},{29,28},{31,35},{28,43},{21,43},{18,35},{20,29},{19,14},{22,8}},1)
  poly(im,{{24,5},{26,10},{28,15},{28,28},{29,35},{27,41},{22,41},{20,35},{21,28},{21,15},{23,10}},4)
 else
  poly(im,{{24,3},{26,9},{27,18},{30,25},{30,42},{28,46},{25,45},{24,41},{23,45},{20,46},{18,42},{18,25},{21,18},{22,10}},1)
  poly(im,{{24,5},{25,10},{26,20},{29,26},{29,41},{27,44},{25,40},{23,40},{21,44},{19,41},{19,26},{22,20},{23,10}},4)
  rect(im,19,25,21,42,3);rect(im,27,25,29,42,3)
  line(im,20,26,20,41,6);line(im,28,26,28,41,5)
 end
 -- Fixed fuselage centreline and upper-left light; bank changes side faces.
 line(im,24,model==1 and 7 or 5,24,model==3 and 36 or 34,6)
 line(im,23,22,23,33,5);line(im,26,23,26,34,bank>0 and 2 or 3)
 local tx=model==3 and 13 or model==2 and 14 or 16
 poly(im,{{22,34},{tx,39},{tx,42},{22,39}},1);poly(im,{{22,35},{tx+1,39},{tx+1,40},{22,38}},4);line(im,22,35,tx+1,39,6)
 poly(im,{{26,34},{48-tx,39},{48-tx,42},{26,39}},1);poly(im,{{26,35},{47-tx,39},{47-tx,40},{26,38}},3);line(im,26,35,47-tx,39,5)
end
local function details(im,model,bank)
 local cx=24+(bank>1 and 1 or bank< -1 and -1 or 0)
 local top=model==1 and 11 or 9;local bottom=model==2 and 21 or 20
 poly(im,{{cx,top},{cx+2,top+4},{cx+1,bottom},{cx-2,bottom},{cx-2,top+4}},1)
 poly(im,{{cx,top+1},{cx+1,top+4},{cx,bottom-1},{cx-1,bottom-1},{cx-1,top+4}},11)
 line(im,cx,top+3,cx,bottom-2,12);dot(im,cx,top+2,7);dot(im,cx-1,top+4,10)
 if model==1 then
  rect(im,21,25,22,29,2);line(im,21,25,21,28,7);rect(im,26,25,27,29,2)
  poly(im,{{24,31},{26,37},{25,41},{23,39}},2);line(im,24,32,24,39,8);dot(im,24,34,10)
 else
  for _,x in ipairs({19,28}) do
   rect(im,x,22,x+1,27,1);line(im,x,22,x,25,5)
   poly(im,{{x+1,32},{x+3,37},{x+2,41},{x,38}},2);line(im,x+1,33,x+2,38,8);dot(im,x+1,34,10)
  end
 end
 -- Native panel seams: one deliberate dark line beside one lit metal edge.
 line(im,22,30,25,30,3);dot(im,23,30,7);line(im,22,33,25,33,2)
 for _,y in ipairs({34,36,38}) do dot(im,23,y,5);dot(im,25,y,2) end
end
local function weapon(im,tip,y,left)
 local x=tip+(left and 2 or -2)
 rect(im,x,y-1,x,y+3,1);line(im,x,y,x,y+2,6);dot(im,x,y-1,7);dot(im,x,y+2,8)
 -- A hardpoint connects each missile to the wing.
 dot(im,x+(left and 1 or -1),y+1,2)
end
local function engine(im,model,boost)
 local locations=model==3 and {{20,44},{28,44}} or {{24,model==2 and 42 or 41}}
 for _,p in ipairs(locations) do
  rect(im,p[1]-1,p[2]-2,p[1]+1,p[2],2);line(im,p[1]-1,p[2],p[1]+1,p[2],13);dot(im,p[1],p[2],14)
  if boost then
   local endy=math.min(46,p[2]+(boost==1 and 2 or 3))
   line(im,p[1],p[2]+1,p[1],endy,13);dot(im,p[1],p[2]+1,14)
   if boost==2 then dot(im,p[1],endy,14) end
  end
 end
end
local models={}
for model,name in ipairs(types) do
 local spr=Sprite(48,48,ColorMode.INDEXED);spr.transparentColor=0
 if spr.layers[1].isBackground then app.layer=spr.layers[1];app.command.LayerFromBackground() end
 local colors={};for i,c in ipairs(common) do colors[i]={c[1],c[2],c[3]} end
 for j=1,3 do colors[8+j]=accents[model][j] end
 -- Two document banks: main material slots 1..15 and shadow slots 17..31.
 for i=0,31 do
  local c=i<16 and colors[i+1] or {20+(i-17)*15,20+(i-17)*15,20+(i-17)*15}
  if i==16 then c={0,0,0} end
  spr.palettes[1]:setColor(i,Color{r=c[1],g=c[2],b=c[3],a=(i==0 or i==16) and 0 or 255})
 end
 local layers={};for i,n in ipairs(layer_names) do local l=i==1 and spr.layers[1] or spr:newLayer();l.name=n;l.stackIndex=i;layers[n]=l end
 local frames={};local function add(tag,bank,state,phase) frames[#frames+1]={tag=tag,bank=bank,state=state,phase=phase or 0} end
 for bank=-3,3 do add('bank',bank,'intact') end
 for _,bank in ipairs({2,1,0}) do add('recovery',bank,'intact') end
 for phase=1,2 do add('boost',0,'boost',phase) end
 for bank=-3,3 do add('damage',bank,'damage') end
 for phase=0,3 do add('destruction',0,'destruction',phase) end
 for _,bank in ipairs({-3,-2,2,3}) do add('aerobatics',bank,'aerobatics') end
 local metadata={name=name,canvas={48,48},pivot={24,24},nose={24,model==1 and 5 or 3},engine=model==3 and {{20,44},{28,44}} or {{24,model==2 and 42 or 41}},gun_origins={{24,model==1 and 5 or 3}},shadow_origin={24,24},palette=colors,frames={}}
 for i,f in ipairs(frames) do
  if i>1 then spr:newFrame() end
  local ims={};for _,n in ipairs(layer_names) do ims[n]=Image(48,48,ColorMode.INDEXED) end
  local leftfar=f.bank>=0
  local lt,ly=wing(ims[leftfar and 'far_wing' or 'near_wing'],true,model,f.bank)
  local rt,ry=wing(ims[leftfar and 'near_wing' or 'far_wing'],false,model,f.bank)
  hull(ims.main_hull,model,f.bank);details(ims.mechanical_details,model,f.bank)
  weapon(ims.weapons,lt,ly,true);weapon(ims.weapons,rt,ry,false)
  engine(ims.engine,model,f.state=='boost' and f.phase or false)
  if f.state=='damage' then
   poly(ims.damage,{{22,27},{25,28},{25,30},{23,31},{21,29}},15)
   line(ims.damage,23,27,24,28,13);dot(ims.damage,22,28,2)
   rect(ims.damage,lt+2,ly,lt+3,ly+1,15)
  end
  if f.state=='aerobatics' then
   -- Individually drawn underside panels and foreshortened canopy, no rotation.
   rect(ims.damage,22,22,26,26,2);line(ims.damage,23,23,25,23,5);line(ims.damage,23,25,25,25,3)
  end
  if f.state=='destruction' then
   local r=({3,7,12,18})[f.phase+1]
   poly(ims.effects,{{24,24-r},{27,20},{24+r,24},{28,28},{24,24+r},{20,28},{24-r,24},{20,20}},13)
   poly(ims.effects,{{24,24-r+2},{26,22},{24+r-2,24},{26,26},{24,24+r-2},{22,26},{24-r+2,24},{22,22}},14)
   if f.phase>=2 then
    -- Named breakup state: a flash masks the hull before its pieces depart.
    for _,n in ipairs({'far_wing','near_wing','main_hull','mechanical_details','weapons','engine'}) do ims[n]:clear() end
    poly(ims.damage,{{7,20},{13,17},{18,21},{16,26},{9,25}},3);line(ims.damage,9,20,14,19,6)
    poly(ims.damage,{{31,30},{38,28},{42,32},{39,37},{33,36}},3);line(ims.damage,33,30,38,30,5)
   end
  end
  local visible=Image(48,48,ColorMode.INDEXED)
  for j=3,#layer_names do visible:drawImage(ims[layer_names[j]]) end
  for y=0,47 do for x=0,47 do if visible:getPixel(x,y)~=0 then ims.shadow:putPixel(x,y,18) end end end
  line(ims.guides,24,1,24,46,7);line(ims.guides,1,24,46,24,7)
  for _,n in ipairs(layer_names) do spr:newCel(layers[n],i,ims[n],Point(0,0)) end
  spr.frames[i].duration=f.state=='destruction' and 0.07 or 0.10
  local label=f.tag..'_'..string.format('%02d',i)
  metadata.frames[#metadata.frames+1]={name=label,tag=f.tag,bank=f.bank,angle=f.bank*15,state=f.state,pivot={24,24},hardpoints={{lt+2,ly},{rt-2,ry}},duration_ms=f.state=='destruction' and 70 or 100}
 end
 layers.guides.isVisible=false;layers.guides.isEditable=false;layers.shadow.isVisible=false
 for _,t in ipairs({{'bank',1,7},{'recovery',8,10},{'boost',11,12},{'damage',13,19},{'destruction',20,23},{'aerobatics',24,27}}) do local tag=spr:newTag(t[2],t[3]);tag.name=t[1] end
 spr:saveAs(root..'/aircraft_'..name..'.aseprite');spr:close();models[#models+1]=metadata
end
local out=assert(io.open(root..'/aircraft_models.json','w'));out:write(json.encode(models));out:close()
print('Saved three native indexed aircraft model/animation sources')
