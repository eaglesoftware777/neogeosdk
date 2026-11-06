-- The Python capture runner supplies paths and compiled symbol addresses.
local config = __SKY_QA_CONFIG__
local machine = manager.machine
local memory = machine.devices[':maincpu'].spaces['program']
local audio = machine.devices[':audiocpu'].spaces['program']
local screen = machine.screens[':screen']
local function cpu_pc(tag)
    local state = machine.devices[tag].state
    local pc = state['CURPC'] or state['PC']
    assert(pc, 'CPU program-counter state unavailable: ' .. tag)
    return pc.value
end
local fields = config.symbols
local log = assert(io.open(config.output .. '/state.csv', 'w'))
log:write('seconds,frame,main_pc,audio_pc,user_mode,credit,player,energy,stage,pose,scroll,slot_wait,timer,protocol,game_frames,queue_peak,lives,invuln,visible,player_x,player_y,hits,vram_pending,vram_duplicates,roll_angle,roll_move,climb_move,special,cores,boss_health,targets,terrain_page,face_x,face_y,encounter,radar_alert,chaff_stock,decoys,travel,fuel,mini_hp\n')
local next_hit = 18
local hits = 0
local frames, next_capture = 0, 0
local queue_peak = 0
local feature_seeded = false
local fuel_seeded = false
local chaff_checkpoint = -1
local ports = assert(io.open(config.output .. '/input-ports.txt', 'w'))
for tag, port in pairs(machine.ioport.ports) do
    for name, field in pairs(port.fields) do ports:write(tag .. ',' .. name .. '\n') end
end
ports:close()
local joystick = machine.ioport.ports[':edge:joy:JOY1'] and ':edge:joy:JOY1' or ':ctrl1:joy:JOY'
local start_port = machine.ioport.ports[':edge:joy:START'] and ':edge:joy:START' or ':ctrl1:joy:START_SELECT'
local start_name = start_port == ':edge:joy:START' and '1 Player Start' or 'P1 Start'

local function input(port, name, value)
    local p = machine.ioport.ports[port]
    if p and p.fields[name] then p.fields[name]:set_value(value) end
end

local function byte(name)
    return memory:read_u8(fields[name])
end

local function pending_groups()
    local seen, total, duplicates = {}, 0, 0
    for _, name in ipairs({'ngsg_high', 'ngsg_norm', 'ngsg_low'}) do
        local count = memory:read_u16(fields[name .. '_n'])
        for i = 0, count-1 do
            local pointer = memory:read_u32(fields[name] + 4*i)
            if seen[pointer] then duplicates = duplicates + 1 end
            seen[pointer] = true
            total = total + 1
        end
    end
    return total, duplicates
end

