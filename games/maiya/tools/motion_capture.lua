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
file:write('seconds,tick,phase,x,y,x_fp,vx,vy,hp,lives,hero,stage,gait,distance,scb_rows,sx,sy,facing,stop,plant,cast,attack,turning,clock,tile,veil,hurt,flash,cam_y,canvas_top,canvas_bottom')
for y=0,3 do for x=0,4 do file:write(',r'..y..'c'..x) end end
for i=1,15 do file:write(',pal'..i) end
file:write('\n')
local started,chooser=nil,nil
local last_tick=-1
local step=-1
local right,left,up,jump,fire=false,false,false,false,false
local next_snap=0
local seen={}
local sample=0
local start_lives=nil
local phases={
 {'prepare',120}, {'walk_right',160}, {'run_right',50}, {'stop_right',20},
 {'walk_left',160}, {'run_left',50}, {'stop_left',20},
 {'reverse_right',35}, {'reverse_left',40}, {'settle',20},
 {'fire_right',100}, {'fire_left',100}, {'fire_still',60},
 {'jump_right',38}, {'land',50}, {'wall_left',180}, {'complete',1}
}
local phase='prepare'
local index,phase_tick=1,0
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
  if active and now>20 and chooser then started=now;start_lives=u('lives');controls(false,false,false,false,false) end
  return
 end
 input('Coin 1',false);input('1 Player Start',false);input('P1 Start',false)
 if not active or u('lives')~=start_lives then file:flush();m:exit();return end
 local tick=mem:read_u16(f.base+f.tick)
 local x,y=mem:read_i16(p+f.x),mem:read_i16(p+f.y)
 if tick~=last_tick then
  step=step+1
  phase_tick=phase_tick+1
  if phase_tick>phases[index][2] then index=index+1;phase_tick=1 end
  phase=phases[index][1]
  right,left,up,jump,fire=false,false,false,false,false
  if phase=='prepare' then right=x<150;fire=true
  elseif phase=='walk_right' then right=phase_tick%24<10
  elseif phase=='walk_left' then left=phase_tick%24<10
  elseif phase=='run_right' or phase=='reverse_right' then right=true
  elseif phase=='run_left' or phase=='reverse_left' or phase=='wall_left' then left=true
  elseif phase=='fire_right' then right=true;fire=true
  elseif phase=='fire_left' then left=true;fire=true
  elseif phase=='fire_still' then fire=true
  elseif phase=='jump_right' then right=true;jump=phase_tick<25 end
  last_tick=tick
 end
 controls(right,left,up,jump,fire)
 local first=mem:read_u16(p+f.sprite_first)
 local cam=mem:read_i16(f.base+f.cam_y)
 local top=y+mem:read_i16(p+f.sprite_offset_y)-cam
 local tile=mem:read_u16(p+f.sprite_tile)
 file:write(string.format('%.4f,%d,%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d',
  now,tick,phase,x,y,mem:read_i32(p+f.x_fp),mem:read_i32(p+f.vx_fp),mem:read_i32(p+f.vy_fp),
  mem:read_u8(p+f.hp),u('lives'),u('hero_choice'),u('stage'),u('gait'),mem:read_u16(f.base+f.walk_distance),
  vram:read(0x8200+first)&63,mem:read_u8(p+f.scale_x),mem:read_u8(p+f.scale_y),u('facing'),
  u('gait_stop'),u('plant'),u('cast'),u('attack'),u('turning'),u('gait_clock'),tile,
  mem:read_u16(f.base+f.veil),u('hurt'),u('flash'),cam,top,top+64))
 for row=0,3 do for col=0,4 do file:write(','..vram:read((first+col)*64+row*2)) end end
 for i=1,15 do file:write(','..mem:read_u16(0x400000+4*32+i*2)) end
 file:write('\n')
 local key=phase..'_'..vram:read(first*64)
 sample=sample+1
 if q.frames then screen:snapshot(q.output..string.format('/frames/%05d.png',sample)) end
 if not seen[key] or now>=next_snap then
  screen:snapshot(q.output..'/'..key..'.png');seen[key]=true;next_snap=now+1
 end
 if phase=='complete' then file:close();m:exit() end
end)
