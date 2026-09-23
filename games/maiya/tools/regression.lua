-- Inputs and debugger-only scenario placement; no test hooks in the cartridge.
local machine = manager.machine
local screen = machine.screens[':screen']
local memory = machine.devices[':maincpu'].spaces['program']
local output = assert(os.getenv('MG_CAPTURE_DIR'))
local a = dofile(output .. '/layout.lua')
local scenario = os.getenv('MG_SCENARIO') or 'walk'
local aes = os.getenv('MG_PLATFORM') == 'aes'
local testing_continue = scenario:sub(1, 8) == 'continue'
local log = assert(io.open(output .. '/telemetry.jsonl', 'w'))
local next_capture, placed, play_frame, continue_frame = 0, false, 0, 0
local pit_step, pit_wait = 0, 0
local pit_after, pit_after_frame = false, 0
local function u8(name) return memory:read_u8(a[name]) end
local function w8(name, value) memory:write_u8(a[name], value) end
local function s16(addr)
    local n = memory:read_u16(addr)
    return n >= 32768 and n - 65536 or n
end
local function input(name, value)
    for _, port in pairs(machine.ioport.ports) do
        local f = port.fields[name]
        if f then f:set_value(value and 1 or 0) end
    end
end
local function position(p, x, y)
    memory:write_u16(p + a.char_x, x)
    memory:write_u16(p + a.char_y, y)
    memory:write_u32(p + a.char_x_fp, x * 256)
    memory:write_u32(p + a.char_y_fp, y * 256)
end
local function start(value)
    input('1 Player Start', value)
    input('P1 Start', value)
