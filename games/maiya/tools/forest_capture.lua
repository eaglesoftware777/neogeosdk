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
local function controls(right,left,up,jump,fire,special)
 input('P1 Right',right);input('P1 Left',left);input('P1 Up',up)
 input('P1 A',jump);input('P1 B',fire)
 input('P1 D',special)
end
local function u(name) return mem:read_u8(f.base+f[name]) end
local function title()
 for i=1,6 do
  if vram:read(0x7002+(9+i)*32+2)%4096~=0xD00+string.byte('CHOOSE',i) then return false end
 end
 return true
end
local file=assert(io.open(q.output..'/section.csv','w'))
file:write('seconds,tick,phase,x,y,vx,vy,hp,lives,key,climbing,hero,stage,gait,distance,scb_rows,sx,sy,pick_mask,on_ledge,cam_y,canvas_top,canvas_bottom,state,state_timer,gate,boss_hp,boss_x,boss_timer,rescues,tile,facing,visible,first,art')
for y=0,3 do for x=0,4 do file:write(',r'..y..'c'..x) end end
file:write('\n')
local started,chooser=nil,nil
local phase='approach'
local jump=false
local short_until=nil
local last_tick=-1
local next_snap=0
local vine=q.level.vines[1]
local keyvine=q.level.vines[2]
local bough=3
local canopy_done=q.route~='canopy'
local start_lives=nil
local boss_started=nil
local sample=0
local art_used=false
local last_art=nil
local last_right,last_left,last_up,last_fire=false,false,false,false
emu.register_frame_done(function()
 local now=m.time:as_double()
 local p=mem:read_u32(f.base+f.player)
 local state=u('state')
 local active=p>=0x100000 and p<0x10efff and (state==1 or state==9 or state==10 or state==2) and u('demo')==0
 if not started then
  input('Coin 1',now>8 and now<8.3)
  local go=now>12 and now<16 and ((now-12)%2<0.12)
  input('1 Player Start',go);input('P1 Start',go)
  local choose=title();if choose and not chooser then chooser=now end
  local picked=mem:read_u8(f.pick)==q.hero
  input('P1 Right',choose and not picked and chooser and now-chooser>0.5 and ((now-chooser-0.5)%0.5<0.08))
  input('P1 A',now>13 and now<19 and ((now-13)%0.75<0.12) and
        (not choose or (picked and chooser and now-chooser>0.45)))
  if active and state==1 and now>20 and chooser then started=now;start_lives=u('lives');controls(false,false,false,false,false) end
  return
 end
 input('Coin 1',false);input('1 Player Start',false);input('P1 Start',false)
 if not active or u('lives')<start_lives then file:flush();m:exit();return end
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
   if footing and x>=target.x-4 and y<=target.y then bough=bough+1 end
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
   if x>840 and x<1408 then fire=false end
   for _,h in ipairs(q.level.hazards) do
    if x>=h.x-120 and x<h.x+h.w+30 then fire=false end
   end
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
  last_tick=tick
 end
 if state==10 then
  phase='door';right=false;left=false;up=false;jump=false;fire=false
 elseif state==9 then
  phase='guardian_intro';right=false;left=false;up=false;jump=false;fire=false
 elseif state==2 then
  phase='complete';right=false;left=false;up=false;jump=false;fire=false
 elseif u('boss_active')~=0 then
  phase='guardian';boss_started=boss_started or tick
  local boss=mem:read_u32(f.base+f.boss)
  local bx=mem:read_i16(boss+f.x)
  right=false;left=false;up=false;fire=true
  if u('facing')~=(bx<x and 1 or 0) then right=bx>x;left=bx<x end
  local bt=mem:read_u16(f.base+f.boss_timer)%200
  if footing and not jump and bt>=40 and bt<65 and math.abs(bx-x)<150 then jump=true end
  if footing and jump and mem:read_i32(p+f.vy_fp)>=0 and bt>100 then jump=false end
 elseif x>=q.level.gate_x-40 and u('has_key')~=0 then
  phase='gate';up=tick%12<6;fire=false;jump=false
 end
 last_right,last_left,last_up,last_fire=right,left,up,fire
 local stock=u('art')
 if last_art and stock<last_art then art_used=true end
 last_art=stock
 local special=q.use_art and not art_used and state==1 and x>=2220 and stock>0 and tick%12<6
 controls(right,left,up,jump,fire,special)
 local first=mem:read_u16(p+f.sprite_first)
 local cam=mem:read_i16(f.base+f.cam_y)
 local top=y+mem:read_i16(p+f.sprite_offset_y)-cam
 file:write(string.format('%.4f,%d,%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d',
  now,tick,phase,x,y,vx,vy,mem:read_u8(p+f.hp),u('lives'),u('has_key'),u('climbing'),
  u('hero_choice'),u('stage'),u('gait'),mem:read_u16(f.base+f.walk_distance),
  vram:read(0x8200+first)&63,mem:read_u8(p+f.scale_x),mem:read_u8(p+f.scale_y),
  mem:read_u16(f.base+f.pick_mask),u('on_ledge'),cam,top,top+64))
 local boss=mem:read_u32(f.base+f.boss)
 local boss_valid=boss>=0x100000 and boss<0x10efff
 file:write(string.format(',%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d',state,
  mem:read_u16(f.base+f.state_timer),u('gate_unlocked'),boss_valid and mem:read_u8(boss+f.hp) or -1,
  boss_valid and mem:read_i16(boss+f.x) or -1,mem:read_u16(f.base+f.boss_timer),u('rescue_mask'),
  mem:read_u16(p+f.sprite_tile),u('facing'),mem:read_u8(p+f.visible),first,stock))
 for row=0,3 do for col=0,4 do file:write(','..vram:read((first+col)*64+row*2)) end end
 file:write('\n')
 sample=sample+1
 if q.frames then screen:snapshot(q.output..string.format('/frames/%05d.png',sample)) end
 if now>=next_snap then
  screen:snapshot(q.output..string.format('/t%05.1f_%s.png',now,phase));next_snap=now+1
 end
 if phase=='complete' then screen:snapshot(q.output..'/complete.png');file:close();m:exit() end
end)
