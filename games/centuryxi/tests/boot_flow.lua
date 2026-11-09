-- Century XI under a system ROM: coins, Start and the credits they cost.
-- Run by tests/test_boot_flow.py; CX_SCENARIO picks the script:
--   boot     two coins, Start, then three more Starts in the menu (free)
--   session  one Penalty Challenge, then Start for a second game
--   versus   Two-Player Versus: player 2 drops a coin and presses Start
--   join     an exhibition: player 2 drops a coin and joins mid-match
-- Lines written: "MARK <name> ..." at the moments the runner checks, and
-- "RESET" if the system ROM's RAM test is seen again (the board restarted).
local machine = manager.machine
local mem = machine.devices[':maincpu'].spaces['program']
local screen = machine.screens[':screen']
local out = assert(io.open(os.getenv('CX_RESULT'), 'w'))
local scenario = os.getenv('CX_SCENARIO')
local mvs = os.getenv('CX_PLATFORM') == 'mvs'
local match_on = tonumber(os.getenv('CX_MATCH_ON'), 16)
local marks, done, reset, ended = {}, {}, false, nil
local played = false

local function input(name, value)
    for _, port in pairs(machine.ioport.ports) do
        local field = port.fields[name]
        if field then field:set_value(value and 1 or 0) end
    end
end
local function pulse(t, list)
    for _, s in ipairs(list) do if s and t > s and t < s + 0.2 then return true end end
    return false
end
local function state()
    return string.format('req=%d mode=%d p1=%d p2=%d credit=%02X/%02X on=%d',
        mem:read_u8(0x10FDAE), mem:read_u8(0x10FDAF), mem:read_u8(0x10FDB6), mem:read_u8(0x10FDB7),
        mvs and mem:read_u8(0xD00034) or 0, mvs and mem:read_u8(0xD00035) or 0, mem:read_u8(match_on))
end
local function mark(name)
    if done[name] then return end
    done[name] = true
    out:write('MARK ' .. name .. ' ' .. state() .. '\n')
    out:flush()
end

local coin1, coin2, start1, start2, down, a = {}, {}, {}, {}, {}, nil
if scenario == 'boot' then
    coin1, start1 = {8, 9}, {12, 16, 18, 20}
    marks = {{11, 'before'}, {14, 'paid'}}
elseif scenario == 'session' then
    coin1, start1, down = {8, 9}, {12}, {14, 14.5, 15, 15.5}
    marks = {{11, 'before'}, {14, 'paid'}}
elseif scenario == 'versus' then
    coin1, coin2, start1, down, start2 = {8, 9, 10}, {17.5}, {12}, {14, 14.5, 15}, {19, 23}
    marks = {{18.8, 'before'}}
elseif scenario == 'join' then
    coin1, coin2, start1, down, start2 = {8, 9, 10}, {36}, {12}, {14, 14.5}, {40}
    marks = {{38, 'before'}}
end

emu.register_frame_done(function()
    local t = machine.time:as_double()
    local req = mem:read_u8(0x10FDAE)
    if t > 8 and req > 3 and not reset then reset = true; out:write(string.format('RESET t=%.2f\n', t)) end
    local p1 = mem:read_u8(0x10FDB6)
    if p1 == 1 then played = true end
    if scenario == 'session' and played and not ended and p1 ~= 1 then
        ended = t
        mark('session_end')
    end
    local s1 = pulse(t, start1) or (ended ~= nil and pulse(t, {ended + 6}))
    local s2 = pulse(t, start2)
    input('Coin 1', mvs and pulse(t, coin1))
    input('Coin 2', mvs and pulse(t, coin2))
    input('1 Player Start', s1); input('P1 Start', s1)
    input('2 Players Start', s2); input('P2 Start', s2)
    input('P1 Down', pulse(t, down))
    local press = false
    if scenario == 'session' then
        press = pulse(t, {16.5}) or (not ended and t > 18 and ((t - 18) % 0.7) < 0.12)
    elseif scenario == 'versus' then
        press = pulse(t, {16.5})
    elseif scenario == 'join' then
        press = pulse(t, {16.5}) or (t > 19 and t < 34 and ((t - 19) % 2) < 0.15)
    end
    input('P1 A', press); input('P1 Button 1', press)
    for _, m in ipairs(marks) do if t > m[1] then mark(m[2]) end end
end)

emu.register_stop(function()
    out:write('MARK end ' .. state() .. '\n')
    screen:snapshot(os.getenv('CX_SNAPSHOT'))
    out:close()
end)
