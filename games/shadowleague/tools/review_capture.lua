-- Additional review evidence: live cartridge frames and read-only sprite RAM.
local output=assert(os.getenv('SLF_CAPTURE_DIR'))
local root=assert(os.getenv('SLF_SCRIPT_ROOT'))
dofile(root..'/qa.lua')
local machine=manager.machine
local cpu=machine.devices[':maincpu'].spaces['program']
local a=dofile(output..'/layout.lua')
local log=assert(io.open(output..'/performance.jsonl','w'))
local sprite=machine.devices[':spritegen']
local ram=nil
if sprite and sprite.items then
 for name,id in pairs(sprite.items) do
  if name:find('m_videoram') and not name:find('drawsource') then ram=emu.item(id);break end
 end
end
assert(ram,'MAME sprite save-item RAM not exposed; cannot measure sprite load')
local frame=0
local function maximum_strips()
 local diff={};for y=0,224 do diff[y]=0 end
 local function interval(first,last)
  first=math.max(0,first);last=math.min(224,last)
  if first<last then diff[first]=diff[first]+1;diff[last]=diff[last]-1 end
 end
 local y,rows=0,0
 for number=0,380 do
  local control=ram:read(0x8200+number)
  if (control & 64)==0 then y=(512-(control>>7)) & 511;rows=control & 63 end
  if rows>=32 then interval(0,224)
  elseif rows>0 then
   interval(y,y+rows*16)
   if y+rows*16>512 then interval(0,y+rows*16-512) end
  end
 end
 local current,maximum=0,0
 for line=0,223 do current=current+diff[line];maximum=math.max(maximum,current) end
 return maximum
end
local peak=0
local previous_state,previous_sim,previous_round=0,0,0
local announcement_pending=0
local observed,missed=0,0
local copies,pending,checked=0,0,0
local palette_pending,palette_pending_max=0,0
local super_freezes,previous_freeze=0,0
emu.register_frame_done(function()
 frame=frame+1
 local strips=maximum_strips();peak=math.max(peak,strips)
 if a.palette_state and machine.time:as_double()>3 then
  palette_pending=cpu:read_u8(a.palette_state+13)~=0 and palette_pending+1 or 0
  palette_pending_max=math.max(palette_pending_max,palette_pending)
 end
 assert(strips<=96,'Sprite scanline limit exceeded')
 if a.actor_deadline_misses then assert(cpu:read_u16(a.actor_deadline_misses)==0,'Actor write crossed the drawing deadline') end
 if os.getenv('SLF_ATTRACT_ONLY')=='1' and a.card_buffer then
  local count=cpu:read_u16(a.bank_copies)
  if count~=copies and count>0 and count<=16 then copies=count;pending=2 end
  if pending>0 then
   pending=pending-1
   if pending==0 then
    local expected=cpu:read_u32(a.card_buffer)
    local attr=ram:read(65)
    local address=ram:read(64) | ((attr & 0xf0)<<12)
    assert(expected<0x100000,'Card lies outside the 20-bit graphics space')
    assert(address==expected,'Live SCB1 lost high C-ROM address bits')
    assert((attr>>8)==cpu:read_u8(a.card_buffer+520),'Live SCB1 palette does not match copied tile map')
    checked=checked+1
   end
  end
 end
 local state=cpu:read_u8(a.match+a.state)
 local freeze=a.super_freeze and cpu:read_u8(a.match+a.super_freeze) or 0
 if freeze>0 and previous_freeze==0 then super_freezes=super_freezes+1 end
 previous_freeze=freeze
 local round=cpu:read_u8(a.match+a.round)
 local sim=cpu:read_u32(a.match+a.frame)
 -- Capture real round transitions after queued FIX writes reach the display.
 -- This observes match state; it never changes the simulation or its artwork.
 if round>0 and (state~=previous_state or round~=previous_round) then announcement_pending=16 end
 if announcement_pending>0 then
  announcement_pending=announcement_pending-1
  if announcement_pending==14 or announcement_pending==8 or announcement_pending==0 then
   machine.screens[':screen']:snapshot(string.format('%s/announce_%05d_state%d_round%d.png',output,frame,state,round))
  end
 end
 if state==1 and previous_state==1 and sim>=previous_sim then
  observed=observed+1
  if sim==previous_sim then missed=missed+1 end
 end
 previous_state=state;previous_sim=sim;previous_round=round
 if frame%30==0 then
  log:write(string.format('{"hardware_frame":%d,"t":%.5f,"state":%d,"simulation_frame":%d,"scanline_strips":%d,"peak_strips":%d,"fight_observations":%d,"missed_simulation_frames":%d,"presentation_checks":%d,"palette_pending_max":%d,"super_freezes":%d,"actor_commits":%d,"actor_deferred":%d,"actor_deadline_misses":%d}\n',frame,machine.time:as_double(),state,sim,strips,peak,observed,missed,checked,palette_pending_max,super_freezes, a.actor_commits and cpu:read_u16(a.actor_commits) or 0, a.actor_deferred and cpu:read_u16(a.actor_deferred) or 0, a.actor_deadline_misses and cpu:read_u16(a.actor_deadline_misses) or 0));log:flush()
  assert(strips<=96,'Sprite scanline limit exceeded')
 end
 if os.getenv('SLF_MOTION_FRAMES')=='1' and machine.time:as_double()>22 and machine.time:as_double()<32 then
  machine.screens[':screen']:snapshot(string.format('%s/motion_%05d.png',output,frame))
 end
end)
