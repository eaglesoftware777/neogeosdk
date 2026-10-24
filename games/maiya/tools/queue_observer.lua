-- Host-only queue snapshots from the matching ELF. No game memory writes.
do
 local machine=manager.machine
 local memory=machine.devices[':maincpu'].spaces['program']
 local f=q.layout
 local log=assert(io.open(q.output..'/pending-groups.csv','w'))
 log:write('seconds,tick,player_x,requested_tile,hardware_tile,list,head_age,index,first,strips,rows,dirty,visible,map_rows\n')
 local ram=emu.item(machine.devices[':spritegen'].items['0/m_videoram'])
 emu.register_frame_done(function()
  local p=memory:read_u32(f.base+f.player)
  if p<0x100000 or p>=0x10efff or machine.time:as_double()<20 then return end
  local first=memory:read_u16(p+f.sprite_first)
  local requested=memory:read_u16(p+f.sprite_tile)
  local hardware=ram:read(first*64)
  if requested==hardware then return end
  for list,name in ipairs({'ngsg_high','ngsg_norm','ngsg_low'}) do
   local n=memory:read_u16(f[name..'_n'])
   local age=memory:read_u8(f.ngsg_head_age+list-1)
   for i=0,n-1 do
    local g=memory:read_u32(f[name]+4*i)
    log:write(string.format('%.4f,%d,%d,%d,%d,%s,%d,%d,%d,%d,%d,%d,%d,%d\n',
     machine.time:as_double(),memory:read_u16(f.base+f.tick),memory:read_i16(p+f.x),
     requested,hardware,name,age,i,memory:read_u16(g+f.group_firstSprite),
     memory:read_u8(g+f.group_strips),memory:read_u8(g+f.group_heightTiles),
     memory:read_u8(g+f.group_dirty),memory:read_u8(g+f.group_visible),memory:read_u8(g+f.group_mapRows)))
   end
  end
  log:flush()
 end)
end
