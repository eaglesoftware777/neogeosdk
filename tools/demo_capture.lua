-- Capture the reel through normal coin/start and controller inputs.
local machine = manager.machine
local memory = machine.devices[':maincpu'].spaces['program']
local screen = machine.screens[':screen']
local output = os.getenv('DEMO_CAPTURE_DIR') or '/tmp/neogeo-qa'
local chapter_address = tonumber(os.getenv('DEMO_CHAPTER_ADDRESS'), 16)
local elapsed_address = tonumber(os.getenv('DEMO_ELAPSED_ADDRESS'), 16)
local restart_address = tonumber(os.getenv('DEMO_RESTART_ADDRESS'), 16)
local vram = emu.item(machine.devices[':spritegen'].items['0/m_videoram'])
local pc_item = emu.item(machine.devices[':maincpu'].items['0/m_pc'])
local profile = {}
local chapter, started, next_capture = 0, 0, 0
local frame = 0
local seen = {}
local controls = os.getenv('DEMO_CAPTURE_CONTROLS') == '1'
local focus = tonumber(os.getenv('DEMO_CAPTURE_CHAPTER')) or 0
local interval = tonumber(os.getenv('DEMO_CAPTURE_INTERVAL')) or 2
local control_phase, control_time, previous_elapsed = 0, 0, 0
local control_report = assert(io.open(output .. '/controls.tsv', 'w'))
control_report:write('chapter\taction\ttime\n')
local report = assert(io.open(output .. '/frames.tsv', 'w'))
report:write('time\tchapter\telapsed\tvisible_strips\tmax_scanline_strips\n')

local function input(port, name, value)
    local p = machine.ioport.ports[port]
    if p and p.fields[name] then p.fields[name]:set_value(value) end
end

local function sample(now, current)
    local counts, total = {}, 0
    local y, height = 0, 0
    for slot = 1, 381 do
        local scb3 = vram:read(0x8200 + slot)
        if (scb3 & 0x40) == 0 then
            y = (496 - (scb3 >> 7)) & 511
            height = scb3 & 63
        end
        if height > 0 then
            total = total + 1
            for line = 0, 223 do
                if ((line - y) & 511) < height * 16 then
                    counts[line] = (counts[line] or 0) + 1
                end
            end
        end
    end
    local peak = 0
    for _, count in pairs(counts) do peak = math.max(peak, count) end
    local elapsed = elapsed_address and memory:read_u16(elapsed_address) or 0
    report:write(string.format('%.3f\t%d\t%d\t%d\t%d\n', now, current, elapsed, total, peak))
    report:flush()
    screen:snapshot(string.format('%s/ch%02d_%06d.png', output, current, frame))
    local state = assert(io.open(string.format('%s/ch%02d_%06d.tsv', output, current, frame), 'w'))
    state:write('slot\tscb2\tscb3\tscb4\ttile\tattr\n')
    for slot = 1, 381 do
        local scb3 = vram:read(0x8200 + slot)
        if (scb3 & 63) > 0 then
            state:write(string.format('%d\t%04x\t%04x\t%04x\t%04x\t%04x\n', slot,
                vram:read(0x8000 + slot), scb3, vram:read(0x8400 + slot),
                vram:read(slot * 64), vram:read(slot * 64 + 1)))
        end
    end
    state:close()
    local palette = assert(io.open(string.format('%s/ch%02d_%06d.palette.bin', output, current, frame), 'wb'))
    for word = 0, 4095 do palette:write(string.pack('>I2', memory:read_u16(0x400000 + word * 2))) end
    palette:close()
    local raw = assert(io.open(string.format('%s/ch%02d_%06d.vram.bin', output, current, frame), 'wb'))
    for word = 0, 0x87ff do raw:write(string.pack('>I2', vram:read(word))) end
    raw:close()
end

