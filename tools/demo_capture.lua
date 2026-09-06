-- Capture the reel through normal coin/start and controller inputs.
local machine = manager.machine
local memory = machine.devices[':maincpu'].spaces['program']
local screen = machine.screens[':screen']
local output = os.getenv('DEMO_CAPTURE_DIR') or '/tmp/neogeo-qa'
local chapter_address = tonumber(os.getenv('DEMO_CHAPTER_ADDRESS'), 16)
local elapsed_address = tonumber(os.getenv('DEMO_ELAPSED_ADDRESS'), 16)
local vram = emu.item(machine.devices[':spritegen'].items['0/m_videoram'])
local pc_item = emu.item(machine.devices[':maincpu'].items['0/m_pc'])
local profile = {}
local chapter, started, next_capture = 0, 0, 0
local frame = 0
local seen = {}
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
end

emu.register_frame_done(function()
    frame = frame + 1
    local now = machine.time:as_double()
    input(':AUDIO_COIN', 'Coin 1', (now >= 7 and now < 7.25) and 1 or 0)
    input(':edge:joy:START', '1 Player Start', (now >= 9 and now < 9.25) and 1 or 0)
    local current = chapter_address and memory:read_u8(chapter_address) or 0
    if current > 25 then current = 0 end
    local pc = pc_item:read(0)
    local key = string.format('%d\t%06x', current, pc)
    profile[key] = (profile[key] or 0) + 1
    if current ~= chapter then
        print(string.format('CHAPTER %02d at %.2fs', current, now))
        chapter, started, next_capture = current, now, now + 0.5
        seen[current] = true
    end
    if now >= next_capture then
        sample(now, current)
        next_capture = now + 2.0
    end
    if (current == 25 and now - started > 4) or (current == 0 and seen[25]) then
        local p = assert(io.open(output .. '/profile.tsv', 'w'))
        for key, count in pairs(profile) do p:write(key .. '\t' .. count .. '\n') end
        p:close()
        report:close()
        machine:exit()
    end
end)