end
emu.register_frame_done(function()
    local t = machine.time:as_double()
    input('Coin 1', t > 8 and t < 8.25)
    start(t > 12 and t < 12.25)
    local p = memory:read_u32(a.player)
    if p < 0x100000 or p > 0x10efff then p = 0 end
    -- Between Start landing and real gameplay there are now three screens
    -- (hero-select, how-to-play, the intro story) that each need a real
    -- button, not just a direction -- mg.player does not exist until she is
    -- through all three. Push through them here, unconditionally; once p is
    -- valid this stops, so a stray press from this schedule can never land
    -- during a scenario's own scripted inputs below.
    if p == 0 then
        input('P1 A', (t > 13.0 and t < 13.15) or (t > 14.5 and t < 14.65) or
                       (t > 16.0 and t < 16.15) or (t > 17.5 and t < 17.65))
    end
    local mode = memory:read_u8(0x10fdaf)
    if p ~= 0 and mode == 2 and u8('demo') == 0 then
        play_frame = play_frame + 1
        if not placed and u8('state') == 1 then
            placed = true
            if scenario == 'climb' then position(p, 708, 192) end
            if scenario == 'boss' then
                position(p, 3560, 192)
                memory:write_u16(a.camera_x, 3400)
                w8('gate_unlocked', 1)
            end
            if scenario == 'bonus' then
                w8('stage', 1)
                w8('state', 2)
                memory:write_u16(a.state_timer, 1)
            end
            if testing_continue then
                -- She always restarts a mission at its own head now, so
                -- there is no mid-level checkpoint to keep in sync here;
                -- only her position at the moment of death is staged.
                position(p, 1500, 192)
                w8('has_key', 1)
                w8('gate_unlocked', 1)
                w8('lives', 0)
                w8('state', 4)
                memory:write_u16(a.state_timer, 1)
            end
            if scenario == 'tray' then
                w8('has_key', 1)
                w8('coins', 99)
                w8('flowers', 99)
                w8('critters', 99)
                for _, name in ipairs({'swift', 'might', 'veil', 'spring', 'crown'}) do
                    memory:write_u16(a[name], 480)
                end
            end
            if scenario == 'factory' then
                w8('next_stage', 6)
                w8('state', 8)
                memory:write_u16(a.state_timer, 1)
            end
            -- Valley of Falls, same way the game reaches it after a boss:
            -- the ordinary mission-to-mission interlude, not a shortcut
            -- wired into the cartridge.
            if scenario == 'pit' then
                w8('next_stage', 1)
                w8('state', 8)
                memory:write_u16(a.state_timer, 1)
            end
        end
        -- Once the interlude has dropped her into Valley of Falls, drive a
        -- fixed sequence past its first pit (x=560, width=48): approach and
        -- stand at the lip, jump it, then a second pass that walks straight
        -- in without jumping, to confirm the fall (and the one-life cost)
        -- actually happens.
        if scenario == 'pit' and u8('stage') == 1 and u8('state') == 1 and p ~= 0 then
            local x = s16(p + a.char_x)
            if pit_step == 0 then
                position(p, 460, 192)
                memory:write_u8(p + a.char_hp, 5)
                pit_step, pit_wait = 1, 0
            elseif pit_step == 1 then
                input('P1 Right', true)
                if x >= 548 then pit_step, pit_wait = 15, 0 end
            elseif pit_step == 15 then
                input('P1 Right', false)   -- stop and stand at the lip first
                pit_wait = pit_wait + 1
                if pit_wait == 15 then screen:snapshot(output .. '/pit_01_at_lip.png') end
                if pit_wait == 20 then pit_step, pit_wait = 2, 0 end
            elseif pit_step == 2 then
                input('P1 Right', true)
                pit_wait = pit_wait + 1
                if pit_wait == 6 then input('P1 A', true) end
                if pit_wait == 12 then input('P1 A', false) end
                if pit_wait == 20 then screen:snapshot(output .. '/pit_02_mid_jump.png') end
                if pit_wait == 40 then
                    screen:snapshot(output .. '/pit_03_after_jump.png')
                    pit_step, pit_wait = 3, 0
                end
            elseif pit_step == 3 then
                input('P1 Right', false)
                position(p, 460, 192)
                memory:write_u8(p + a.char_hp, 5)
                w8('lives', 3)
                pit_step, pit_wait = 4, 0
            elseif pit_step == 4 then
                input('P1 Right', true)
                if x >= 555 then
                    screen:snapshot(output .. '/pit_04_before_fall.png')
                    pit_step, pit_wait = 5, 0
                end
            elseif pit_step == 5 then
                input('P1 Right', true)
                pit_wait = pit_wait + 1
                if pit_wait == 24 then screen:snapshot(output .. '/pit_05_falling.png') end
            end
        end
        -- The fall interrupts state==1, so the branch above stops running
        -- (and pit_step freezes at 5) the instant she dies; catch the
        -- after-death and respawn shots here instead, outside that gate.
        if scenario == 'pit' and pit_step == 5 and u8('state') == 4 and not pit_after then
            pit_after, pit_after_frame = true, play_frame
        end
        if pit_after and play_frame - pit_after_frame == 40 then
            screen:snapshot(output .. '/pit_06_after_fall.png')
        end
        if pit_after and play_frame - pit_after_frame == 200 then
            screen:snapshot(output .. '/pit_07_respawned.png')
        end
        input('P1 Right', scenario == 'walk' or (scenario == 'bonus' and play_frame % 240 < 120))
        input('P1 Left', scenario == 'boss' or (scenario == 'bonus' and play_frame % 240 >= 120))
        input('P1 Up', scenario == 'climb' and placed and play_frame < 210)
        input('P1 Down', scenario == 'climb' and play_frame >= 240 and play_frame < 390)
        input('P1 A', (scenario == 'walk' or scenario == 'bonus') and play_frame % 180 < 6)
        input('P1 B', (scenario == 'walk' or scenario == 'bonus') and play_frame % 24 < 6)
        input('P1 D', false)
        if scenario == 'pickups' and placed then
            local x, y = 901, 192
            if play_frame > 180 then x, y = 528, 104 end
            if play_frame > 300 then x, y = 901, 192 end
            position(p, x, y)
            memory:write_u8(p + a.char_hp, 5)
        end
        if testing_continue and u8('state') == 5 then
            continue_frame = continue_frame + 1
            if scenario == 'continue-timeout' then
                start(false)
            elseif aes then
                input('P1 Right', scenario == 'continue-exit' and continue_frame >= 45 and continue_frame < 55)
                input('P1 A', continue_frame >= 95 and continue_frame < 105)
            else
                input('Coin 1', continue_frame >= 45 and continue_frame < 55)
                input('P1 A', continue_frame >= 65 and continue_frame < 75)
                start((continue_frame >= 15 and continue_frame < 25) or
                      (continue_frame >= 95 and continue_frame < 105))
            end
        end
        if scenario == 'factory' and u8('stage') == 6 and u8('state') == 1 then
            position(p, 4840, 192)
            w8('gate_unlocked', 1)
            memory:write_u8(p + a.char_hp, 5)
            local boss = memory:read_u32(a.boss)
            if boss >= 0x100000 and boss <= 0x10efff and memory:read_u16(boss + a.char_data0) == 6 then
                memory:write_u8(boss + a.char_hp, 1)
                input('P1 D', play_frame % 90 < 10)
            end
        end
        -- Keep arena observation alive without changing movement or collisions.
        if scenario == 'boss' then memory:write_u8(p + a.char_hp, 5) end
    end
    if t >= next_capture then
        local b = memory:read_u32(a.boss)
        if b < 0x100000 or b > 0x10efff then b = 0 end
        local key_count, duplicates, sources = 0, 0, {}
        for i = 0, 3 do
            local item = a.items + i * a.item_size
            if memory:read_u8(item + a.item_life) ~= 0 then
                key_count = key_count + memory:read_u8(item + a.item_key)
                local source = memory:read_u8(item + a.item_source)
                if source ~= 0 then
                    if sources[source] then duplicates = duplicates + 1 end
                    sources[source] = true
                end
            end
        end
        screen:snapshot(string.format('%s/frame_%05d.png', output, math.floor(t * 10)))
        log:write(string.format('{"time":%.2f,"mode":%d,"state":%d,"stage":%d,"player":%d,"x":%d,"y":%d,"flip":%d,"climbing":%d,"camera_x":%d,"shake_x":%d,"boss_active":%d,"boss":%d,"boss_x":%d,"lives":%d,"continues":%d,"credit":%d,"player_mode":%d,"start_count":%d,"has_key":%d,"gate_unlocked":%d,"demo":%d,"pick_mask":%d,"key_count":%d,"duplicate_items":%d,"boss_kind":%d}\n',
            t, mode, u8('state'), u8('stage'), p, p ~= 0 and s16(p + a.char_x) or 0,
            p ~= 0 and s16(p + a.char_y) or 0, p ~= 0 and memory:read_u8(p + a.char_flip_x) or 0,
            u8('climbing'), s16(a.camera_x), s16(a.shake_x), u8('boss_active'), b, b ~= 0 and s16(b + a.char_x) or 0,
            u8('lives'), u8('continues'), aes and 0 or memory:read_u8(0xd00034), memory:read_u8(0x10fdb6),
            u8('start_count'), u8('has_key'), u8('gate_unlocked'), u8('demo'), memory:read_u16(a.pick_mask),
            key_count, duplicates, b ~= 0 and memory:read_u16(b + a.char_data0) or -1))
        log:flush()
        next_capture = t + 0.5
    end
end)
emu.register_stop(function() log:close() end)