emu.register_frame_done(function()
    frames = frames + 1
    local now = machine.time:as_double()
    local player = memory:read_u32(fields.s_player)
    local mode = memory:read_u8(0x10fdaf)
    local queued = (byte('sound_queue_head') - byte('sound_queue_tail')) % 32
    queue_peak = math.max(queue_peak, queued)
    input(':AUDIO_COIN', 'Coin 1', (not config.attract_only and ((now > 3 and now < 3.2) or (now > 10 and now < 10.2) or (now > 20 and now < 20.2))) and 1 or 0)
    local start = not config.attract_only and now > 5 and now < 28 and ((now - 5) % 2 < 0.15)
    input(start_port, start_name, start and 1 or 0)
    local phase = now % (config.features and 10 or 6)
    local moving = not config.damage or hits < 3
    local left = config.features and phase < 3.5 or not config.features and phase > 1 and phase < 2
    local right = config.features and phase > 4 and phase < 7 or not config.features and phase > 4 and phase < 5
    input(joystick, 'P1 Left', moving and player ~= 0 and left and 1 or 0)
    input(joystick, 'P1 Right', moving and player ~= 0 and right and 1 or 0)
    input(joystick, 'P1 Up', config.features and player ~= 0 and phase > 7 and phase < 7.7 and 1 or 0)
    input(joystick, 'P1 A', not config.attract_only and now > 11 and
          (not config.boss or player == 0 or config.features) and (math.floor(now * 4) % 2 == 0) and 1 or 0)
    local boost = config.features and ((phase > 2.0 and phase < 2.2) or (phase > 7.1 and phase < 7.3)) or
                  not config.features and config.stress and phase < 3
    input(joystick, 'P1 B', player ~= 0 and boost and 1 or 0)
    local bomb = config.features and phase > 8 and phase < 8.2 or
                 not config.features and config.stress and phase > 3 and phase < 3.15
    input(joystick, 'P1 C', player ~= 0 and bomb and 1 or 0)
    input(joystick, 'P1 D', player ~= 0 and now > 12 and now < 12.15 and 1 or 0)
    if config.encounters and player ~= 0 then
        local p = config.layout
        local tx, ty = 128, 176
        local encounter = byte('s_encounter')
        if config.radar_test and encounter == 2 and byte('s_radar_alert') == 0 then
            tx, ty = 128, 96
        else
            for i=0,p[2]-1 do
                local c=fields.ng_chars+i*p[1]
                if memory:read_u8(c+p[3]) ~= 0 and memory:read_u8(c+p[4]) == p[11] then
                    if encounter == 0 or memory:read_u16(c+p[14]) == 0x8000 then
                        tx=memory:read_u16(c+p[6]); break
                    end
                end
            end
        end
        local px,py=memory:read_u16(player+p[6]),memory:read_u16(player+p[7])
        input(joystick,'P1 Left',px>tx+2 and 1 or 0)
        input(joystick,'P1 Right',px<tx-2 and 1 or 0)
        input(joystick,'P1 Up',py>ty+2 and 1 or 0)
        input(joystick,'P1 Down',py<ty-2 and 1 or 0)
        input(joystick,'P1 A',1)
        input(joystick,'P1 C',0)
        local checkpoint=byte('s_targets_destroyed')
        local release=encounter == 2 and checkpoint ~= chaff_checkpoint and
                      byte('s_radar_alert') == (config.radar_test and 1 or 0)
        input(joystick,'P1 B',release and 1 or 0)
        input(joystick,'P1 D',release and 1 or 0)
        if release and byte('s_chaff_stock') < 6-checkpoint then chaff_checkpoint=checkpoint end
    end
    if config.fuel_out and player ~= 0 and not fuel_seeded then
        memory:write_u8(fields.s_resources+1,2)
        fuel_seeded=true
    end
    if (config.features or config.encounters) and player ~= 0 and not feature_seeded then
        memory:write_u8(fields.s_resources, 4)
        memory:write_u8(fields.s_resources + 1, 240)
        memory:write_u8(fields.s_resources + 3, 3)
        feature_seeded = true
    end
    if config.stage and mode == 2 and player == 0 and now > 5 then
        memory:write_u8(fields.s_stage, config.stage)
    end
    if player == 0 and mode == 2 then memory:write_u8(fields.s_pick, config.pilot or 0) end
    if config.boss and player ~= 0 then
        memory:write_u8(fields.s_waves_left, 0)
        memory:write_u8(fields.s_squad_left, 0)
        memory:write_u16(fields.s_wave_timer, 0)
    end
    if config.stress and player ~= 0 then
        memory:write_u8(fields.s_resources, 4)
        memory:write_u8(fields.s_resources + 1, 240)
        memory:write_u8(fields.s_resources + 3, 3)
    end
    if config.damage and player ~= 0 and now >= next_hit and hits < 3 then
        local p = config.layout
        for i = 0, p[2]-1 do
            local c = fields.ng_chars + i*p[1]
            local kind = memory:read_u8(c+p[4])
            if memory:read_u8(c+p[3]) ~= 0 and (kind == p[11] or kind == p[12]) then
                memory:write_u8(fields.s_invuln, 0)
                memory:write_u8(fields.s_energy, hits == 0 and 100 or 15)
                memory:write_u8(fields.s_resources + 2, 0)
                local x, y = memory:read_u16(player+p[6]), memory:read_u16(player+p[7])
                memory:write_u16(c+p[6], x)
                memory:write_u16(c+p[7], y)
                memory:write_u32(c+p[8], x*256)
                memory:write_u32(c+p[9], y*256)
                next_hit = now + 6
                hits = hits + 1
                break
            end
        end
    end
    if config.stage and player ~= 0 and not config.damage then
        -- Inspection runs keep the craft alive to reach each sector's boss.
        memory:write_u8(fields.s_invuln, 120)
    end
    if now >= next_capture then
        local name = config.output .. '/t' .. string.format('%05.1f', now):gsub(',', '.')
        screen:snapshot(name .. '.png')
        local protocol = ''
        for i = 0, 3 do protocol = protocol .. string.format('%02x', audio:read_u8(0xfe44 + i)) end
        local pending, duplicates = pending_groups()
        local face = memory:read_u32(fields.s_hud_face)
        local mini_hp = 0
        if config.layout then
            local p = config.layout
            for i=0,p[2]-1 do
                local c=fields.ng_chars+i*p[1]
                if memory:read_u8(c+p[3]) ~= 0 and memory:read_u8(c+p[4]) == p[11] and
                   memory:read_u16(c+p[13]) == 5 then mini_hp=memory:read_u8(c+p[16]) end
            end
        end
        local angle = memory:read_u16(fields.s_roll)
        if angle >= 32768 then angle = angle - 65536 end
        log:write(string.format('%s,%d,%x,%x,%d,%d,%x,%d,%d,%d,%d,%d,%d,%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n',
            string.format('%.3f', now):gsub(',', '.'), frames, cpu_pc(':maincpu'),
            cpu_pc(':audiocpu'), mode,
            memory:read_u8(0x10fd00), player, byte('s_energy'), byte('s_stage'),
            byte('s_plane_pose'), math.floor(memory:read_u32(fields.s_camera) / 256) % 512,
            audio:read_u8(0xfe42), audio:read_u8(0xf927), protocol,
            memory:read_u16(fields.s_frames), queue_peak, byte('s_lives'), byte('s_invuln'),
            player ~= 0 and memory:read_u8(player+4) or 0,
            player ~= 0 and memory:read_u16(player+6) or 0,
            player ~= 0 and memory:read_u16(player+8) or 0, hits, pending, duplicates, angle, byte('s_roll_maneuver'), byte('s_loop_maneuver'),
            memory:read_u8(fields.s_resources+8), memory:read_u8(fields.s_resources+3),
            memory:read_u16(fields.s_boss_health), byte('s_targets_destroyed'),
            math.max(memory:read_u8(fields.s_ground_page), memory:read_u8(fields.s_ground_page+1)),
            face ~= 0 and memory:read_u16(face+6) or 0,
            face ~= 0 and memory:read_u16(face+8) or 0, byte('s_encounter'), byte('s_radar_alert'),
            byte('s_chaff_stock'), memory:read_u16(fields.s_decoyed_missiles),
            math.floor(memory:read_u32(fields.s_camera)/256), memory:read_u8(fields.s_resources+1), mini_hp))
        log:flush()
        next_capture = now + ((config.features or config.encounters or config.fuel_out) and .25 or config.damage and now >= 18 and now <= 34 and .25 or 2)
    end
end)

