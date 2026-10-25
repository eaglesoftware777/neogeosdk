local m=manager.machine
local mem=m.devices[':maincpu'].spaces['program']
local vram=emu.item(m.devices[':spritegen'].items['0/m_videoram'])
local log=assert(io.open(q.out..'/trace.csv','w'))
log:write('seconds,clock,pad,sfx,music,hero,tick,px,py,pz,hp,alive,state,loads,bg,errors\n')
local seen={};local choose_clock=nil;local play_clock=nil
local function input(name,on)
 for _,p in pairs(m.ioport.ports) do if p.fields[name] then p.fields[name]:set_value(on and 1 or 0) end end
end
local function read(a,n)
 if n==4 then return mem:read_u32(a) elseif n==2 then return mem:read_u16(a) end
 return mem:read_u8(a)
end
local function r(name,n) local a=q.syms[name];return a and read(a,n or 1) or 0 end
local function f(name) local v=q.fields[name];return v and read(v.addr,v.size) or 0 end
local function text()
 local s={}
 for y=0,31 do for x=0,39 do local c=vram:read(0x7000+x*32+y)%4096-0xD00;s[#s+1]=string.char(c>=32 and c<=126 and c or 32) end end
 return table.concat(s)
end
local function dump(name,a,b)
 local file=assert(io.open(q.out..'/'..name,'wb'))
 for first=a,b,4096 do local t={} for i=first,math.min(b,first+4095) do t[#t+1]=string.char(mem:read_u8(i)) end file:write(table.concat(t)) end
 file:close()
end
local finished=false
local function finish()
 if finished then return end
 finished=true
 if q.media_audit then
  for bank=0,3 do
   local size=math.min(0x100000,q.resident_bytes-bank*0x100000)
   mem:write_u8(0xFF0121,0);mem:write_u8(0xFF0105,0)
   mem:write_u8(0xFF01A1,bank);mem:write_u8(0xFF016F,1)
   dump('resident'..bank..'.bin',0xE00000,0xE00000+size-1)
   mem:write_u8(0xFF0141,0);mem:write_u8(0xFF016F,0)
  end
 end
 dump('program.bin',0x100,q.program_bytes-1)
 dump('firmware.bin',0xc00000,0xc7ffff)
 log:close();m:exit()
end
emu.register_frame_done(function()
 local t=m.time:as_double();local clock=r('ng_cd_frames',4)
 local ready=mem:read_u32(0x100)==0x4e454f2d and mem:read_u8(0x10fd80)==0x82
 if not ready then clock=0 end
 local a=q.profile=='stock' and ((t>15 and t<15.2) or (t>25 and t<25.2))
 local start=q.profile=='stock' and t>18 and t<18.3
 local right=false;local left=q.profile=='stock' and t>8 and t<8.1
 local up=q.profile=='stock' and not ready and t>9 and t<9.1
 local down=false;local b=false;local c=false;local d=false
 if ready and not q.media_audit then
  local choose=text():find('CHOOSE YOUR HERO',1,true)
  a=clock>180 and clock%45<5;start=clock>120 and clock<500 and clock%150<5
  if choose then
   if not choose_clock then choose_clock=clock end
   local delta=clock-choose_clock
   start=false;a=false
   right=(q.hero>=1 and delta>=8 and delta<12) or (q.hero>=2 and delta>=20 and delta<24)
   a=delta>40 and clock%30<5
  end
  if f('tick')>0 and f('alive')>0 and not play_clock then play_clock=clock end
  if play_clock then
   local delta=clock-play_clock
   start=false;a=false
   right=delta<65 or (delta>=300 and delta<350)
   left=delta>=80 and delta<145
   b=delta>=170 and delta<177
   a=(delta>=185 and delta%15<5)
   d=delta>=500 and delta<507
   c=delta>=650 and delta<657
   down=delta>=710 and delta<718
   up=delta>=730 and delta<738
   for _,n in ipairs({10,70,150,195,350,550,850}) do
    if delta>=n and not seen[n] then m.screens[':screen']:snapshot(q.out..'/fight-'..n..'.png');seen[n]=true end
   end
   if delta>900 then finish();return end
  end
 end
 input('P1 Start',start);input('P1 A',a);input('P1 B',b);input('P1 C',c);input('P1 D',d)
 input('P1 Right',right);input('P1 Left',left);input('P1 Up',up);input('P1 Down',down)
 if ready and (clock%5==0 or q.media_audit) then
  log:write(string.format('%.4f,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n',t,clock,r('ng_cd_input_seen',2),r('ns_cd_sfx_calls',4),r('ns_cd_music_track',2),r('s_hero'),f('tick'),f('px'),f('py'),f('pz'),f('hp'),f('alive'),f('state'),r('ns_cd_background_loads',4),r('ns_cd_background',2),r('ns_cd_load_failures',4)));log:flush()
 end
 if ready and q.media_audit and r('ns_cd_media_done',2)==1 then finish();return end
 if t>210 then m.screens[':screen']:snapshot(q.out..'/timeout.png');finish() end
end)
