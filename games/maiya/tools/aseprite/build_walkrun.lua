-- Native indexed authoring, adapted from create_studio_aseprite.lua's setup.
-- Exact text-map pixels only; no sprite transforms or colour conversion.
local root=assert(app.params.root, 'root parameter required')
local function read(name)
  local f=assert(io.open(root..'/'..name,'r'));local s=f:read('*a');f:close();return s
end
local palette=json.decode(read('palette.json'))
local poses=json.decode(read('poses.json'))
local names={'ref','far_arm','far_leg','near_leg','torso','head_hair','near_arm'}
local spr=Sprite(80,64,ColorMode.INDEXED)
if spr.layers[1].isBackground then
  app.layer=spr.layers[1];app.command.LayerFromBackground()
end
spr.transparentColor=0
for i=0,15 do
  local c=palette.colors[i+1]
  spr.palettes[1]:setColor(i,Color{r=c[1],g=c[2],b=c[3],a=i==0 and 0 or 255})
end
local layers={}
for i,name in ipairs(names) do
  local l=i==1 and spr.layers[1] or spr:newLayer();l.name=name;l.stackIndex=i;layers[name]=l
end
local function pixels(name)
  local im=Image(80,64,ColorMode.INDEXED);local y=0
  for row in read('maps/'..name..'.txt'):gmatch('[^\r\n]+') do
    assert(#row==80 and y<64,'bad pixel-map dimensions')
    for x=0,79 do
      local c=row:sub(x+1,x+1);local n=c=='.' and 0 or tonumber(c,16)
      assert(n and n<=15,'off-palette pixel');im:putPixel(x,y,n)
    end
    y=y+1
  end
  assert(y==64,'pixel map must have 64 rows');return im
end
for i,pose in ipairs(poses) do
  if i>1 then spr:newFrame() end
  for j=2,#names do
    local name=names[j];spr:newCel(layers[name],i,pixels(pose.name..'__'..name),Point(0,0))
  end
  local ref=pixels(pose.mode=='stop' and 'reference_idle0' or 'reference_stride')
  for x=1,78 do ref:putPixel(x,61,14);ref:putPixel(x,10,14) end
  spr:newCel(layers.ref,i,ref,Point(0,0))
  -- a walk covers 64 px of road per 80 clock units, a run 80
  local speed=pose.mode=='run' and 800/256 or 512/256*80/64
  spr.frames[i].duration=pose.mode=='stop' and 4/60 or pose.hold_distance/speed/60
end
layers.ref.isVisible=false;layers.ref.isEditable=false
local first,last,order={},{},{}
for i,pose in ipairs(poses) do
  if not first[pose.mode] then first[pose.mode]=i;order[#order+1]=pose.mode end
  last[pose.mode]=i
end
for _,mode in ipairs(order) do
  local t=spr:newTag(first[mode],last[mode]);t.name=mode;t.aniDir=AniDir.FORWARD
end
spr:saveAs(root..'/maiya_walkrun.aseprite');spr:close()
print('Saved indexed 80x64 source: walk '..(last.walk-first.walk+1)..', run '..(last.run-first.run+1)..', stop 1')
