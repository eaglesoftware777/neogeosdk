-- Passive live audio capture: the demo boots and runs on its own CPUs.
--
-- Nothing is parked or injected.  Every byte the 68000 writes to the sound
-- latch is logged with its program counter, every YM write on the ADPCM-B,
-- ADPCM-A level and fade-relevant registers is logged, chapter changes are
-- logged, and MAME's -wavwrite records the real mix.  Optionally presses A
-- on a schedule to walk the reel the way a player would.
local machine = manager.machine
local main = machine.devices[':maincpu']
local memory = main.spaces['program']
local z80 = machine.devices[':audiocpu']
local output = assert(os.getenv('DEMO_AUDIO_DIR'))
local chapter_address = tonumber(os.getenv('DEMO_CHAPTER_ADDRESS'), 16)
local press_every = tonumber(os.getenv('DEMO_AUDIO_PRESS_EVERY') or '0')
local stop_at = tonumber(os.getenv('DEMO_AUDIO_STOP_CHAPTER') or '26')
local extra_coin = tonumber(os.getenv('DEMO_AUDIO_COIN_AT') or '0')
local pc_item = emu.item(main.items['0/m_pc'])
local log = assert(io.open(output .. '/events.tsv', 'w'))
log:write('time\tkind\tvalue\tpc\n')
local latch = {0, 0}
local chapter, started = 0, 0

-- The tap handles must stay referenced or the collector removes the taps.
taps = {}
taps.ym = z80.spaces['io']:install_write_tap(0, 0xffff, 'ym', function(offset, data)
    local port = offset & 255
    if port == 4 or port == 6 then
        latch[(port - 4) // 2 + 1] = data
    elseif port == 5 or port == 7 then
        local bank = (port - 5) // 2
        local reg = latch[bank + 1]
        if (bank == 0 and reg >= 0x10 and reg <= 0x1c) or (bank == 1 and reg == 0x01) then
            log:write(string.format('%.4f\tym%d\t%02x=%02x\t\n',
                machine.time:as_double(), bank, reg, data))
        end
    end
end)
if os.getenv('DEMO_AUDIO_TRACE_Z80') == '1' then
    -- Queue-level trace: what the NMI enqueued, what the main loop dequeued,
    -- and the parameter-wait state, so a lost or reordered byte is visible.
    local zram = z80.spaces['program']
    taps.fifo_w = zram:install_write_tap(0xf800, 0xf821, 'fifo-w', function(offset, data)
        local what = offset == 0xf820 and 'rd_ptr' or offset == 0xf821 and 'wr_ptr'
                     or string.format('enq[%02x]', offset & 31)
        log:write(string.format('%.4f\tz80\t%s=%02x\t\n', machine.time:as_double(), what, data))
    end)
    taps.fifo_r = zram:install_read_tap(0xf800, 0xf81f, 'fifo-r', function(offset, data)
        log:write(string.format('%.4f\tz80\tdeq[%02x]=%02x\t\n', machine.time:as_double(), offset & 31, data))
    end)
    taps.param = zram:install_write_tap(0xfe06, 0xfe0b, 'param', function(offset, data)
        if offset == 0xfe06 or offset == 0xfe0b then
            log:write(string.format('%.4f\tz80\t%s=%02x\t\n', machine.time:as_double(),
                offset == 0xfe06 and 'wait_kind' or 'param_mode', data))
        end
    end)
end
taps.latch = memory:install_write_tap(0x320000, 0x320001, 'latch', function(offset, data, mask)
    local byte = (mask & 0xff00) ~= 0 and (data >> 8) or (data & 0xff)
    log:write(string.format('%.4f\tcmd\t%02x\t%06x\n',
        machine.time:as_double(), byte, pc_item:read(0)))
end)

local function input(port, name, value)
    local p = machine.ioport.ports[port]
    if p and p.fields[name] then p.fields[name]:set_value(value) end
end

emu.register_frame_done(function()
    local now = machine.time:as_double()
    -- One coin to start with, and optionally another mid-reel: a coin cue
    -- must land over the music, not replace it.
    input(':AUDIO_COIN', 'Coin 1', ((now >= 7 and now < 7.25) or
        (extra_coin > 0 and now >= extra_coin and now < extra_coin + 0.25)) and 1 or 0)
    input(':edge:joy:START', '1 Player Start', (now >= 9 and now < 9.25) and 1 or 0)
    local current = memory:read_u8(chapter_address)
    if current > 26 then current = 0 end
    if current ~= chapter then
        chapter, started = current, now
        log:write(string.format('%.4f\tchapter\t%d\t\n', now, current))
        log:flush()
    end
    local a = 0
    if press_every > 0 and chapter > 0 and now - started >= press_every then
        a = ((now - started - press_every) < 0.25) and 1 or 0
    end
    input(':edge:joy:JOY1', 'P1 A', a)
    if chapter == stop_at and now - started > 3 then
        log:close()
        machine:exit()
    end
end)
