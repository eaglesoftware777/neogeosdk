-- Ordinary controls only. q is supplied by review_section.py from its ELF.
local m=manager.machine
local mem=m.devices[':maincpu'].spaces['program']
local screen=m.screens[':screen']
local vram=emu.item(m.devices[':spritegen'].items['0/m_videoram'])
local f=q.layout
local inputs={}
for _,port in pairs(m.ioport.ports) do
 for name,field in pairs(port.fields) do
  inputs[name]=inputs[name] or {};table.insert(inputs[name],field)
 end
end
local function input(name,on)
 for _,field in ipairs(inputs[name] or {}) do field:set_value(on and 1 or 0) end
end
local function controls(right,left,up,jump,fire)
 input('P1 Right',right);input('P1 Left',left);input('P1 Up',up)
 input('P1 A',jump);input('P1 B',fire)
end
local function u(name) return mem:read_u8(f.base+f[name]) end
local function title()
 for i=1,6 do
  if vram:read(0x7002+(9+i)*32+2)%4096~=0xD00+string.byte('CHOOSE',i) then return false end
 end
 return true
end
local file=assert(io.open(q.output..'/section.csv','w'))
file:write('seconds,tick,phase,x,y,vx,vy,hp,lives,key,climbing,hero,stage,gait,distance,scb_rows,sx,sy,pick_mask,on_ledge,cam_y,canvas_top,canvas_bottom')
if q.route=='camera' then
 file:write(',far_y,road_y')
 for i=0,8 do file:write(',ledge'..i..'_y,ledge'..i..'_rows') end
