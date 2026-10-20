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
-- Observe actual hardware-port stores without adding cartridge instructions.
-- CPU bookkeeping can finish in active display after its last safe VRAM store;
-- this distinguishes that case from a write that really crossed the window.
local active_video,active_palette,covered_video=0,0,0
local video_writes,palette_writes=0,0
local video_line_min,video_line_max=264,0
local function interactive_picture()
 return cpu:read_u8(a.match+a.state)==1 and cpu:read_u8(a.match+a.round)>0
    and (not a.scene_setup or cpu:read_u8(a.scene_setup)==0)
    and (not a.palette_state or cpu:read_u8(a.palette_state+10)<16)
end
local video_tap=cpu:install_write_tap(0x3c0002,0x3c0003,'slf_video_timing',function(offset,data,mask)
 video_writes=video_writes+1
 local y=machine.screens[':screen']:vpos()
 video_line_min=math.min(video_line_min,y);video_line_max=math.max(video_line_max,y)
 if y>=16 and y<240 then
  if interactive_picture() then active_video=active_video+1 else covered_video=covered_video+1 end
 end
end)
local palette_tap=cpu:install_write_tap(0x400000,0x401fff,'slf_palette_timing',function(offset,data,mask)
 palette_writes=palette_writes+1
 local y=machine.screens[':screen']:vpos()
 if y>=16 and y<240 and interactive_picture() then active_palette=active_palette+1 end
end)
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
 assert(video_tap and palette_tap,'Hardware write observers were not installed')
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
  local extra=''
  extra=extra..string.format(',"video_writes_observed":%d,"palette_writes_observed":%d,"video_line_min":%d,"video_line_max":%d,"interactive_active_video_writes":%d,"interactive_active_palette_writes":%d,"covered_active_video_writes":%d',video_writes,palette_writes,video_line_min,video_line_max,active_video,active_palette,covered_video)
  for _,name in ipairs({'hardware_ticks','input_sequence','input_dropped','input_catchups','commit_skipped','scene_input_ignored'}) do
   if a[name] then extra=extra..string.format(',"%s":%d',name,cpu:read_u32(a[name])) end
  end
  if a.irq_lines_peak then extra=extra..string.format(',"irq_lines_peak":%d',cpu:read_u16(a.irq_lines_peak)) end
  if a.perf then
   if a.perf_commit_phase_peak then
    local values={}
    for i=0,7 do values[i+1]=tostring(cpu:read_u16(a.perf+a.perf_commit_phase_peak+i*2)) end
    extra=extra..',"commit_phase_peak":['..table.concat(values,',')..']'
   end
   for _,name in ipairs({'lines_peak','commit_late','commit_spill','commit_lines_peak','queue_full'}) do
    extra=extra..string.format(',"%s":%d',name,cpu:read_u16(a.perf+a['perf_'..name]))
   end
   for _,name in ipairs({'outside_sum','outside_active_sum','overruns'}) do
    extra=extra..string.format(',"%s":%d',name,cpu:read_u32(a.perf+a['perf_'..name]))
   end
  end
  if a.render_queue_stats then
   extra=extra..string.format(',"command_overflows":%d,"palette_overflows":%d',cpu:read_u32(a.render_queue_stats),cpu:read_u32(a.render_queue_stats+4))
  end
  for _,name in ipairs({'actor_requested','actor_uploading','actor_displayed','actor_age_peak'}) do
   if a[name] then
    local values={}
    for i=0,3 do values[i+1]=tostring(name=='actor_age_peak' and cpu:read_u16(a[name]+i*2) or cpu:read_u32(a[name]+i*4)) end
    extra=extra..',"'..name..'":['..table.concat(values,',')..']'
   end
  end
  log:write(string.format('{"hardware_frame":%d,"t":%.5f,"state":%d,"simulation_frame":%d,"scanline_strips":%d,"peak_strips":%d,"fight_observations":%d,"missed_simulation_frames":%d,"presentation_checks":%d,"palette_pending_max":%d,"super_freezes":%d,"actor_commits":%d,"actor_deferred":%d,"actor_deadline_misses":%d%s}\n',frame,machine.time:as_double(),state,sim,strips,peak,observed,missed,checked,palette_pending_max,super_freezes, a.actor_commits and cpu:read_u16(a.actor_commits) or 0, a.actor_deferred and cpu:read_u16(a.actor_deferred) or 0, a.actor_deadline_misses and cpu:read_u16(a.actor_deadline_misses) or 0,extra));log:flush()
  assert(strips<=96,'Sprite scanline limit exceeded')
 end
 if os.getenv('SLF_MOTION_FRAMES')=='1' and machine.time:as_double()>22 and machine.time:as_double()<32 then
  machine.screens[':screen']:snapshot(string.format('%s/motion_%05d.png',output,frame))
 end
end)
