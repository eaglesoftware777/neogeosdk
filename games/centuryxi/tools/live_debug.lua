-- Century XI: the play-test logger.
--
-- tools/live_debug.py writes this, with the build's addresses in C, as the live
-- session's watch (live\watch\centuryxi_<mode>.lua): ngsdk_live.lua loads it, calls
-- W.stage for the match phase, and this file adds its own logs beside the session's
-- live.log. tools/genre_qa.py --lua runs the same file in the automated tests.
--
--   cx_debug.log    the match as it happened: phases, goals, shots, saves, fouls, cards,
--                   keepers, the buttons pressed (and what the game was doing),
--                   every pass a player played that missed its man (and why),
--                   sprites lost to the 96-a-line limit, men or pitch columns that vanished
--                   for a frame (flicker, ghosts), slow seconds with what was on screen
--   cx_seconds.csv  one row a second: clock, phase, score, ball, owner, camera, frame rate,
--                   sprites on the busiest line, flicker counts
--   cx_passes.csv   every pass: who, to whom, how, and what became of it
--   cx_mark_N.txt   at each M: every man, the ball, the camera and the last 40 log lines
--   cx_summary.txt  at the end: totals, pass completion, flicker, overflow, slow seconds
--   snap\cx_*.png   snapshots of the moments above (a few, never a flood)
--@@CONSTANTS@@

local machine = manager.machine
local mem = machine.devices[":maincpu"].spaces["program"]
local vram = emu.item(machine.devices[":spritegen"].items["0/m_videoram"])
local screen = machine.screens[":screen"]
local live = os.getenv("NGSDK_LIVE_DIR")
local dir = live or (cfg and cfg.out) or "."
local sep = package.config:sub(1, 1)
local snapdir = live and (dir .. sep .. "snap") or dir
local W = C.world
local O = C.off
CXDEBUG = CXDEBUG or {}               -- notifier subscriptions: a global, never collected

local function file(name, header)
    local f = io.open(dir .. sep .. name, "a")
    if f and header then f:write(header, "\n") end
    return f
end
local log = file("cx_debug.log")
local secs = file("cx_seconds.csv", "wall,t,clock,half,phase,score,ball_x,ball_y,ball_h,owner,ctrl1,ctrl2,cam_x,cam_y,game_fps,busiest_line,men_flicker,overflow_lines")
local passes = file("cx_passes.csv", "clock,team,player,from,to,kind,lofted,dist,result,receiver,closest,steered")
local recent = {}

-- ------------------------------------------------------------------ reading
local function u8(o) return mem:read_u8(W + o) end
local function u16(o) return mem:read_u16(W + o) end
local function s16(o) return mem:read_i16(W + o) end
local function u32(o) return mem:read_u32(W + o) end
local function man(i, f) return W + i * O.mansize + MAN_OFF[f] end
local function m8(i, f) return mem:read_u8(man(i, f)) end
local function ms16(i, f) return mem:read_i16(man(i, f)) end
local function units(v) return v // 16 end
local function ball(f) return W + O.ball + BALL_OFF[f] end
local function b16(f) return mem:read_i16(ball(f)) end
local function b8(f) return mem:read_u8(ball(f)) end

local function team_code(t)
    local id = u8(O.team + t)
    local s = ""
    for k = 0, 2 do s = s .. string.char(mem:read_u8(C.nation_code + id * 4 + k)) end
    return s
end
local function name_of(i)
    if i >= 22 then return "nobody" end
    local t = i >= 11 and 1 or 0
    local id, sq = u8(O.team + t), m8(i, "squad")
    local s = ""
    for k = 0, 9 do
        local c = mem:read_u8(C.roster_name + (id * 16 + sq) * 11 + k)
        if c == 0 then break end
        s = s .. string.char(c)
    end
    return string.format("%s %s (man %d, %s)", team_code(t), s, i, i % 11 == 0 and "keeper" or ("no. " .. (sq + 1)))