emu.register_frame_done(function()
    frame = frame + 1
    local now = machine.time:as_double()
    input(':AUDIO_COIN', 'Coin 1', (now >= 7 and now < 7.25) and 1 or 0)
    input(':edge:joy:START', '1 Player Start', (now >= 9 and now < 9.25) and 1 or 0)
    local current = chapter_address and memory:read_u8(chapter_address) or 0
    if current > 26 then current = 0 end
    -- run_chapter temporarily rewinds the view index during a restart.
    -- Wait for the header to publish its original index before assessing it.
    if controls and (control_phase == 1 or control_phase == 2) and current == chapter - 1 then
        input(':edge:joy:JOY1', 'P1 C', 0)
        return
    end
    local pc = pc_item:read(0)
    local key = string.format('%d\t%06x', current, pc)
    profile[key] = (profile[key] or 0) + 1
    if current ~= chapter then
        if controls and chapter > 0 then
            control_report:write(string.format('%d\t%s\t%.3f\n', chapter,
                control_phase == 4 and 'A_PASS' or 'UNEXPECTED_ADVANCE', now))
            control_report:flush()
        end
        print(string.format('CHAPTER %02d at %.2fs', current, now))
        chapter, started, next_capture = current, now, now + 0.5
        control_phase, control_time, previous_elapsed = 0, now, 0
        seen[current] = true
    end
    if focus > 0 then
        input(':edge:joy:JOY1', 'P1 A',
            current ~= focus and now - started >= 2 and ((now - started) % 2) < 0.25 and 1 or 0)
    end
    if controls and current > 0 then
        local elapsed = memory:read_u16(elapsed_address)
        local a, c = 0, 0
        if control_phase == 0 and now - started >= 2 and elapsed >= 90 then
            if current == 18 and restart_address and memory:read_u8(restart_address) == 0 then
                control_phase, control_time = 2, now
                control_report:write(string.format('%d\tC_RESERVED\t%.3f\n', current, now))
                control_report:flush()
            else
                control_phase, control_time = 1, now
            end
        elseif control_phase == 1 and elapsed < previous_elapsed then
            control_phase, control_time = 2, now
            control_report:write(string.format('%d\tC_PASS\t%.3f\n', current, now))
            control_report:flush()
        elseif control_phase == 1 and now - control_time > 8 then
            control_report:write(string.format('%d\tC_FAIL\t%.3f\n', current, now))
            control_phase, control_time = 2, now
        elseif control_phase == 2 and elapsed >= (current == 24 and 480 or current == 18 and 300 or 110) then
            control_phase, control_time = 4, now
        end
        if control_phase == 1 and now - control_time < 0.25 then c = 1 end
        if control_phase == 4 and ((now - control_time) % 2) < 0.25 then a = 1 end
        input(':edge:joy:JOY1', 'P1 A', a)
        input(':edge:joy:JOY1', 'P1 C', c)
        -- Exercise movement and firing after restart, including the mini shooter.
        local play = control_phase == 2 and (current == 17 or current == 18 or current == 21 or current == 24 or current == 25)
        if play and current == 18 then
            input(':edge:joy:JOY1', 'P1 C', (elapsed % 90 < 10) and 1 or 0)
        end
        input(':edge:joy:JOY1', 'P1 B', play and (elapsed % 30 < 15) and 1 or 0)
        input(':edge:joy:JOY1', 'P1 Left', play and (elapsed % 120 < 40) and 1 or 0)
        input(':edge:joy:JOY1', 'P1 Right', play and (elapsed % 120 >= 60 and elapsed % 120 < 100) and 1 or 0)
        previous_elapsed = elapsed
    end
    if now >= next_capture and (focus == 0 or current == focus) then
        sample(now, current)
        next_capture = now + interval
    end
    if (focus > 0 and seen[focus] and current ~= focus) or
       (current == 26 and (not controls and now - started > 4 or controls and control_phase == 4)) or (current == 0 and seen[26]) then
        local p = assert(io.open(output .. '/profile.tsv', 'w'))
        for key, count in pairs(profile) do p:write(key .. '\t' .. count .. '\n') end
        p:close()
        report:close()
        control_report:close()
        machine:exit()
    end
end)
