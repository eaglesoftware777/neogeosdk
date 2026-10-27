local m=manager.machine
local mem=m.devices[':maincpu'].spaces['program']
local vram=emu.item(m.devices[':spritegen'].items['0/m_videoram'])
local log=assert(io.open(q.out..'/trace.csv','w'))
log:write('seconds,clock,pc,pad,sfx,music,journey,room,nparty,px,py,hp,card\n')
local seen={};local picked=false;local choose_time=nil;local resume_time=nil
local function input(name,on)
 for _,p in pairs(m.ioport.ports) do if p.fields[name] then p.fields[name]:set_value(on and 1 or 0) end end
end
local function r(name,n)
 local a=q.syms[name];if not a then return 0 end
 if n==4 then return mem:read_u32(a) elseif n==2 then return mem:read_u16(a) end
 return mem:read_u8(a)
end
local function text()
 local s={}
 for y=0,31 do for x=0,39 do local c=vram:read(0x7000+x*32+y)%4096-0xD00;s[#s+1]=string.char(c>=32 and c<=126 and c or 32) end end
 return table.concat(s)
end
local function dump(name,a,b)
 local f=assert(io.open(q.out..'/'..name,'wb'))
 for first=a,b,4096 do local t={} for i=first,math.min(b,first+4095) do t[#t+1]=string.char(mem:read_u8(i)) end f:write(table.concat(t)) end
 f:close()
end
emu.register_frame_done(function()
 local t=m.time:as_double();local clock=r('ng_cd_frames',4)
 local ready=mem:read_u32(0x100)==0x4e454f2d and mem:read_u8(0x10fd80)==0x82
 if not ready then clock=0 end
 local a=(q.profile=='stock' and ((t>15 and t<15.2) or (t>25 and t<25.2)))
 local start=q.profile=='stock' and t>18 and t<18.3;local right=false;local left=q.profile=='stock' and t>8 and t<8.1;local b=false
 local down=false
 if ready then
  local words=text();local choose=words:find('WHO SETS OUT',1,true)
  a=clock>180 and clock<2200 and clock%45<4
  start=clock>120 and clock<500 and clock%150<5
  if choose then
   if not choose_time then choose_time=clock end
   start=false;a=false
   if q.journey==1 and not picked then right=clock-choose_time<5;if clock-choose_time>=5 then picked=true end else picked=true end
   if picked and clock-choose_time>30 then a=clock%30<4 end
  end
  if q.resume and words:find('CONTINUE THE JOURNEY',1,true) then
   if not resume_time then resume_time=clock end
   start=false;a=false;down=clock-resume_time<5
   if clock-resume_time>30 then a=clock%30<4 end
  end
  right=right or (clock>2250 and clock<2440)
  left=clock>2500 and clock<2630
  b=(clock>2700 and clock<2900 and clock%35<5) or (clock>3400 and clock<3410)
  start=start or (clock>3000 and clock<3005)
  if clock>3100 and clock<3105 then right=clock%25<3 end
  if clock>3250 and clock<3255 then a=true end
 end
 input('P1 Down',down)
 input('P1 Up',q.profile=='stock' and not ready and t>9 and t<9.1)
 input('P1 A',a);input('P1 Start',start);input('P1 Right',right);input('P1 Left',left);input('P1 B',b)
 if ready and clock%10==0 then
  log:write(string.format('%.4f,%d,%06X,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n',t,clock,(m.devices[':maincpu'].state['PC'] or m.devices[':maincpu'].state['CURPC']).value,r('ng_cd_input_seen',2),r('ml_cd_sfx_calls',4),r('ml_cd_music_track',2),r('ml_journey'),r('room',2),r('nparty'),r('px',4),r('py',4),r('hp'),mem:read_u8(0x10FDC6)));log:flush()
 end
 if ready then
  for _,n in ipairs({60,210,700,1300,1900,2300,2600,3000,3280,3500,4200}) do if clock>=n and not seen[n] then m.screens[':screen']:snapshot(q.out..'/runtime-'..n..'.png');seen[n]=true end end
 end
 if ready and clock>=4210 then dump('checkpoint.bin',0x100400,0x1005ff);dump('program.bin',0x100,q.program_bytes-1);dump('firmware.bin',0xc00000,0xc7ffff);log:close();m:exit() end
 if t>175 then m.screens[':screen']:snapshot(q.out..'/timeout.png');log:close();m:exit() end
end)
