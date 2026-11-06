-- Read-only host observation. Retain taps for the entire emulator lifetime.
do
 local machine=manager.machine
 local cpu=machine.devices[':maincpu']
 local memory=cpu.spaces['program']
 local screen=machine.screens[':screen']
 local f=q.layout
 local log=assert(io.open(q.output..'/video-stores.csv','w'))
 log:write('seconds,total_vram,game_vram,active_vram,game_palette,active_palette,first_offset,scanline_peak\n')
 local totals={0,0,0,0,0}
 local first=-1
 local peak=0
 local ram=emu.item(machine.devices[':spritegen'].items['0/m_videoram'])
 local function strip_bound()
  -- Conservative bound: ignore shrink and include offscreen-X strips,
  -- using the visible raw raster lines16..239, not cropped screenshot Y.
  local diff={};for line=16,240 do diff[line]=0 end
  local function interval(a,b)
   a=math.max(16,a);b=math.min(240,b)
   if a<b then diff[a]=diff[a]+1;diff[b]=diff[b]-1 end
  end
  local y,rows=0,0
  for number=0,380 do
   local control=ram:read(0x8200+number)
   if (control&64)==0 then y=(512-(control>>7))&511;rows=control&63 end
   if rows>=32 then interval(16,240)
   elseif rows>0 then
    interval(y,y+rows*16)
    if y+rows*16>512 then interval(0,y+rows*16-512) end
   end
  end
  local n,bound=0,0
  for line=16,239 do n=n+diff[line];bound=math.max(bound,n) end
  return bound
 end
 local function playing()
  local p=memory:read_u32(f.base+f.player)
  return p>=0x100000 and p<0x10efff and memory:read_u8(f.base+f.state)==1 and
         memory:read_u8(f.base+f.demo)==0 and machine.time:as_double()>20
 end
 _G.maiya_video_probe_taps={}
 _G.maiya_video_probe_taps[1]=memory:install_write_tap(0x3c0000,0x3c0007,'maiya_video_stores',function(offset,data,mask)
  if first<0 then first=offset end
  if offset==0x3c0002 then
   totals[1]=totals[1]+1
   if playing() then
    totals[2]=totals[2]+1
    local line=screen:vpos()
    if line>=16 and line<240 then totals[3]=totals[3]+1 end
   end
  end
 end)
 _G.maiya_video_probe_taps[2]=memory:install_write_tap(0x400000,0x401fff,'maiya_palette_stores',function(offset,data,mask)
  if playing() then
   totals[4]=totals[4]+1
   local line=screen:vpos()
   if line>=16 and line<240 then totals[5]=totals[5]+1 end
  end
 end)
 emu.register_frame_done(function()
  if playing() then peak=math.max(peak,strip_bound()) end
  log:write(string.format('%.4f,%d,%d,%d,%d,%d,%d,%d\n',machine.time:as_double(),totals[1],totals[2],totals[3],totals[4],totals[5],first,peak))
  log:flush()
 end)
end