end
local function clock_text()
    local fpm = u16(O.fpm)
    if fpm == 0 then fpm = 1 end
    local m = u16(O.clock) // fpm
    return string.format("%s %02d'", u8(O.half) == 0 and "1st" or (u8(O.half) == 1 and "2nd" or ("ET" .. (u8(O.half) - 1))), m)
end
local function score_text() return string.format("%s %d-%d %s", team_code(0), u8(O.score), u8(O.score + 1), team_code(1)) end
local function phase_name(p) return C.phases[p + 1] or tostring(p) end
local function state_name(s) return C.states[s + 1] or tostring(s) end
local function dist(ax, ay, bx, by) local dx, dy = ax - bx, ay - by; return math.floor(math.sqrt(dx * dx + dy * dy)) end

local function say(kind, text)
    local line = string.format("%s %8.2f  %-9s %-9s %s", os.date("%H:%M:%S"), machine.time:as_double(), clock_text(), kind, text)
    if log then log:write(line, "\n") log:flush() end
    recent[#recent + 1] = line
    if #recent > 40 then table.remove(recent, 1) end
end

local shots_taken, last_shot = {}, {}
local function snap(kind, every)
    local t = machine.time:as_double()
    if last_shot[kind] and t - last_shot[kind] < (every or 8) then return "" end
    last_shot[kind] = t
    shots_taken[kind] = (shots_taken[kind] or 0) + 1
    local path = string.format("%s%scx_%s_%03d.png", snapdir, sep, kind, shots_taken[kind])
    pcall(function() screen:snapshot(path) end)
    return "  (" .. path:match("[^\\/]+$") .. ")"
end

-- ------------------------------------------------------------------ the match
local last_frame, last_phase, last_owner = nil, nil, 255
local keeper_held = {false, false}
local total = {goals = 0, shots = {0, 0}, saves = 0, fouls = 0, cards = 0}
local pass = nil                          -- the pass on its way
local pass_n = {{0, 0}, {0, 0}}           -- [team][1 tried, 2 reached]
local pass_fail = {}                      -- reasons, for the summary

local function men_near_ball(r)
    local bx, by, n = units(b16("x")), units(b16("y")), 0
    for i = 0, 21 do
        if dist(units(ms16(i, "x")), units(ms16(i, "y")), bx, by) < r then n = n + 1 end
    end
    return n
end

local function pass_done(result, receiver)
    local p = pass
    pass = nil
    local t = p.team + 1
    if result ~= "into space" then        -- a kick-off tap or a ball into space has no man to reach
        pass_n[t][1] = pass_n[t][1] + 1
        if result == "reached" then pass_n[t][2] = pass_n[t][2] + 1 end
    end
    if passes then
        passes:write(string.format("%s,%d,%d,%d,%d,%s,%d,%d,%s,%d,%d,%d\n", p.clock, p.team, p.human and 1 or 0, p.from, p.to,
            p.kind, p.lofted and 1 or 0, p.dist, result, receiver or 255, p.closest, p.steered and 1 or 0))
        passes:flush()
    end
    if result ~= "reached" and result ~= "into space" and p.human then
        local why = result
        if result == "another mate" then why = "reached another team-mate" end
        pass_fail[why] = (pass_fail[why] or 0) + 1
        say("PASS?", string.format("P%d's %s pass %s -> %s %s: %s; the ball came within %d of him%s%s", p.team + 1, p.kind,
            name_of(p.from), name_of(p.to), p.dist .. " units", why, p.closest,
            p.steered and ", the player steered him away from it" or "",
            snap("pass", 10)))
    end
end

local function track_pass(ev, owner)
    if pass then
        if pass.to < 22 then
            local d = dist(units(ms16(pass.to, "x")), units(ms16(pass.to, "y")), units(b16("x")), units(b16("y")))
            if d < pass.closest then pass.closest = d end
            if u8(O.recv_dir + pass.team) == 0xFE then pass.steered = true end
        end
        if owner < 22 and owner ~= pass.from then
            if owner == pass.to then pass_done("reached", owner)
            elseif (owner >= 11 and 1 or 0) == pass.team then pass_done("another mate", owner)
            else pass_done("intercepted", owner) end
        elseif u8(O.phase) ~= 1 then
            pass_done((ev & C.ev.OUT) ~= 0 and "out of play" or ("stopped: " .. phase_name(u8(O.phase))), nil)
        end
    end
    if (ev & C.ev.PASS) ~= 0 then
        local from = b8("last")
        -- played first time by the man it was for (a one-two): it reached him
        if pass then pass_done(from == pass.to and "reached" or "played on before it arrived", from == pass.to and from or nil) end
        if from < 22 then
            local t = from >= 11 and 1 or 0
            local to = u8(O.pass_to)
            pass = {clock = clock_text(), team = t, human = u8(O.human + t) ~= 0, from = from, to = to,
                kind = (to == 255 and "into space" or (C.kicks[m8(from, "kick") + 1] or "pass")), lofted = mem:read_i16(ball("vh")) > 10,
                dist = to < 22 and dist(units(ms16(from, "x")), units(ms16(from, "y")), units(ms16(to, "x")), units(ms16(to, "y"))) or 0,
                closest = 9999, steered = false}
            if from % 11 == 0 then
                say("KEEPER", string.format("%s plays it out: %s to %s", name_of(from), pass.lofted and "kicked long" or "thrown/rolled", name_of(to)))
            end
            if to == 255 then pass_done("into space", nil) end
        end
    end
end

local prev_ev = 0
local function match_tick(step)
    local raw, phase, owner = u32(O.ev), u8(O.phase), b8("owner")
    -- a late frame's caught-up tick keeps the tick before's events: seen last time, not again
    local ev = step == 1 and (raw & ~prev_ev) or raw
    prev_ev = raw
    if phase ~= last_phase then
        if last_phase ~= nil then say("PHASE", string.format("%s -> %s  %s", phase_name(last_phase), phase_name(phase), score_text())) end
        last_phase = phase
    end
    if (ev & C.ev.GOAL) ~= 0 then
        total.goals = total.goals + 1
        local sc = u8(O.scorer)
        say("GOAL", string.format("%s  scored by %s, %s%s", score_text(), name_of(sc),
            C.goal_kinds[u8(O.goal_kind) + 1] or "?", snap("goal", 3)))
    end
    if (ev & C.ev.SHOT) ~= 0 then
        local s = b8("last")
        if s < 22 then
            local t = s >= 11 and 1 or 0
            total.shots[t + 1] = total.shots[t + 1] + 1
            local right = ((t == 0) ~= (u8(O.half) % 2 == 1))
            local gx = right and C.len or 0
            say("SHOT", string.format("%s from %d units, speed %d", name_of(s), dist(units(ms16(s, "x")), units(ms16(s, "y")), gx, C.mid_y),
                math.abs(b16("vx")) + math.abs(b16("vy"))))
        end
    end
    if (ev & C.ev.SAVE) ~= 0 then total.saves = total.saves + 1; say("SAVE", "by " .. name_of(owner < 22 and owner or b8("last"))) end
    if (ev & C.ev.POST) ~= 0 then say("POST", "off the woodwork") end
    if (ev & C.ev.MISS) ~= 0 then say("MISS", "wide or over") end
    if (ev & C.ev.FOUL) ~= 0 then total.fouls = total.fouls + 1; say("FOUL", "on the ball: " .. name_of(owner)) end
    if (ev & C.ev.CARD) ~= 0 then
        total.cards = total.cards + 1
        say("CARD", string.format("%s for %s", u8(O.card_red) ~= 0 and "red" or "yellow", name_of(u8(O.card_man))))
    end
    if (ev & C.ev.OFFSIDE) ~= 0 then say("OFFSIDE", "flag up") end
    if (ev & C.ev.CHALLENGE) ~= 0 then say("VAR", "a side asked for a review") end
    -- a player's keeper with it in his hands
    for t = 0, 1 do
        local k = t * 11
        local held = u8(O.human + t) ~= 0 and owner == k and m8(k, "state") == C.hold_state
        if held and not keeper_held[t + 1] then say("KEEPER", string.format("P%d's keeper %s has it in his hands", t + 1, name_of(k))) end
        keeper_held[t + 1] = held
    end
    track_pass(ev, owner)
    last_owner = owner
end

-- ------------------------------------------------------------------ the picture
-- A sprite's first strip as the hardware has it: height (0 hidden), x, y and palette.
local function strip(s)
    local scb3 = vram:read(0x8200 + s)
    local y = (496 - (scb3 >> 7)) & 0x1ff
    if y >= 256 then y = y - 512 end
    local x = (vram:read(0x8400 + s) >> 7) & 0x1ff
    if x >= 384 then x = x - 512 end
    local w = 16
    for j = 1, 3 do
        if (vram:read(0x8200 + s + j) & 0x40) == 0 then break end
        w = w + 16
    end
    return {size = scb3 & 0x3f, sticky = (scb3 & 0x40) ~= 0, x = x, y = y, w = w, h = (scb3 & 0x3f) * 16, pal = vram:read(s * 64 + 1) >> 8}
end
local function inside(a) return a and a.size > 0 and a.x >= 24 and a.x <= 280 and a.y >= 24 and a.y <= 180 end

-- A man on screen two frames ago and again now, near the same place, who in the frame
-- between was drawn nowhere near there: he vanished for a frame (flicker). Drawn there but
-- in another man's colours: a ghost (the wrong man in his place). The men's slots follow the
-- depth order, so a man changing slots between frames is no fault: he is looked for in all.
local blinks = {men = 0, ghost = 0, ball = 0}
local sec_blinks = {men = 0}
local frames_seen = {}          -- the last three frames' men: {x, y, pal, size} per slot
local slots_n = (C.spr.men_end - C.spr.men + 1) // 5
local ball_hist = {}
local function men_now()
    local t = {}
    for k = 0, slots_n - 1 do t[k] = strip(C.spr.men + k * 5) end
    return t
end
-- In frame fr, is the man drawn anywhere between where he was (a) and is (c)? His figure,
-- not its first strip: a pose with an arm out starts a column further left. "same" his
-- palette there, "other" only other men there, nil nobody.
local function overlaps(b, x0, y0, x1, y1) return b.x < x1 and b.x + b.w > x0 and b.y < y1 and b.y + b.h > y0 end
local function drawn_between(fr, a, c)
    local x0, y0 = math.min(a.x, c.x) - 8, math.min(a.y, c.y) - 8
    local x1, y1 = math.max(a.x + a.w, c.x + c.w) + 8, math.max(a.y + a.h, c.y + c.h) + 8
    local found = nil
    for k = 0, slots_n - 1 do
        local b = fr[k]
        if b.size > 0 and overlaps(b, x0, y0, x1, y1) then
            if b.pal == a.pal then return "same" end
            found = "other"
        end
    end
    return found
end
local function check_blinks()
    local now = men_now()
    frames_seen[#frames_seen + 1] = now
    if #frames_seen > 3 then table.remove(frames_seen, 1) end
    local cut = 0
    if #frames_seen == 3 then
        -- a cut (the replay, a new scene): everybody somewhere else at once, whichever slot
        -- each man is in now
        local where = {}
        for k = 0, slots_n - 1 do
            local b = frames_seen[2][k]
            if b.size > 0 then where[b.pal] = b end
        end
        for k = 0, slots_n - 1 do
            local a = frames_seen[1][k]
            local b = a.size > 0 and where[a.pal]
            if b and (math.abs(a.x - b.x) > 40 or math.abs(a.y - b.y) > 40) then cut = cut + 1 end
        end
    end
    if #frames_seen == 3 and cut < 3 then
        local before, gap = frames_seen[1], frames_seen[2]
        for k = 0, slots_n - 1 do
            local a = before[k]
            if inside(a) then
                -- the same man now (his palette, close by), then where he should be between
                local c, best = nil, 41
                for j = 0, slots_n - 1 do
                    local d = now[j]
                    if d.size > 0 and d.pal == a.pal then
                        local e = math.max(math.abs(d.x + d.w // 2 - a.x - a.w // 2), math.abs(d.y + d.h - a.y - a.h))
                        if e < best then best, c = e, d end
                    end
                end
                if c and inside(c) then
                    local there = drawn_between(gap, a, c)
                    if there ~= "same" then
                        local what = there == "other" and "ghost" or "men"
                        blinks[what] = blinks[what] + 1
                        sec_blinks.men = sec_blinks.men + 1
                        if blinks.men + blinks.ghost <= 12 then
                            -- every slot in the three frames, for the first few: what moved where
                            local f = io.open(dir .. sep .. "cx_flicker_detail.txt", "a")
                            if f then
                                f:write(string.format("%.2f s: palette %d at %d,%d (%s)\n", machine.time:as_double(), a.pal, a.x, a.y, what))
                                for j = 0, slots_n - 1 do
                                    local x, y, z = before[j], gap[j], now[j]
                                    f:write(string.format("  slot %2d  %3d %4d,%4d s%d | %3d %4d,%4d s%d | %3d %4d,%4d s%d\n", j,
                                        x.pal, x.x, x.y, x.size, y.pal, y.x, y.y, y.size, z.pal, z.x, z.y, z.size))
                                end
                                f:close()
                            end
                        end
                        if blinks[what] <= 40 or blinks[what] % 50 == 0 then
                            say(what == "ghost" and "GHOST" or "FLICKER", string.format("a man (palette %d) at %d,%d %s for one frame; ball %d,%d, %d men near it%s",
                                a.pal, a.x, a.y, what == "ghost" and "was not drawn (only another man there)" or "was not drawn",
                                units(b16("x")), units(b16("y")), men_near_ball(120), snap("flicker", 6)))
                        end
                    end
                end
            end
        end
    end
    local bnow = strip(C.spr.ball)
    ball_hist[#ball_hist + 1] = bnow
    if #ball_hist > 3 then table.remove(ball_hist, 1) end
    if #ball_hist == 3 then
        local a, b = ball_hist[1], ball_hist[2]
        if inside(a) and inside(bnow) and math.abs(bnow.x - a.x) < 24 and math.abs(bnow.y - a.y) < 24 and b.size == 0 then
            blinks.ball = blinks.ball + 1
            if blinks.ball <= 20 then say("FLICKER", string.format("the ball at %d,%d was not drawn for one frame%s", a.x, a.y, snap("flicker", 6))) end
        end
    end
end

local busiest, over_lines, overflow_total, overflow_logged = 0, 0, 0, 0
local function check_lines()
    local d = {}
    for l = 0, 224 do d[l] = 0 end
    local y, size = 0, 0
    for s = 1, 380 do
        local scb3 = vram:read(0x8200 + s)
        if (scb3 & 0x40) == 0 then
            y = (496 - (scb3 >> 7)) & 0x1ff
            size = scb3 & 0x3f
        end
        if size > 0 then
            local sy = y
            if sy >= 256 then sy = sy - 512 end
            local top, bottom = math.max(sy, 0), math.min(sy + size * 16, 224)
            if bottom > top then d[top] = d[top] + 1; d[bottom] = d[bottom] - 1 end
        end
    end
    local n, worst, worst_line, over = 0, 0, 0, 0
    for l = 0, 223 do
        n = n + d[l]
        if n > worst then worst, worst_line = n, l end
        if n > 96 then over = over + 1 end
    end
    if worst > busiest then busiest = worst end
    if over > 0 then
        over_lines = over_lines + over
        overflow_total = overflow_total + 1
        if overflow_logged < 30 then
            overflow_logged = overflow_logged + 1
            say("OVERFLOW", string.format("%d sprites on line %d (96 shown): %d lines lose sprites, ball at %d,%d%s", worst, worst_line,
                over, units(b16("x")), units(b16("y")), snap("overflow", 10)))
        end
    end
end

-- ------------------------------------------------------------------ seconds, marks, the end
local vbl, frames_before, fps, slow = 0, nil, 0, 0
local dropped, dropped_sec, catch_seen = 0, 0, nil   -- pictures not drawn: a late frame's tick paid without drawing
local fps_hist = {}
local function second()
    local f = mem:read_u32(C.frames)
    fps = frames_before and ((f - frames_before) & 0xFFFFFFFF) or 0
    if fps > 200 then fps = 0 end
    frames_before = f
    local playing = last_phase == 1
    fps = math.max(0, fps - dropped_sec)        -- pictures drawn, not loops run
    dropped_sec = 0
    if playing then
        fps_hist[fps] = (fps_hist[fps] or 0) + 1
        if fps < 50 then
            slow = slow + 1
            if slow <= 60 or slow % 30 == 0 then
                say("SLOW", string.format("%d frames drawn this second: ball at %d,%d, %d men within 120 of it, camera %d,%d%s", fps,
                    units(b16("x")), units(b16("y")), men_near_ball(120), mem:read_i16(C.camx), mem:read_i16(C.camy), snap("slow", 20)))
            end
        end
    end
    if secs and last_phase ~= nil then
        secs:write(string.format("%s,%d,%s,%d,%s,%d-%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", os.date("%H:%M:%S"),
            math.floor(machine.time:as_double()), clock_text(), u8(O.half), phase_name(last_phase), u8(O.score), u8(O.score + 1),
            units(b16("x")), units(b16("y")), units(b16("h")), b8("owner"), u8(O.ctrl), u8(O.ctrl + 1),
            mem:read_i16(C.camx), mem:read_i16(C.camy), fps, busiest, sec_blinks.men, over_lines))
        secs:flush()
    end
    busiest, over_lines, sec_blinks.men = 0, 0, 0
end

local marks = 0
local key = machine.input:code_from_token("KEYCODE_M")
local key_down = false
local function dump_mark()
    marks = marks + 1
    local f = io.open(string.format("%s%scx_mark_%d.txt", dir, sep, marks), "w")
    if not f then return end
    f:write(string.format("mark %d at %.2f s, %s, %s, phase %s\n", marks, machine.time:as_double(), clock_text(), score_text(), phase_name(u8(O.phase))))
    f:write(string.format("ball  x %d y %d h %d  vx %d vy %d vh %d  owner %d last %d  pass_to %d\n", units(b16("x")), units(b16("y")),
        units(b16("h")), b16("vx"), b16("vy"), b16("vh"), b8("owner"), b8("last"), u8(O.pass_to)))
    f:write(string.format("controlled: P1 man %d, P2 man %d   humans %d %d   camera %d,%d   fps %d\n", u8(O.ctrl), u8(O.ctrl + 1),
        u8(O.human), u8(O.human + 1), mem:read_i16(C.camx), mem:read_i16(C.camy), fps))
    f:write("man  name                                   state     x     y    vx   vy  dir timer\n")
    for i = 0, 21 do
        f:write(string.format("%3d  %-38s %-8s %5d %5d %5d %4d %4d %4d\n", i, name_of(i), state_name(m8(i, "state")),
            units(ms16(i, "x")), units(ms16(i, "y")), ms16(i, "vx"), ms16(i, "vy"), m8(i, "dir"), m8(i, "timer")))
    end
    f:write("\nthe log before it:\n")
    for _, l in ipairs(recent) do f:write(l, "\n") end
    f:close()
    say("MARK", string.format("#%d: the match at this moment is in cx_mark_%d.txt", marks, marks))
end

-- The buttons each player pressed, with what the game was doing: "I pressed B and nothing
-- happened" can be checked against who he had and where the ball was.
local pads = {{addr = 0x10FD96, prev = 0}, {addr = 0x10FD9C, prev = 0}}
local BUTTONS = {{0x10, "A"}, {0x20, "B"}, {0x40, "C"}, {0x80, "D"}}
local function check_pads()
    for p = 1, 2 do
        local pad = pads[p]
        local cur = mem:read_u8(pad.addr)
        local edge = cur & ~pad.prev
        pad.prev = cur
        if edge & 0xF0 ~= 0 then
            local names = {}
            for _, b in ipairs(BUTTONS) do if edge & b[1] ~= 0 then names[#names + 1] = b[2] end end
            local dirs = (cur & 1 ~= 0 and "up " or "") .. (cur & 2 ~= 0 and "down " or "") .. (cur & 4 ~= 0 and "left " or "") .. (cur & 8 ~= 0 and "right" or "")
            local c = u8(O.ctrl + p - 1)
            local own = b8("owner")
            say("PRESS", string.format("P%d %s%s  controls %s (%s), ball with %s, %s", p, table.concat(names, "+"),
                dirs ~= "" and (" stick " .. dirs) or "", c < 22 and name_of(c) or "nobody", c < 22 and state_name(m8(c, "state")) or "-",
                own < 22 and name_of(own) or "nobody", phase_name(u8(O.phase))))
        end
    end
end

local function frame()
    vbl = vbl + 1
    local um = mem:read_u8(0x10FDAF)
    local fr = u16(O.frame)
    if um == 2 and fr ~= last_frame then
        if last_frame ~= nil then pcall(match_tick, (fr - last_frame) & 0xFFFF) end
        last_frame = fr
    end
    if um == 2 and fr ~= catch_seen then
        catch_seen = fr
        if mem:read_u8(C.catch_run) ~= 0 then dropped, dropped_sec = dropped + 1, dropped_sec + 1 end
    end
    if um == 2 and last_phase ~= nil then
        pcall(check_pads)
        pcall(check_blinks)
        if vbl % 2 == 0 then pcall(check_lines) end
    end
    if vbl % 60 == 0 then pcall(second) end
    local down = machine.input:code_pressed(key)
    if down and not key_down then pcall(dump_mark) end
    key_down = down
end

local function finish()
    local f = io.open(dir .. sep .. "cx_summary.txt", "w")
    if not f then return end
    local function pct(t) local p = pass_n[t]; return p[1] > 0 and string.format("%d of %d (%d%%)", p[2], p[1], p[2] * 100 // p[1]) or "none" end
    f:write("Century XI play-test summary (build " .. C.build .. ")\n\n")
    f:write(string.format("goals %d, shots %d / %d, saves %d, fouls %d, cards %d\n", total.goals, total.shots[1], total.shots[2],
        total.saves, total.fouls, total.cards))
    f:write(string.format("passes reaching their man: side 1 %s, side 2 %s\n", pct(1), pct(2)))
    for why, n in pairs(pass_fail) do f:write(string.format("  a player's pass that did not: %s x%d\n", why, n)) end
    f:write(string.format("flicker (not drawn for one frame): men %d, ball %d; of them with another man standing there %d\n",
        blinks.men, blinks.ball, blinks.ghost))
    f:write(string.format("frames with sprites lost to the 96-a-line limit: %d\n", overflow_total))
    f:write(string.format("seconds of play under 50 frames: %d; pictures dropped to pay a late frame's tick: %d\n", slow, dropped))
    local keys = {}
    for k in pairs(fps_hist) do keys[#keys + 1] = k end
    table.sort(keys)
    f:write("frames drawn per second in play (frames: seconds):")
    for _, k in ipairs(keys) do f:write(string.format(" %d:%d", k, fps_hist[k])) end
    f:write("\n")
    f:close()
    if log then log:close() end
    if secs then secs:close() end
    if passes then passes:close() end
end

say("START", "Century XI play-test logger, build " .. C.build)
CXDEBUG[#CXDEBUG + 1] = emu.add_machine_frame_notifier(frame)
CXDEBUG[#CXDEBUG + 1] = emu.add_machine_stop_notifier(finish)

local Wt = {names = C.phases}
Wt.stage = function(m) return m:read_u8(W + O.phase) end
return Wt
