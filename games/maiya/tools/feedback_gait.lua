local f=qa.offsets
local base=qa.base
local outdir=qa.output
local veil=1
local luna=0
local machine=manager.machine
local mem=machine.devices[':maincpu'].spaces['program']
local screen=machine.screens[':screen']
local vram=emu.item(machine.devices[':spritegen'].items['0/m_videoram'])
local out=assert(io.open(outdir..'/state.csv','w'))
out:write('t,phase,tick,state,tile,x,y,vx,ox,oy,sx,sy,facing,cast,attack,plant,held,wait,queued,run_hold,hurt,shots,gait,dist,right,left,b,camx,camy,stop,spr,scb1,scb2,scb3,scb4,scb3b\n')
local hardware=assert(io.open(outdir..'/tiles.csv','w'))
for y=0,3 do for x=0,4 do hardware:write((y==0 and x==0 and '' or ',')..'r'..y..'c'..x) end end
hardware:write('\n')
local function setin(n,v) for _,port in pairs(machine.ioport.ports) do if port.fields[n] then port.fields[n]:set_value(v and 1 or 0) end end end
local function s16(a) local n=mem:read_u16(a);return n>=32768 and n-65536 or n end
local function s32(a) local n=mem:read_u32(a);return n>=2147483648 and n-4294967296 or n end
local t0=nil
local frame=0
local shots_n=6
-- phases relative to the first frame of play: {name, from, to}
local phases={
 {'walk_taps',0.5,3.0},{'run',3.2,4.6},{'stop',4.6,5.0},
 {'fire_held',5.0,8.0},{'fire_mash',8.3,11.0},{'stand_fire',11.3,12.8},
 {'fire_held_left',13.0,15.5},{'run_left',15.8,17.0},{'stop_left',17.0,17.6}}
local function phase_at(r)
 for _,p in ipairs(phases) do if r>=p[2] and r<p[3] then return p[1] end end
 return '-'
end
emu.register_frame_done(function()
 local t=machine.time:as_double()
 frame=frame+1
 local p=mem:read_u32(base+f.player)
 local state=mem:read_u8(base+f.state)
 local active=p>=0x100000 and p<0x10efff and state==1
 if not t0 then
  if luna>0 then mem:write_u8(luna,1) end
  setin('Coin 1',t>8 and t<8.3)
  setin('1 Player Start',t>12 and t<16 and ((t-12)%2<0.12));setin('P1 Start',t>12 and t<16 and ((t-12)%2<0.12))
  setin('P1 A',t>13 and ((t-13)%0.75<0.12))
  if active and t>20 then t0=t;setin('P1 A',false) end
  return
 end
 local r=t-t0
 local ph=phase_at(r)
 local right,left,b=false,false,false
 if ph=='walk_taps' then right=((r-0.5)%0.45)<0.2
 elseif ph=='run' then right=true
 elseif ph=='fire_held' then right=true;b=true
 elseif ph=='fire_mash' then right=true;b=(frame%6)<3
 elseif ph=='stand_fire' then b=(frame%12)<4
 elseif ph=='fire_held_left' then left=true;b=true
 elseif ph=='run_left' then left=true end
 setin('P1 Right',right);setin('P1 Left',left);setin('P1 B',b);setin('P1 A',false)
 if veil==1 then mem:write_u16(base+f.veil,600) end
 local shots=0
 for i=0,shots_n-1 do
  local s=base+f.shots+i*f.shot_size
  if mem:read_u8(s+f.shot_life)>0 and mem:read_u8(s+f.shot_hostile)==0 then shots=shots+1 end
 end
 if active then
  local first=mem:read_u16(p+32)
  for y=0,3 do for x=0,4 do hardware:write((y==0 and x==0 and "" or ",")..vram:read((first+x)*64+y*2)) end end
  hardware:write("\n")
  out:write(string.format('%.4f,%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n',
   r,ph,mem:read_u16(base+f.tick),state,mem:read_u16(p+f.char_tile),s16(p+f.char_x),s16(p+f.char_y),s32(p+f.char_vx_fp),
   s16(p+f.char_ox),s16(p+f.char_oy),mem:read_u8(p+f.char_sx),mem:read_u8(p+f.char_sy),mem:read_u8(base+f.facing),
   mem:read_u8(base+f.cast),mem:read_u8(base+f.attack),mem:read_u8(base+f.plant),mem:read_u8(base+f.fire_held),
   mem:read_u8(base+f.fire_wait),mem:read_u8(base+f.fire_queued),mem:read_u8(base+f.run_hold),mem:read_u8(base+f.hurt),
   shots,mem:read_u8(base+f.gait),mem:read_u16(base+f.walk_distance),right and 1 or 0,left and 1 or 0,b and 1 or 0,
   s16(base+f.cam_x),s16(base+f.cam_y),mem:read_u8(base+f.gait_stop),
   mem:read_u16(p+32),vram:read(mem:read_u16(p+32)*64),vram:read(0x8000+mem:read_u16(p+32)),vram:read(0x8200+mem:read_u16(p+32)),
   vram:read(0x8400+mem:read_u16(p+32)),vram:read(0x8201+mem:read_u16(p+32))))
 end
 if ph~='-' and frame%12==0 then screen:snapshot(outdir..'/'..ph..string.format('_%05.2f_%06d',r,frame)..'.png') end
 if r>18 then out:close();hardware:close();machine:exit() end
end)
