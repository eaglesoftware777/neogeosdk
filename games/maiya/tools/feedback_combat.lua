-- Targeted real-cartridge checks. Seeded scenes, not campaign balance proof.
local machine=manager.machine
local mem=machine.devices[':maincpu'].spaces['program']
local screen=machine.screens[':screen']
local f,base=qa.offsets,qa.base
local out=assert(io.open(qa.output..'/combat.csv','w'))
out:write('phase,age,x,y,on_ledge,enemy_hp,enemy_hurt,rising,attack,cast,target_x,target_y,target_body_y,target_height,target_active,target_pointer\n')
local function addr(n) return base+f[n] end
local function w8(n,v) mem:write_u8(addr(n),v) end
local function w16(n,v) mem:write_u16(addr(n),v) end
local function r8(n) return mem:read_u8(addr(n)) end
local function valid(p) return p>=0x100000 and p<0x10efff end
local function setin(n,v)
 for _,port in pairs(machine.ioport.ports) do
  if port.fields[n] then port.fields[n]:set_value(v and 1 or 0) end
 end
end
local function position(p,x,y,vx,vy)
 mem:write_u16(p+f.char_x,x);mem:write_u16(p+f.char_y,y)
 mem:write_u32(p+f.char_x_fp,x*256);mem:write_u32(p+f.char_y_fp,y*256)
 mem:write_u32(p+f.char_vx_fp,vx or 0);mem:write_u32(p+f.char_vy_fp,vy or 0)
end
local enemy,body=nil,nil
local function isolate_target()
 for i=0,qa.enemy_count-1 do
  local e=addr('enemies')+i*f.enemy_size
  local b=mem:read_u32(e+f.enemy_body)
  if e~=enemy and valid(b) then position(b,2000,192) end
 end
end
local phases={'below','same','kneel_below','rise','left_edge','left_outside','right_edge','right_outside'}
local started=false
local age,phase=0,1
local function reset(p)
 for _,n in ipairs({'hurt','cast','attack','plant','dp_step','dp_timer','rise_wait',
                     'rising','combo_timer','crouch_timer','airborne','drop','sitting'}) do w8(n,0) end
 mem:write_u16(addr('previous_joy'),0)
 mem:write_u8(addr('combo_buffer'),0);mem:write_u8(addr('combo_buffer')+1,0)
 for i=0,2 do mem:write_u8(addr('fire')+i,0) end
 for i=0,qa.shot_count-1 do mem:write_u8(addr('shots')+i*f.shot_size+f.shot_life,0) end
 mem:write_u8(enemy+f.enemy_type,1) -- ordinary armored beetle, three hit points
 mem:write_u8(enemy+f.enemy_hurt,0);mem:write_u8(enemy+f.enemy_mood,0)
 mem:write_u8(body+f.char_hp,3);mem:write_u8(body+f.char_max_hp,3)
 position(body,524,192)
 local name=phases[phase]
 local x,y,vy=500,192,0
 if name=='below' or name=='kneel_below' then y=128 end
 if name=='left_edge' then x,y,vy=476,127,512 end
 if name=='left_outside' then x,y,vy=475,127,512 end
 if name=='right_edge' then x,y,vy=548,127,512 end
 if name=='right_outside' then x,y,vy=549,127,512 end
 position(p,x,y,0,vy);w16('player_prev_y',y)
 w8('on_ledge',0)
 isolate_target()
end
emu.register_frame_done(function()
 local t=machine.time:as_double()
 local p=mem:read_u32(addr('player'))
 local active=valid(p) and r8('state')==1
 if not started then
  setin('Coin 1',t>8 and t<8.3)
  setin('1 Player Start',t>12 and t<16 and ((t-12)%2<0.12))
  setin('P1 Start',t>12 and t<16 and ((t-12)%2<0.12))
  setin('P1 A',t>13 and ((t-13)%0.75<0.12))
  if active and t>20 then
   setin('P1 A',false);w16('veil',600)
   -- Bring an ordinary encounter into view, then use its existing body.
   position(p,500,192)
   for i=0,qa.enemy_count-1 do
    local e=addr('enemies')+i*f.enemy_size
    local b=mem:read_u32(e+f.enemy_body)
    if valid(b) then enemy,body=e,b;break end
   end
   if enemy then started=true;reset(p) end
  end
  return
 end
 w16('veil',600)
 local name=phases[phase]
 out:write(string.format('%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n',name,age,
  mem:read_u16(p+f.char_x),mem:read_u16(p+f.char_y),r8('on_ledge'),
  mem:read_u8(body+f.char_hp),mem:read_u8(enemy+f.enemy_hurt),r8('rising'),r8('attack'),r8('cast'),
  mem:read_u16(body+f.char_x),mem:read_u16(body+f.char_y),mem:read_u16(body+f.char_body_y),
  mem:read_u16(body+f.char_body_h),mem:read_u8(body+f.char_active),mem:read_u32(enemy+f.enemy_body)))
 position(body,524,192)
 isolate_target()
 local right,down,b=false,false,false
 if name=='rise' then
  right=(age>=2 and age<5) or (age>=8 and age<14)
  down=age>=5 and age<14;b=age>=11 and age<14
 elseif name=='below' or name=='same' or name=='kneel_below' then
  b=age>=4 and age<6;down=name=='kneel_below'
 end
 setin('P1 Right',right);setin('P1 Down',down);setin('P1 B',b)
 if age==12 or age==24 then screen:snapshot(qa.output..'/'..name..'_'..age..'.png') end
 age=age+1
 if age>=40 then
  age=0;phase=phase+1
  setin('P1 Right',false);setin('P1 Down',false);setin('P1 B',false)
  if phase>#phases then out:close();machine:exit();return end
  reset(p)
 end
end)
