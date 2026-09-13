local snap_interval = 2.0
local last_snap_time = -100.0
local last_coin_time = -1000.0
local last_start_time = -1000.0
local last_a_time = -1000.0
local pulses = {}

local function get_field(port_tag, field_name)
    local port = manager.machine.ioport.ports[port_tag]
    if not port then return nil end
    return port.fields[field_name]
end

local function pulse(port_tag, field_name, now, dur)
    local f = get_field(port_tag, field_name)
    if f then
        f:set_value(1)
        table.insert(pulses, {port=port_tag, field=field_name, at=now+(dur or 0.3)})
    end
end

emu.register_periodic(function()
    local now = manager.machine.time:as_double()
    if now >= 2.0 and (now - last_coin_time) >= 8.0 then
        last_coin_time = now
        pulse(":AUDIO_COIN", "Coin 1", now)
    end
    if now >= 2.6 and (now - last_start_time) >= 8.0 then
        last_start_time = now
        pulse(":SYSTEM", "1 Player Start", now)
    end
    if now >= 15.0 and (now - last_a_time) >= 6.0 then
        last_a_time = now
        pulse(":edge:joy:JOY1", "P1 A", now)
    end
    for i = #pulses, 1, -1 do
        local p = pulses[i]
        if now >= p.at then
            local f = get_field(p.port, p.field)
            if f then f:set_value(0) end
            table.remove(pulses, i)
        end
    end
    if now - last_snap_time >= snap_interval then
        last_snap_time = now
        manager.machine.video:snapshot()
    end
end)
