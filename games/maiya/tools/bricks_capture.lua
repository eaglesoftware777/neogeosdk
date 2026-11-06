-- Read-only observation of ordinary inputs: road, first bricks, road again.
local m=manager.machine
local mem=m.devices[':maincpu'].spaces['program']
local screen=m.screens[':screen']
local vram=emu.item(m.devices[':spritegen'].items['0/m_videoram'])
local f=q.layout
local function input(name,on)
 for _,port in pairs(m.ioport.ports) do
  if port.fields[name] then port.fields[name]:set_value(on and 1 or 0) end
 end
end
local function u(name) return mem:read_u8(f.base+f[name]) end
local function title()
 for i=1,6 do
  if vram:read(0x7002+(9+i)*32+2)%4096~=0xD00+string.byte('CHOOSE',i) then return false end
 end
 return true
end
local function hardware_y(slot)
 local y=(512-(vram:read(0x8200+slot)>>7))&511
 return (y>255 and y-512 or y)-16
end
local file=assert(io.open(q.output..'/section.csv','w'))
file:write('seconds,tick,phase,x,y,vx,vy,on_ledge,cam_y,scb_rows,far_y,road_y,brick_y,sprite_feet,canvas_top,canvas_bottom,hp,lives,hero,stage\n')
local started,chooser=nil,nil
local phase='approach'
local last_tick=-1
local phase_tick=0
local right,jump,fire=false,false,false
local next_snap=0
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
  if active and now>20 and chooser then started=true;input('P1 A',false);input('P1 Right',false) end
  return
 end
 input('Coin 1',false);input('1 Player Start',false);input('P1 Start',false)
 if not active then file:flush();m:exit();return end
 local tick=mem:read_u16(f.base+f.tick)
 local x,y=mem:read_i16(p+f.x),mem:read_i16(p+f.y)
 local vx,vy=mem:read_i32(p+f.vx_fp),mem:read_i32(p+f.vy_fp)
 if tick~=last_tick then
  last_tick=tick;phase_tick=phase_tick+1
  right,jump,fire=false,false,false
  if phase=='approach' then
   right=true;fire=(tick%12)<4
   if x>=408 then phase='onto_bricks';phase_tick=0;fire=false;jump=true end
  elseif phase=='onto_bricks' then
   right=x<464;jump=true
   if y==144 and vy==0 and u('on_ledge')~=0 then phase='brick_rest';phase_tick=0;jump=false;right=false end
  elseif phase=='brick_rest' then
   if phase_tick>80 then phase='brick_jump';phase_tick=0;jump=true end
  elseif phase=='brick_jump' then
   jump=phase_tick<50
   if phase_tick>80 then phase='off_bricks';phase_tick=0;jump=false;right=true end
  elseif phase=='off_bricks' then
   right=true;fire=y>160
   if y==192 and vy==0 and u('on_ledge')==0 then phase='ground_rest';phase_tick=0;right=false end
  elseif phase=='ground_rest' then
   fire=true
   if phase_tick>80 then phase='complete';fire=false end
  end
 end
 input('P1 Right',right);input('P1 Left',false);input('P1 Up',false)
 input('P1 A',jump);input('P1 B',fire)
 local cam=mem:read_i16(f.base+f.cam_y)
 local first=mem:read_u16(p+f.sprite_first)
 local offset=mem:read_i16(p+f.sprite_offset_y)
 local brick=-999
 local ledge=mem:read_u16(f.ledge_first)
 for i=0,8 do
  if mem:read_u16(f.ledge_owner+i*2)==32 and (vram:read(0x8200+ledge+i*2)&63)>0 then
   brick=hardware_y(ledge+i*2)
  end
 end
 local top=y+offset-cam
 file:write(string.format('%.4f,%d,%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n',
  now,tick,phase,x,y,vx,vy,u('on_ledge'),cam,vram:read(0x8200+first)&63,
  hardware_y(mem:read_u16(f.far_first)),hardware_y(mem:read_u16(f.road_first)),brick,
  hardware_y(first)-offset,top,top+64,mem:read_u8(p+f.hp),u('lives'),u('hero_choice'),u('stage')))
 if now>=next_snap then screen:snapshot(q.output..string.format('/t%05.2f_%s.png',now,phase));next_snap=now+0.25 end
 if phase=='complete' then file:close();m:exit() end
end)
