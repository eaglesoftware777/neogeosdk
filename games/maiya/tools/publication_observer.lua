-- Observe the actual producer/IRQ handoff; never edit game memory.
do
 local machine=manager.machine
 local memory=machine.devices[':maincpu'].spaces['program']
 local ram=emu.item(machine.devices[':spritegen'].items['0/m_videoram'])
 local f=q.layout
 local events=assert(io.open(q.output..'/publication-events.csv','w'))
 events:write('seconds,address,data,mask,pending\n')
 local log=assert(io.open(q.output..'/publications.csv','w'))
 log:write('seconds,published,consumed,published_tick,current_tick,visible,owned,first,tile,flip,complete,rows\n')
 local epoch=0
 local pending,consumed=nil,nil
 local address=f.mg_video_sync
 local shift=address%2==0 and 8 or 0
 _G.maiya_publication_tap=memory:install_write_tap(address&0xfffffe,address|1,'maiya_publication',function(offset,data,mask)
  if (mask&(0xff<<shift))==0 then return end
  local value=(data>>shift)&0xff
  if machine.time:as_double()<=20 then return end
  events:write(string.format('%.6f,%x,%x,%x,%d\n',machine.time:as_double(),offset,data,mask,value));events:flush()
  if value==1 then
   local p=memory:read_u32(f.base+f.player)
   if p<0x100000 or p>=0x10efff then pending=nil;return end
   local index=(p-f.ng_chars)//f.character_size
   local g=f.ng_char_groups+index*f.group_size
   epoch=epoch+1
   local first=memory:read_u16(g+f.group_firstSprite)
   pending={epoch=epoch,tick=memory:read_u16(f.base+f.tick),visible=memory:read_u8(p+f.visible),
    owned=memory:read_u16(f.ng_char_uploaded_first+index*2)==first and 1 or 0,
    first=first,tile=memory:read_u16(g+f.group_tileBase),flip=memory:read_u8(g+f.group_hflip),
    rows=memory:read_u8(g+f.group_heightTiles),strips=memory:read_u8(g+f.group_strips)}
  elseif value==0 and pending then consumed=pending;pending=nil end
 end)
 emu.register_frame_done(function()
  if not consumed then return end
  local complete=1
  for y=0,consumed.rows-1 do for x=0,consumed.strips-1 do
   local tile=consumed.tile+y*consumed.strips+(consumed.flip~=0 and consumed.strips-1-x or x)
   if ram:read((consumed.first+x)*64+y*2)~=tile then complete=0 end
  end end
  log:write(string.format('%.4f,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n',
   machine.time:as_double(),epoch,consumed.epoch,consumed.tick,memory:read_u16(f.base+f.tick),
   consumed.visible,consumed.owned,consumed.first,consumed.tile,consumed.flip,complete,
   ram:read(0x8200+consumed.first)&63));log:flush()
 end)
end
