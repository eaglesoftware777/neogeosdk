-- Shadow League headless run: coin, start, pick, fight with scripted input;
-- snapshots and match telemetry. Reads the cartridge's RAM only.
local machine = manager.machine
local screen = machine.screens[':screen']
local memory = machine.devices[':maincpu'].spaces['program']
local output = assert(os.getenv('SLF_CAPTURE_DIR'))
local a = dofile(output .. '/layout.lua')
local aes = os.getenv('SLF_PLATFORM') == 'aes'
local team = os.getenv('SLF_TEAM') == '1'
local log = assert(io.open(output .. '/telemetry.jsonl', 'w'))
local frame, next_shot = 0, 0
local aliases = {
    ['P1 Button 1'] = {'P1 A', 'P1 Button A', 'P1 Button 1'},
    ['P1 Button 2'] = {'P1 B', 'P1 Button B', 'P1 Button 2'},
    ['P1 Button 3'] = {'P1 C', 'P1 Button C', 'P1 Button 3'},
    ['P1 Button 4'] = {'P1 D', 'P1 Button D', 'P1 Button 4'},
}
local bindings = {}
local function binding(name)
    if bindings[name] ~= nil then return bindings[name] end
    for _, candidate in ipairs(aliases[name] or {name}) do
        for _, port in pairs(machine.ioport.ports) do
            local field = port.fields[candidate]
            if field then bindings[name] = field; return field end
        end
    end
    bindings[name] = false
    return false
end
local function input(name, value)
    local field = binding(name)
    if field then field:set_value(value and 1 or 0) end
end
local function u8(off) return memory:read_u8(a.match + off) end
local function u16(off) return memory:read_u16(a.match + off) end
local function s32(off)
    local v = memory:read_u32(a.match + off)
    return v >= 0x80000000 and v - 0x100000000 or v
end
local function max_life(off)
    local pointer = memory:read_u32(a.match + off)
    return pointer == 0 and 0 or memory:read_u16(pointer + a.def_life)
end
local P1 = {'P1 Up', 'P1 Down', 'P1 Left', 'P1 Right', 'P1 Button 1', 'P1 Button 2', 'P1 Button 3', 'P1 Button 4'}
for _, name in ipairs(P1) do assert(binding(name), "Missing input: " .. name) end
assert(binding("P1 Start") or binding("1 Player Start"), "Missing start input")
if not aes then assert(binding("Coin 1"), "Missing coin input") end
local function pad(bits)
    for i, name in ipairs(P1) do input(name, (bits >> (i - 1)) & 1 == 1) end
end
-- A short repertoire: walk in, jab, kick, quarter-circle specials, a jump-in.
local moves = {
    {8, 8, 8, 8, 0}, {16, 0}, {8, 8, 32, 0}, {64, 0, 0}, {2, 2, 2 | 32, 0},
    {2, 2 | 8, 8 | 16, 0}, {2, 2 | 8, 8 | 64, 0}, {9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 128, 0},
    {16 | 64, 0}, {4, 4, 4, 4, 0}, {2, 2 | 128, 0}, {8, 8, 8, 64, 0},
}
local seq, pos, hold, next_move = nil, 1, 0, 1
emu.register_frame_done(function()
    local t = machine.time:as_double()
    frame = frame + 1
    if not aes then input('Coin 1', t > 8 and t < 8.25) end
    local st = (t > 12 and t < 12.25)
    input('1 Player Start', st)
    input('P1 Start', st)
    if team and t > 14 and t < 14.25 then
        pad(8)
    elseif t > 16 and u8(a.round) == 0 then
        local tap = (math.floor(t * 2) % 2 == 0)
        pad(tap and 16 or 0)
    elseif u8(a.round) > 0 then
        if hold > 0 then hold = hold - 1 else
            if not seq then seq = moves[next_move]; next_move = next_move % #moves + 1; pos = 1 end
            pad(seq[pos]); pos = pos + 1; hold = 1
            if pos > #seq then seq = nil end
        end
    end
    if t >= next_shot then
        screen:snapshot(string.format('%s/shot_%03d.png', output, math.floor(t)))
        next_shot = next_shot + (t < 40 and 4 or 3)
    end
    if frame % 30 == 0 then
        log:write(string.format('{"t":%.2f,"state":%d,"round":%d,"secs":%d,"life0":%d,"life1":%d,' ..
            '"x0":%d,"x1":%d,"st0":%d,"st1":%d,"wins0":%d,"wins1":%d,"winner":%d,"frame":%d,"mode":%d,"active0":%d,"active1":%d,"count0":%d,"count1":%d,"assist0":%d,"assist1":%d,"maxlife0":%d,"maxlife1":%d,"joy0":%d,"cpu0":%d,"cpu1":%d}\n',
            t, u8(a.state), u8(a.round), u8(a.seconds), u16(a.life0), u16(a.life1), s32(a.x0) // 256,
            s32(a.x1) // 256, u8(a.st0), u8(a.st1), u8(a.wins0), u8(a.wins1), u8(a.winner),
            memory:read_u32(a.match + a.frame), u8(a.mode), u8(a.active0), u8(a.active1),
            u8(a.count0), u8(a.count1), u8(a.assist0), u8(a.assist1),
            max_life(a.def0), max_life(a.def1), memory:read_u8(0x10FD96), u8(a.cpu0), u8(a.cpu1)))
        log:flush()
    end
end)