local palette_log=assert(io.open(config.output .. '/hardware.csv','w'))
palette_log:write('seconds,all_dirty,level,half_done,reference,terrain_color,peak_strips,irq_lines,irq_count,cloud_x,cloud_y,camera_x\n')
local sprite=machine.devices[':spritegen']
local ram
for name,id in pairs(sprite.items) do
 if name:find('m_videoram') and not name:find('drawsource') then ram=emu.item(id);break end
end
assert(ram,'Sprite save-item RAM unavailable')
local function strip_count()
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
emu.register_frame_done(function()
 local base=fields.ng_palfx_scr
 local cloud=memory:read_u32(fields.s_overhead_cloud)
 local x,y=-32768,-32768
 if cloud~=0 then x=memory:read_i16(cloud+config.layout[6]);y=memory:read_i16(cloud+config.layout[7]) end
 local irq=fields.s_palette_irq_count and memory:read_u16(fields.s_palette_irq_count) or 0
 local lines=fields.s_palette_irq_lines and memory:read_u8(fields.s_palette_irq_lines) or 0
 palette_log:write(string.format('%.4f,%d,%d,%d,%04x,%04x,%d,%d,%d,%d,%d,%d\n',machine.time:as_double(),memory:read_u8(base+13),memory:read_u8(base+10),memory:read_u8(base+14),memory:read_u16(0x400000),memory:read_u16(0x400222),strip_count(),lines,irq,x,y,memory:read_i32(fields.s_camera+4)))
 palette_log:flush()
end)
