-- Read-only native SCB and game-state inspection; inputs/inspection seeds are explicit.
local machine = manager.machine
local memory = machine.devices[':maincpu'].spaces['program']
local audio = machine.devices[':audiocpu'].spaces['program']
local screen = machine.screens[':screen']
local fields = config.symbols
local log = assert(io.open(config.output .. '/state.csv', 'w'))
log:write('seconds,frame,main_pc,audio_pc,user_mode,credit,player,energy,stage,pose,scroll,slot_wait,timer,protocol,game_frames,queue_peak,lives,invuln,visible,player_x,player_y,hits,vram_pending,vram_duplicates,roll_angle,roll_move,climb_move,special,cores,boss_health,targets,terrain_page,face_x,face_y,encounter,radar_alert,chaff_stock,decoys,travel,fuel,mini_hp\n')
local vram=emu.item(machine.devices[':spritegen'].items['0/m_videoram'])
local metrics=assert(io.open(config.output..'/depth.csv','w'))
metrics:write('seconds,video_frame,game_frame,sortie_frame,raw_scb_peak,estimated_peak,camera_x,shadow_groups,cloud_slot,player_slot,min_bullet_slot,max_bullet_slot,modules,terrain_x,road_x,shadow_drops,wrecks,terrain_x2,under_x\n')
local next_hit = 18
local hits = 0
local frames, next_capture = 0, 0
local queue_peak = 0
local feature_seeded = false
local fuel_seeded = false
local chaff_checkpoint = -1
-- Optional: knock out the Crimson Keep's turrets one at a time (left after
-- 8 s of the fight, right after 16 s) and log where each new enemy shot
-- starts relative to the hull.
local turret_log = config.silence and assert(io.open(config.output .. '/turrets.csv', 'w'))
if turret_log then turret_log:write('seconds,phase,dx,dy\n') end
local turret_shots, boss_seen_at = {}, nil
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
    input(':AUDIO_COIN', 'Coin 1', (not config.attract_only and now > 3 and now < 3.2) and 1 or 0)
    local start = not config.attract_only and now > 5 and now < 18 and ((now - 5) % 2 < 0.15)
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
        memory:write_u32(fields.s_camera,3840*256)
        memory:write_u8(fields.s_waves_left, 0)
        memory:write_u8(fields.s_squad_left, 0)
        memory:write_u16(fields.s_wave_timer, 0)
    end
    if turret_log and player ~= 0 then
        local p = config.layout
        local boss
        for i = 0, p[2]-1 do
            local c = fields.ng_chars + i*p[1]
            if memory:read_u8(c+p[3]) ~= 0 and memory:read_u8(c+p[4]) == 3 then boss = c end
        end
        if boss then
            boss_seen_at = boss_seen_at or now
            local phase = now - boss_seen_at < 8 and 0 or now - boss_seen_at < 16 and 1 or 2
            local bx, by = memory:read_u16(boss+p[6]), memory:read_u16(boss+p[7])
            for i = 0, p[2]-1 do
                local c = fields.ng_chars + i*p[1]
                local kind = memory:read_u8(c+p[4])
                if memory:read_u8(c+p[3]) ~= 0 and kind == 14 and memory:read_u16(c+p[13]) < phase then
                    memory:write_u8(c+p[16], 0)
                elseif memory:read_u8(c+p[3]) == 0 or kind ~= p[12] then
                    turret_shots[i] = nil
                end
                if memory:read_u8(c+p[3]) ~= 0 and kind == p[12] then
                    -- A slot's shot is logged once, while it is young.
                    local age = memory:read_u16(c+p[13]+2)
                    if age > 2 then turret_shots[i] = nil end
                    if age <= 1 and not turret_shots[i] then
                        turret_shots[i] = true
                        local x, y = memory:read_u16(c+p[6]), memory:read_u16(c+p[7])
                        if x >= 32768 then x = x - 65536 end
                        if y >= 32768 then y = y - 65536 end
                        turret_log:write(string.format('%.3f,%d,%d,%d\n', now, phase, x - bx, y - by))
                        turret_log:flush()
                    end
                end
            end
        end
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
            string.format('%.3f', now):gsub(',', '.'), frames, machine.devices[':maincpu'].state['PC'].value,
            machine.devices[':audiocpu'].state['PC'].value, mode,
            memory:read_u8(0x10fd00), player, byte('s_energy'), byte('s_stage'),
            memory:read_u16(fields.s_plane_pose), math.floor(memory:read_u32(fields.s_camera) / 256) % 512,
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

        local edges={};for y=0,224 do edges[y]=0 end
        local baseline,y,rows=0,0,0
        local function span(a,z)
            if a>=224 then return end;z=math.min(224,z)
            if a<z then edges[a]=edges[a]+1;edges[z]=edges[z]-1 end
        end
        for id=0,380 do
            local control=vram:read(0x8200+id)
            if (control&0x40)==0 then y=(512-(control>>7))&511;rows=control&63 end
            if rows>=32 then baseline=baseline+1
            elseif rows>0 then local stop=y+rows*16;span(y,stop);if stop>512 then span(0,stop-512)end end
        end
        local count,peak=baseline,baseline;for line=0,223 do count=count+edges[line];peak=math.max(peak,count)end
        local shadows=0;for _,slot in ipairs({45,48,53,58,63,68})do if(vram:read(0x8200+slot)&63)>0 then shadows=shadows+1 end end
        local cloud_slot,player_slot,min_bullet,max_bullet,modules,road_x,wrecks=-1,-1,999,-1,0,-999,0
        local under_x=-999
        for i=0,63 do
            local c=fields.ng_chars+i*108
            if memory:read_u8(c)~=0 then
                local kind=memory:read_u8(c+1);local slot=memory:read_u16(c+32)
                local loaded=slot<381 and (vram:read(0x8200+slot)&63)>0 and vram:read(slot*64)==memory:read_u16(c+34)
                if kind==1 and loaded then player_slot=slot
                elseif (kind==4 or kind==5) and loaded then min_bullet=math.min(min_bullet,slot);max_bullet=math.max(max_bullet,slot)
                elseif kind==15 and loaded then cloud_slot=slot
                elseif kind==14 then modules=modules+1
                elseif kind==16 and loaded then wrecks=wrecks+1
                elseif kind==11 and loaded then
                    local x=(vram:read(0x8400+slot)>>7)&511;if x>=256 then x=x-512 end
                    local offset=memory:read_u16(c+50);if offset>=32768 then offset=offset-65536 end
                    road_x=x-offset
                    -- the ground page under the car (slot 1 or 23, 256 rows each)
                    local cy=memory:read_u16(c+8);if cy>=32768 then cy=cy-65536 end
                    for _,ps in ipairs({1,23}) do
                        local py=(512-(vram:read(0x8200+ps)>>7))&511
                        if ((cy-py)&511)<256 then
                            under_x=(vram:read(0x8400+ps)>>7)&511;if under_x>=256 then under_x=under_x-512 end
                        end
                    end
                end
            end
        end
        local cam=memory:read_u32(fields.s_camera+4);if cam>=0x80000000 then cam=cam-0x100000000 end
        local terrain=(vram:read(0x8401)>>7)&511;if terrain>=256 then terrain=terrain-512 end
        -- the ground layer's second page (slot 23): one of the two is recycled above the screen
        local terrain2=(vram:read(0x8417)>>7)&511;if terrain2>=256 then terrain2=terrain2-512 end
        metrics:write(string.format('%.3f,%d,%d,%d,%d,%d,%.3f,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n',now,frames,memory:read_u16(fields.s_frames),memory:read_u16(fields.s_sortie_frames),peak,memory:read_u16(fields.s_depth_strip_peak),cam/256,shadows,cloud_slot,player_slot,min_bullet,max_bullet,modules,terrain,road_x,memory:read_u16(fields.s_depth_shadow_drops),wrecks,terrain2,under_x));metrics:flush()

        next_capture = now + ((config.features or config.encounters or config.fuel_out) and .25 or config.damage and now >= 18 and now <= 34 and .25 or 2)
    end
end)
