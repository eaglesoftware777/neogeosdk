-- q comes from the matching ELF. Controller inputs only; RAM is read-only.
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
local function active()
 return mem:read_u8(f.s_bound)==1 and mem:read_u8(0x100700)~=0 and mem:read_u16(f.puzzle+f.tick)>0
end
local phases={{'settle',30,0},{'right',30,8},{'left',30,4},{'up',24,1},
 {'down',24,2},{'diagonal',30,9},{'reverse',30,6},{'opposed',24,12},
 {'release',30,0},{'complete',1,0}}
if q.route=='bubble' then
 phases={{'settle',30,0},{'aim_left',8,4},{'fire',1,16},{'release',3,0},
  {'steer_in_flight',8,8},{'opposed',6,12},{'coast',40,0},
  {'recenter',1,1},{'swap_next',1,32},{'settle_next',15,0},
  {'hammer',1,64},{'settle_hammer',25,0},{'complete',1,0}}
end
local file=assert(io.open(q.output..'/controls.csv','w'))
file:write('seconds,frame,phase,joy,repeat,tick,state,moves,mode,w,h,angle,x,y,vx,vy,panel,score\n')
local started=false
local index,t,last=1,0,-1
local seen_play=false
local bot_last,bot_queue,bot_index,bot_wait=-1,{},1,0
local approach_won,approach_moves=false,0
local function buttons(bits)
 input('P1 Right',(bits&8)~=0);input('P1 Left',(bits&4)~=0)
 input('P1 Down',(bits&2)~=0);input('P1 Up',(bits&1)~=0)
 input('P1 A',(bits&16)~=0);input('P1 B',(bits&32)~=0)
 input('P1 C',(bits&64)~=0);input('P1 D',(bits&128)~=0)
end
local function first_board()
 local frame=mem:read_u16(f.s_frames)
 if frame==bot_last then return end
 bot_last=frame
 approach_moves=math.max(approach_moves,mem:read_u16(f.puzzle+f.moves_used))
 if mem:read_u8(f.puzzle+f.state)==6 then
  approach_won=true;buttons(0);return
 end
 if bot_wait>0 then bot_wait=bot_wait-1;return end
 if bot_index<=#bot_queue then
  local item=bot_queue[bot_index];bot_index=bot_index+1
  buttons(item[1]);bot_wait=item[2]-1;return
 end
 buttons(0)
 if mem:read_u8(f.puzzle+f.state)~=0 then return end
 local snapshot=q.output..'/hint-board.bin'
 local out=assert(io.open(snapshot,'wb'))
 for _,name in ipairs({'mode','w','h','colors'}) do out:write(string.char(mem:read_u8(f.puzzle+f[name]))) end
 for _,name in ipairs({'color','kind','cover'}) do
  for cell=0,48 do out:write(string.char(mem:read_u8(f.puzzle+f[name]+cell))) end
 end
 out:close()
 local pipe=assert(io.popen(string.format('%q %q',q.hint_helper,snapshot),'r'))
 local hint=pipe:read('*a');local ok=pipe:close();assert(ok,'Host hint rejected snapshot')
 local cell,dir=hint:match('(%d+)%s+(%d+)');assert(cell and dir,'No host hint')
 local direction=tonumber(dir)==0 and 8 or 2
 -- D puts the real cursor on exactly the game's own hinted cell, A picks
 -- it up, and a direction executes the normal player swap. No RAM writes.
 bot_queue={{128,1},{0,4},{16,1},{0,2},{direction,1},{0,20}};bot_index=1
end
local totals={vram=0,palette=0,active_vram=0,active_palette=0}
_G.sw_controls_taps={
 mem:install_write_tap(0x3c0000,0x3c0007,'sw_video',function(offset,data,mask)
  if offset==0x3c0002 and started and active() then
   totals.vram=totals.vram+1
   if screen:vpos()>=16 and screen:vpos()<240 then totals.active_vram=totals.active_vram+1 end
  end
 end),
 mem:install_write_tap(0x400000,0x401fff,'sw_palette',function(offset,data,mask)
  if started and active() then
   totals.palette=totals.palette+1
   if screen:vpos()>=16 and screen:vpos()<240 then totals.active_palette=totals.active_palette+1 end
  end
 end)
}
emu.register_frame_done(function()
 local now=m.time:as_double()
 if not started then
  if active() then
   if not seen_play then buttons(0) end
   seen_play=true
   -- Stop boot pulses as soon as real play appears. Another START edge
   -- would legitimately pause an AES game that reaches its board early.
   input('Coin 1',false);input('1 Player Start',false);input('P1 Start',false)
   if q.route=='bubble' and mem:read_u8(f.puzzle+f.mode)==0 and q.hint_helper then
    first_board();return
   end
   buttons(0)
   if now>20 then started=true;screen:snapshot(q.output..'/board.png') end
  else
   if seen_play then buttons(0) end
   input('Coin 1',not seen_play and now>8 and now<8.3)
   local start=not seen_play and now>12 and now<16 and (now%1<0.12)
   input('1 Player Start',start);input('P1 Start',start)
   input('P1 A',now>13 and now%1<0.12)
  end
  return
 end
 local frame=mem:read_u16(f.s_frames)
 if frame==last then return end
 last=frame
 t=t+1
 if t>phases[index][2] then index=index+1;t=1;screen:snapshot(q.output..'/'..phases[index][1]..'.png') end
 local control=vram:read(0x8200+21)
 local panel=(control&63)==14 and ((control>>7)&511)==492 and ((vram:read(0x8400+21)>>7)&511)==4
 for col=0,5 do
  if (vram:read(0x8200+21+col)&63)~=14 then panel=false end
  for row=0,13 do
   if vram:read((21+col)*64+row*2)~=q.panel.tile_base+col+row*6 then panel=false end
  end
 end
 local score=true
 for col=1,5 do
  if (vram:read(0x7002+col*32+13)&4095)~=0xD00+string.byte('SCORE',col) then score=false end
 end
 file:write(string.format('%.6f,%d,%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n',now,frame,phases[index][1],
  mem:read_u16(f.s_joy),mem:read_u16(f.s_repeat),mem:read_u16(f.puzzle+f.tick),
  mem:read_u8(f.puzzle+f.state),mem:read_u16(f.puzzle+f.moves_used),
  mem:read_u8(f.puzzle+f.mode),mem:read_u8(f.puzzle+f.w),mem:read_u8(f.puzzle+f.h),
  mem:read_u16(f.puzzle+f.angle),mem:read_i32(f.puzzle+f.shot_x_q8),mem:read_i32(f.puzzle+f.shot_y_q8),
  mem:read_i32(f.puzzle+f.shot_vx_q8),mem:read_i32(f.puzzle+f.shot_vy_q8),panel and 1 or 0,score and 1 or 0))
 buttons(phases[index][3])
 if phases[index][1]=='complete' then
  file:close()
  local out=assert(io.open(q.output..'/stores.json','w'))
  out:write(string.format('{"vram":%d,"palette":%d,"active_vram":%d,"active_palette":%d,"approach_won":%s,"approach_moves":%d}',
   totals.vram,totals.palette,totals.active_vram,totals.active_palette,tostring(approach_won),approach_moves));out:close();m:exit()
 end
end)
