-- Boot a game through the real coin/start path and snapshot it at intervals.
--
-- Unlike tools/demo_capture.lua this knows nothing about a chapter index, so
-- it works for any game in the tree.  Drives a coin and 1P start, then holds
-- a gentle input pattern so a game that waits for the stick still moves.
local machine = manager.machine
local screen = machine.screens[':screen']
local memory = machine.devices[':maincpu'].spaces['program']
local vram = emu.item(machine.devices[':spritegen'].items['0/m_videoram'])
local output = os.getenv('GAME_CAPTURE_DIR') or '/tmp/neogeo-qa'
local every = tonumber(os.getenv('GAME_CAPTURE_EVERY') or '2.0')
local play = os.getenv('GAME_CAPTURE_PLAY') ~= '0'
local frame, next_capture = 0, 0

local function input(port, name, value)
    local p = machine.ioport.ports[port]
    if p and p.fields[name] then p.fields[name]:set_value(value) end
end

emu.register_frame_done(function()
    frame = frame + 1
    local now = machine.time:as_double()

    -- Coin and start are retried for the first twenty seconds.  A title
    -- sequence that is still running its own animation swallows the first
    -- press, and one press is then the difference between capturing a game
    -- and capturing its attract loop for the whole run.
    local function pulse(base, period, width)
        if now < base or now > 20 then return 0 end
        return (((now - base) % period) < width) and 1 or 0
    end
    input(':AUDIO_COIN', 'Coin 1', pulse(4, 3.0, 0.25))
    input(':edge:joy:START', '1 Player Start', pulse(6, 3.0, 0.25))
    input(':edge:joy:JOY1', 'P1 A', pulse(7, 3.0, 0.25))

    if play and now > 7 then
        -- A slow sweep plus a held fire button: enough to leave a title
        -- screen, pick a option and make something happen on the field.
        local phase = (now - 7) % 8.0
        input(':edge:joy:JOY1', 'P1 Left',  (phase > 1.0 and phase < 2.5) and 1 or 0)
        input(':edge:joy:JOY1', 'P1 Right', (phase > 4.0 and phase < 5.5) and 1 or 0)
        input(':edge:joy:JOY1', 'P1 Up',    (phase > 6.0 and phase < 6.6) and 1 or 0)
        input(':edge:joy:JOY1', 'P1 A', (math.floor(now * 6) % 2 == 0) and 1 or 0)
        input(':edge:joy:JOY1', 'P1 B', (math.floor(now * 4) % 2 == 0) and 1 or 0)
    end

    if now >= next_capture then
        local name = string.format('%s/t%06.1f_%06d', output, now, frame)
        screen:snapshot(name .. '.png')
        -- Keep the actual SCB tile attributes and palette RAM with the image.
        -- A correct ROM preview alone cannot catch a missing runtime binding.
        local state = assert(io.open(name .. '.vram.bin', 'wb'))
        for word = 0, 0x87ff do state:write(string.pack('>I2', vram:read(word))) end
        state:close()
        local palette = assert(io.open(name .. '.palette.bin', 'wb'))
        for word = 0, 4095 do palette:write(string.pack('>I2', memory:read_u16(0x400000 + word * 2))) end
        palette:close()
        next_capture = now + every
    end
end)
