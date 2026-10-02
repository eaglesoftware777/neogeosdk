-- MAME-only stage selection. The cartridge contains no test shortcuts.
local machine = manager.machine
local memory = machine.devices[':maincpu'].spaces['program']
local screen = machine.screens[':screen']
local layout = dofile(assert(os.getenv('MG_V3_LAYOUT')))
local mode = os.getenv('MG_V3_MODE') or 'stage'
local target = tonumber(os.getenv('MG_V3_STAGE') or '0') or 0
if mode == 'ending' then target = 11 end
local placed = false
local ready_frames = 0
local next_diag = 5
local end_pending = false

local function input(name, value)
    for _, port in pairs(machine.ioport.ports) do
        local field = port.fields[name]
        if field then field:set_value(value and 1 or 0) end
    end
end

emu.register_frame_done(function()
    local t = machine.time:as_double()
    local begin = mode ~= 'title' and mode ~= 'attract'
    input('Coin 1', begin and t > 8 and t < 8.25)
    input('1 Player Start', begin and t > 12 and t < 12.25)
    input('P1 Start', begin and t > 12 and t < 12.25)

    local player = memory:read_u32(layout.player)
    local in_game = player >= 0x100000 and player <= 0x10efff
    if os.getenv('MG_V3_DIAG') and t >= next_diag then
        print(string.format('V3 t=%.1f player=%08x state=%d stage=%d demo=%d',
            t, player, memory:read_u8(layout.state), memory:read_u8(layout.stage),
            memory:read_u8(layout.demo)))
        local snapshot = os.getenv('MG_V3_SNAPSHOT')
        if snapshot then screen:snapshot(snapshot) end
        next_diag = next_diag + 5
    end
    local advance = mode ~= 'title' and mode ~= 'attract' and mode ~= 'select'
    input('P1 A', advance and not in_game and t > 13 and ((t - 13) % 0.75) < 0.15)

    if not placed and in_game and memory:read_u8(layout.demo) == 0 and
            memory:read_u8(layout.state) == 1 then
        placed = true
        if (mode == 'stage' or mode == 'transition' or mode == 'ending') and
                target > 0 and target < 12 then
            memory:write_u8(layout.next_stage, target)
            memory:write_u8(layout.state, 8)
            memory:write_u16(layout.state_timer, 1)
            end_pending = mode == 'ending'
        elseif mode == 'bonus' then
            memory:write_u8(layout.stage, 1)
            memory:write_u8(layout.state, 2)
            memory:write_u16(layout.state_timer, 1)
        end
    end
    if end_pending and in_game and memory:read_u8(layout.state) == 1 and
            memory:read_u8(layout.stage) == 11 then
        memory:write_u8(layout.state, 2)
        memory:write_u16(layout.state_timer, 2)
        end_pending = false
        print('V3 ending sequence ready')
    end
    if placed and in_game and memory:read_u8(layout.state) == 1 and
            memory:read_u8(layout.stage) == target then
        ready_frames = ready_frames + 1
        if ready_frames == 30 then
            print(string.format('V3 stage %d ready', target + 1))
            local snapshot = os.getenv('MG_V3_SNAPSHOT')
            if snapshot then screen:snapshot(snapshot) end
        end
    end
end)