end
file:write('\n')
local started,chooser=nil,nil
local phase=q.route=='camera' and 'ground_rest' or 'approach'
local camera_tick=0
local jump=false
local short_until=nil
local last_tick=-1
local next_snap=0
local vine=q.level.vines[1]
local keyvine=q.level.vines[2]
local bough=3
local canopy_done=q.route~='canopy' and q.route~='camera'
local start_lives=nil
local last_right,last_left,last_up,last_fire=false,false,false,false
emu.register_frame_done(function()
 local now=m.time:as_double()
 local p=mem:read_u32(f.base+f.player)
 local active=p>=0x100000 and p<0x10efff and u('state')==1 and u('demo')==0
 if not started then
  input('Coin 1',now>8 and now<8.3)
  local go=now>12 and now<16 and ((now-12)%2<0.12)
  input('1 Player Start',go);input('P1 Start',go)
  local choose=title();if choose and not chooser then chooser=now end
  local picked=mem:read_u8(f.pick)==q.hero
  input('P1 Right',choose and not picked and chooser and now-chooser>0.5 and ((now-chooser-0.5)%0.5<0.08))
  input('P1 A',now>13 and now<19 and ((now-13)%0.75<0.12) and
        (not choose or (picked and chooser and now-chooser>0.45)))
  if active and now>20 then started=now;start_lives=u('lives');controls(false,false,false,false,false) end
  return
 end
 input('Coin 1',false);input('1 Player Start',false);input('P1 Start',false)
 if not active or u('lives')~=start_lives then file:flush();m:exit();return end
 local tick=mem:read_u16(f.base+f.tick)
 local x,y=mem:read_i16(p+f.x),mem:read_i16(p+f.y)
 local vx,vy=mem:read_i32(p+f.vx_fp),mem:read_i32(p+f.vy_fp)
 local right,left,up,fire=last_right,last_left,last_up,last_fire
 local footing=u('on_ledge')~=0 or y>=192
 if tick~=last_tick then
  right,left,up,fire=true,false,false,(tick%12)<4
  local landed=jump and footing and vy>=0
  if landed then jump=false end
  if short_until and tick>=short_until then jump=false;short_until=nil end
  if phase=='approach' and not canopy_done and x>=vine.x then phase='vine' end
  if phase=='vine' then
   right=x<vine.x+2;left=x>vine.x+30;up=not right and not left
   jump=false
   if y<=vine.top and u('climbing')==0 then phase='canopy' end
  elseif phase=='canopy' then
   fire=false
   local target=q.level.platforms[bough]
   local prev=q.level.platforms[bough-1]
   if footing and x>=target.x-4 and y<=target.y then
    if q.route=='camera' and bough==3 then phase='ledge_rest';camera_tick=0
    else bough=bough+1 end
   end
   if bough>q.canopy_end then canopy_done=true;phase='key' else
    target=q.level.platforms[bough];prev=q.level.platforms[bough-1]
    local edge=prev.x+math.max(1,math.floor((prev.w+16)/32))*32
    if footing and x>=edge-12 and x<target.x and not jump and not landed then
     jump=true
     local rise=prev.y-target.y
     if target.x-edge<=32 and rise>=0 and rise<=16 then short_until=tick+3 end
    end
   end
  end
  if (phase=='approach' or phase=='key' or phase=='exit') then
   if x>840 then fire=false end
   -- Full jump holds across a danger; release only once landed.
   if footing and not jump and not landed then
    for _,h in ipairs(q.level.hazards) do
     if x>=h.x-40 and x<h.x+h.w and y>=184 then jump=true;break end
    end
   end
   if x>=keyvine.x+2 and x<=keyvine.x+30 and u('has_key')==0 then
    phase='key';right=false;left=false;up=true;jump=false
   end
   if u('has_key')~=0 then phase='exit';right=true;up=false end
  end
  if q.route=='camera' and (phase:sub(1,7)=='ground_' or phase:sub(1,6)=='ledge_') then
   local prefix=phase:sub(1,7)=='ground_' and 'ground' or 'ledge'
   camera_tick=camera_tick+1
   right,left,up,fire,jump=false,false,false,false,false
   if camera_tick>320 then
    phase=prefix=='ground' and 'approach' or 'complete'
   elseif camera_tick>80 then
    local at=camera_tick-81
    phase=prefix..'_jump'..(math.floor(at/80)+1)
    jump=at%80<50
   end
  end
  last_tick=tick
 end
 if x>=1408 and u('has_key')~=0 and footing and u('climbing')==0 then
  phase='complete';right=false;left=false;up=false;jump=false;fire=false
 end
 last_right,last_left,last_up,last_fire=right,left,up,fire
 controls(right,left,up,jump,fire)
 local first=mem:read_u16(p+f.sprite_first)
 local cam=mem:read_i16(f.base+f.cam_y)
 local top=y+mem:read_i16(p+f.sprite_offset_y)-cam
 file:write(string.format('%.4f,%d,%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d',
  now,tick,phase,x,y,vx,vy,mem:read_u8(p+f.hp),u('lives'),u('has_key'),u('climbing'),
  u('hero_choice'),u('stage'),u('gait'),mem:read_u16(f.base+f.walk_distance),
  vram:read(0x8200+first)&63,mem:read_u8(p+f.scale_x),mem:read_u8(p+f.scale_y),
  mem:read_u16(f.base+f.pick_mask),u('on_ledge'),cam,top,top+64))
 if q.route=='camera' then
  local function y_at(slot)
   local y=(512-(vram:read(0x8200+slot)>>7))&511
   return y>255 and y-512 or y
  end
  file:write(','..y_at(mem:read_u16(f.far_first))..','..y_at(mem:read_u16(f.road_first)))
  local ledge=mem:read_u16(f.ledge_first)
  for i=0,8 do
   local slot=ledge+i*2
   file:write(','..y_at(slot)..','..(vram:read(0x8200+slot)&63))
  end
 end
 file:write('\n')
 if now>=next_snap then
  screen:snapshot(q.output..string.format('/t%05.1f_%s.png',now,phase));next_snap=now+1
 end
 if phase=='complete' then screen:snapshot(q.output..'/complete.png');file:close();m:exit() end
end)
