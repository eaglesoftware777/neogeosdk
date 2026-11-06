"""
Chapter VII: The Great Valley Restoration (the ancient hydraulic complex).

Behind the old watermill a stair goes down to the works the seven wonders
were built to serve: the valley's reservoir and the machine that shares its
water. Director Valen's crews are already there, drilling a new channel to
send the whole reservoir to his central plant, and the rising water has
trapped some of his own workers.

The six wonder components go into the distribution machine's sockets; in
the sluice gallery the water goes to the three valley channels while the
overflow, which floods the workers' bay, stays shut. In the bay a bridge
gives way behind the sisters; the trapped workers are freed by one sister
holding the counterweight plate and Luna working the cage's fine lever,
and they open the ladder out. The drill is stopped by pulling its three
breakers between the steam jets. In the control room both sisters open the
reservoir valve, and the Builders' Charter shows how the water was meant to
be shared: by season and by need, every village its part. Valen listens.
"""
from worldlib import (room, exit_, add_exit, npc, sign, pickup, chest, rest, gate, creature, trigger, talk,
                      item, chapter, counter, signal_flag, lock, bridge, plate, lever, hazard, water_src,
                      valve, outlet, block, find, collectible, speaker, canvas, put, text, say, J, maiya, luna)

speaker('BRACK', 'Foreman Brack')

item('CHARTER', 'key|record', "Builders' Charter", 'The seventh wonder: how the builders meant the water to be '
     'shared.')
collectible('WONDER7', 'wonder', "The Builders' Charter")

counter('QUEST7')   # 0 arrived, 1 Brack's news, 2 the machine, 3 the sluices, 4 the workers, 5 the drill, 6 the
                    # valve, 7 the plan

chapter(7, 'The Great Valley Restoration', 'The Hydraulic Halls', "The Builders' Charter",
        start=('wheel_passage', 9, 2, 'down'), quest='QUEST7', wonder='CHARTER',
        restore_title='The valley is restored',
        restore_text='Water for every village,\nby season and by need, as\nthe builders meant it.',
        back=('control_room', 9, 10, 'down'),
        end_text='The seven wonders turn\ntogether. The valley will\nnot go thirsty again.',
        goals=[(0, "Go down the stair behind the watermill into the old hydraulic works."),
               (1, "Set the six wonder components in the distribution machine, west of the camp."),
               (2, "In the sluice gallery, send water to all three valley channels. Keep the overflow shut: "
                   "it floods the workers' bay."),
               (3, "Rescue the workers trapped in the bay: one of you holds the plate, Luna works the lever."),
               (4, "Stop Valen's drill: pull its three breakers. Mind the steam."),
               (5, "In the control room, open the reservoir valve together: a plate and a fine lever."),
               (6, "Talk to Director Valen. Show him how the builders meant the water to be shared."),
               (7, "The valley is restored. Director Valen has something to say.")])

# ---- the passage behind the wheel ------------------------------------------------------------------------
talk('ch7_intro', 'SUNBOY', "This is where I found the drawings! Seven buildings, and the lines all lead down "
     "here.", next='ch7_intro2')
talk('ch7_intro2', 'TOVAN', "And I heard drills in the night, under my own mill. Valen's crews came in from the "
     "other side. Go carefully, girls.", next='ch7_intro3')
say('ch7_intro3', "Carefully. Yes. I'm getting good at carefully.", next='ch7_intro4')
talk('ch7_intro4', J('LUNA', 'MAIYA'),
     J("I'll stay up here at the high sluice with Master Tovan. If the reservoir rises, someone has to open the old "
       "spill in time. Go on, Maiya: careful, and quick.",
       "I'll stay up here at the high sluice with Master Tovan. If the water rises, somebody strong has to haul "
       "the old spill open. Go on, Luna. Be clever down there!"))
talk('passage_tablet', 'SIGN', "CARVED OVER THE STAIR: SEVEN WORKS FOR ONE VALLEY. THE WATER BELONGS TO NO ONE AND "
     "TO EVERYONE. SHARE IT BY THE SEASON AND BY THE NEED.")
talk('sunboy7', 'SUNBOY', "I'll stay with Master Tovan. Somebody has to tell the village what's happening!")
talk('tovan7', 'TOVAN', "A machine that shares water fairly between seven regions... the builders were better "
     "engineers than all of us.")
talk('mill_down_locked', 'TOVAN', "Not down there, young lady. Not yet. Whatever is under my mill can wait.")

g = canvas('X', ':')
put(g, 9, 0, 10, 0, 'S'); put(g, 9, 1, 10, 3, 'S')
put(g, 9, 13, 10, 13, ':')
put(g, 2, 2, 4, 4, 'Z'); put(g, 15, 2, 17, 4, 'Z')
room('wheel_passage', 'The Passage Behind the Wheel', text(g),
     region=8, music='works', restored_music='restored',
     exits=[exit_(9, 0, 2, 1, 'mill', 2, 4, 'down'),
            exit_(9, 13, 2, 1, 'works_gate', 9, 1, 'down')],
     objects=[
         sign(6, 6, 'passage_tablet', look='tablet'),
         npc(13, 6, 'SUNBOY', 'sunboy7', name='sunboy7'),
         npc(6, 9, 'TOVAN', 'tovan7', name='tovan7'),
         find(17, 11, 'history', "Builder's Stone Mark", 'badge'),
         find(2, 11, 'flower', 'Dandelion in a Crack', 'flower_orange'),
     ],
     triggers=[trigger('room', once='CH7_BEGUN', actions=[('talk', 'ch7_intro')])])
add_exit('mill', exit_(2, 3, 1, 1, 'wheel_passage', 9, 1, 'up', cond=('ctr_ge', 'CHAPTER', 6),
                       locked='mill_down_locked'))

# ---- the works gate: Valen's camp ----------------------------------------------------------------------------
talk('brack7', 'BRACK', "The lads are out and the drill is stopped. Whatever happens up there with the Director, "
     "you have my thanks.", cond=('flag', 'DRILL_STOPPED'), alt='brack7_mid', next=None)
talk('brack7_mid', 'BRACK', "The distribution machine is west of here. The Director says it's a museum piece. "
     "My grandfather said it was the cleverest thing in the valley.", cond=('ctr_ge', 'QUEST7', 1),
     alt='brack7_first')
talk('brack7_first', 'BRACK', "Who let a youngster down here? ...Wait, you're one of the sisters. The ones fixing the old "
     "wonders.", next='brack7_b', actions=[('ctr', 'QUEST7', 1)])
talk('brack7_b', 'BRACK', "Here's the truth of it. The Director wants the whole reservoir piped to his central "
     "plant. We're drilling a new channel through the reservoir wall.", next='brack7_c')
talk('brack7_c', 'BRACK', "And the water's rising in the bay below, and three of my lads are cut off down there. "
     "The old gates jammed. I don't know how to work any of it.", next='brack7_d')
say('brack7_d', "The builders' machine can send the water where it's meant to go. Then I can reach your "
    "workers.")
talk('worker7', 'WORKER', "I just wanted a steady job. Nobody said anything about drilling into a lake.")
talk('gallery_locked', 'SIGN', "A great gate, shut. Its gearing runs back to the distribution machine.")
talk('drill_locked', 'SIGN', "The service stair down to the drill. Flooded to the steps.")
talk('camp_sign', 'SIGN', "VALEN WORKS. AUTHORISED CREWS ONLY. HARD HATS. (THE SISTERS ARE PROBABLY AUTHORISED.)")

g = canvas('X', ':')
put(g, 9, 0, 10, 0, ':'); g[7][0] = g[8][0] = ':'; g[7][19] = g[8][19] = ':'; put(g, 9, 13, 10, 13, ':')
put(g, 2, 2, 5, 3, 'I'); put(g, 14, 10, 17, 11, 'I'); put(g, 3, 10, 4, 11, 'E')
put(g, 13, 2, 17, 2, 'Z')
room('works_gate', "Valen's Camp", text(g),
     region=8, music='works', restored_music='restored',
     exits=[exit_(9, 0, 2, 1, 'wheel_passage', 9, 12, 'up'),
            exit_(0, 7, 1, 2, 'pump_hall', 18, 7, 'left'),
            exit_(19, 7, 1, 2, 'sluice_gallery', 1, 11, 'right', cond=('flag', 'MACHINE_ASSEMBLED'),
                  locked='gallery_locked'),
            exit_(9, 13, 2, 1, 'drill_chamber', 1, 3, 'down', cond=('flag', 'WORKERS_SAFE'), locked='drill_locked')],
     objects=[
         npc(9, 6, 'BRACK', 'brack7', name='brack'),
         npc(13, 8, 'WORKER', 'worker7', name='worker7', wander=1, speed=6),
         sign(7, 11, 'camp_sign', look='signpost'),
         find(18, 1, 'history', "Smoggar-Era Valve Key", 'key'),
         find(1, 12, 'photo', "Valen's Camp", 'photo'),
         find(18, 12, 'diagram', 'New Channel Survey (Wrong)', 'scroll'),
     ])

# ---- the distribution hall -------------------------------------------------------------------------------------
say('core_hint', "The distribution machine. Six sockets round its core, each shaped for one of the wonders' "
    "parts.")
say('sock_wheel', "A socket shaped for the Waterwheel Mechanism.")
say('sock_map', "A frame shaped for the Forest Survey Map.")
say('sock_lens', "A cradle for the Precision Optical Lens.")
say('sock_gear', "A housing for the Ancient Gear Assembly.")
say('sock_instr', "A mount for the Underground Survey Instrument.")
say('sock_chart', "A frame for the Master Navigation Chart.")
say('sock_need', "I don't have the piece for this socket yet.")
say('machine_runs', "Six for six! Listen to it hum... and something big just unlocked out by the camp.",
     actions=[('ctr', 'QUEST7', 2)])
talk('hall_plaque', 'SIGN', "PLAQUE: SEVEN REGIONS, ONE RESERVOIR. THE MACHINE MEASURES EACH REGION'S NEED: THE "
     "MILL'S FLOW, THE FOREST'S ROADS, THE COAST'S TIDES, THE ORCHARD'S ROWS, THE GROTTO'S RIVER, THE STARS' SEASONS.")

g = canvas('X', 'm')
g[7][19] = g[8][19] = 'm'
put(g, 7, 4, 12, 8, 'I')
room('pump_hall', 'The Distribution Hall', text(g),
     region=8, music='works', restored='pump_hall_restored', restored_music='restored',
     exits=[exit_(19, 7, 1, 2, 'works_gate', 1, 7, 'right')],
     objects=[
         lock(6, 5, 'WHEELMECH', used=0, flag_='SET_WHEEL', look='socket', name='s_wheel', hint='sock_wheel',
              text='sock_need'),
         lock(6, 7, 'SURVEYMAP', used=0, flag_='SET_MAP', look='socket', name='s_map', hint='sock_map',
              text='sock_need'),
         lock(9, 3, 'OPTLENS', used=0, flag_='SET_LENS', look='socket', name='s_lens', hint='sock_lens',
              text='sock_need'),
         lock(10, 9, 'GEARASSEMBLY', used=0, flag_='SET_GEAR', look='socket', name='s_gear', hint='sock_gear',
              text='sock_need'),
         lock(13, 5, 'INSTRUMENT', used=0, flag_='SET_INSTR', look='socket', name='s_instr', hint='sock_instr',
              text='sock_need'),
         lock(13, 7, 'NAVCHART', used=0, flag_='SET_CHART', look='socket', name='s_chart', hint='sock_chart',
              text='sock_need'),
         signal_flag('SET_WHEEL', 'A1', name='f1'), signal_flag('SET_MAP', 'A2', name='f2'),
         signal_flag('SET_LENS', 'A3', name='f3'), signal_flag('SET_GEAR', 'A4', name='f4'),
         signal_flag('SET_INSTR', 'A5', name='f5'), signal_flag('SET_CHART', 'A6', name='f6'),
         gate(0, 0, inp=['A1', 'A2', 'A3', 'A4', 'A5', 'A6'], flag_='MACHINE_ASSEMBLED', look='none', name='assembled',
              actions=[('talk', 'machine_runs')]),
         sign(3, 11, 'hall_plaque', look='tablet'),
         find(17, 1, 'diagram', 'Distribution Machine Drawing', 'scroll'),
         find(1, 1, 'history', "Builders' Measuring Rod", 'shard'),
         find(16, 12, 'photo', 'The Machine Wakes', 'photo'),
     ])

# ---- the sluice gallery ------------------------------------------------------------------------------------------
say('intake_hint', "The intake: it lets the reservoir into the gallery's channels.")
say('north_hint', "The sluice to the north channel: the valley and Willowbrook.")
say('east_hint', "The sluice to the east channel: the forest and the coast.")
say('south_hint', "The sluice to the south channel: the grove and the grotto.")
say('overflow_hint', "The overflow sluice. It's open: it pours straight into the workers' bay!")
say('bay_door_hint', "The bay door. It won't open against the water pouring in from the overflow.")
say('safety_hint', "A safety gate on the main line. While the overflow runs, it holds the valley channels "
     "shut.")
say('steam_hint', "Steam from the cracked boiler pipes. It comes in puffs: go between them.")
say('sluices_set', "North, east and south channels running, the overflow shut. The bay will drain now.",
     actions=[('ctr', 'QUEST7', 3)])

g = canvas('X', ':')
g[11][0] = g[12][0] = ':'; put(g, 9, 13, 10, 13, ':')
put(g, 1, 3, 17, 3, 'c'); put(g, 1, 1, 18, 2, 'X')
put(g, 2, 4, 2, 8, 'c')
put(g, 6, 4, 6, 6, 'c'); put(g, 11, 4, 11, 8, 'c'); put(g, 16, 4, 16, 10, 'c')
room('sluice_gallery', 'The Sluice Gallery', text(g),
     region=8, music='danger', restored='sluice_gallery_restored', restored_music='works',
     exits=[exit_(0, 11, 1, 2, 'works_gate', 18, 7, 'left'),
            exit_(9, 13, 2, 1, 'workers_bay', 9, 1, 'down')],
     objects=[
         water_src(1, 3, name='reservoir'),
         valve(2, 5, open_=1, name='v_over', hint='overflow_hint'),
         valve(3, 3, open_=0, name='intake', hint='intake_hint'),
         gate(5, 3, inp=['OVER'], invert=1, look='floodgate', name='safety', hint='safety_hint'),
         valve(6, 4, open_=0, name='v_north', hint='north_hint'),
         valve(11, 5, open_=0, name='v_east', hint='east_hint'),
         valve(16, 6, open_=0, name='v_south', hint='south_hint'),
         outlet(6, 6, 'NORTH', name='north', look='none'),
         outlet(11, 8, 'EAST', name='east', look='none'),
         outlet(16, 10, 'SOUTH', name='south', look='none'),
         outlet(2, 8, 'OVER', name='over', look='none'),
         gate(9, 12, w=2, inp=['OVER'], invert=1, look='flood', name='bay_door', hint='bay_door_hint'),
         gate(0, 0, inp=['NORTH', 'EAST', 'SOUTH'], flag_='SLUICES_SET', look='none', name='set',
              actions=[('talk', 'sluices_set')]),
         hazard(4, 9, 24, 8, damage=1, look='steam', name='steam1', hint='steam_hint'),
         hazard(14, 11, 20, 8, damage=1, look='steam', name='steam2', hint='steam_hint'),
         find(1, 12, 'flower', 'Moss Rose', 'flower_red'),
         find(13, 12, 'diagram', 'Sluice Logic Table', 'scroll'),
     ])

# ---- the workers' bay ------------------------------------------------------------------------------------------------
say('bay_arrive', "There they are, on the far side! Hold on, I'm coming!", actions=[('set', 'BRIDGE_UP')])
say('bridge_falls', "Whoa! The bridge just dropped into the water behind me. No going back that way.",
    next='bridge_falls2')
say('bridge_falls2', "Then I go forward. There's a ladder on this side; the workers will know it.")
say('cage_hint', "A jammed safety cage. It needs the counterweight held down and the release lever worked at "
    "the same time.")
say('hold_hint', "The cage's counterweight plate. Something heavy has to stay on it: that crate!")
say('pin_hint', "A locking pin.", "The counterweight's locking pin. Set it, and the weight stays down by itself.")
say('release_hint', "The cage release. A big stiff lever: I'll heave it.", "The cage release. Delicate work.")
talk('workers7', 'WORKER', "You came for us! The ladder's behind that grate: we'll open it. And the Director's "
     "drill is right below... please, stop it before it breaks the wall.",
     cond=('flag', 'CAGE_OPEN'), alt='workers7_trapped', actions=[('set', 'WORKERS_SAFE'), ('ctr', 'QUEST7', 4)])
talk('workers7_trapped', 'WORKER', "Help! The cage jammed when the water came up!")
say('ladder_hint', "A service ladder behind a grate. The workers have the key.")

g = canvas('X', ':')
put(g, 9, 0, 10, 0, ':')
put(g, 1, 5, 18, 8, '~')
put(g, 14, 9, 18, 9, 'I'); put(g, 14, 10, 14, 12, 'I'); g[11][14] = ':'
g[11][0] = g[12][0] = ':'
room('workers_bay', "The Workers' Bay", text(g),
     region=8, music='danger', restored='workers_bay_restored', restored_music='works',
     exits=[exit_(9, 0, 2, 1, 'sluice_gallery', 9, 11, 'up'),
            exit_(0, 11, 1, 2, 'drill_chamber', 18, 11, 'left', cond=('flag', 'WORKERS_SAFE'), locked='ladder_hint')],
     objects=[
         signal_flag('BRIDGE_UP', 'UP', name='up'),
         bridge(9, 5, h=4, inp=['UP'], look='planks', name='gangway'),
         *maiya(plate(4, 11, 'HOLD', weight=2, name='hold', hint='hold_hint'),
                block(6, 11, heavy=1, look='crate', name='bay_crate', hint='hold_hint')),
         *luna(lever(4, 11, 'HOLD', fine=1, name='pin', hint='pin_hint', flag_='BAY_PIN')),
         lever(12, 10, 'FREE', fine=J(0, 1), name='release', hint='release_hint'),
         gate(14, 11, inp=['HOLD', 'FREE'], flag_='CAGE_OPEN', look='woodgate', name='cage', hint='cage_hint'),
         npc(16, 11, 'WORKER', 'workers7', name='worker_a'),
         npc(17, 10, 'WORKER', 'workers7', name='worker_b'),
         gate(1, 11, h=2, flag_='WORKERS_SAFE', look='woodgate', name='grate', hint='ladder_hint'),
         find(17, 1, 'history', "Worker's Lucky Coin", 'coin'),
         find(2, 2, 'history', 'Old Bay Lantern', 'shard'),
     ],
     triggers=[trigger('room', once='BAY_ENTERED', actions=[('talk', 'bay_arrive')]),
               trigger('enter', 9, 9, 1, 1, once='BRIDGE_FALLEN', actions=[('clear', 'BRIDGE_UP'),
                                                                          ('talk', 'bridge_falls')])])

# ---- Valen's drill ----------------------------------------------------------------------------------------------------
say('breaker_hint', "A breaker for the drill's motor. All three must be pulled before it stops.")
say('drill_hint', "Valen's drill, grinding at the reservoir wall. One more day and it would have broken "
     "through.")
say('robot_hint', "A security robot. Its sensor is on top: one crack of the whip there and it switches itself "
    "off.", "A security robot. It sweeps back and forth: I'll slip past between its turns.")
say('drill_stops', "It's stopping... it stopped! The wall held!", actions=[('ctr', 'QUEST7', 5)])
say('blast_hint', "A blast door to the control room. It's interlocked with the drill.")

g = canvas('X', 'm')
put(g, 9, 0, 10, 0, 'm'); g[11][19] = g[12][19] = 'm'; g[3][0] = g[4][0] = 'S'
put(g, 8, 3, 11, 6, 'I'); put(g, 1, 1, 7, 1, 'O'); put(g, 12, 1, 18, 1, 'O')
room('drill_chamber', "Valen's Drill", text(g),
     region=8, music='danger', restored_music='works',
     exits=[exit_(19, 11, 1, 2, 'workers_bay', 1, 11, 'right'),
            exit_(0, 3, 1, 2, 'works_gate', 9, 12, 'left'),
            exit_(9, 0, 2, 1, 'control_room', 9, 12, 'up', cond=('flag', 'DRILL_STOPPED'), locked='blast_hint')],
     objects=[
         lever(3, 8, 'B1', name='breaker1', hint='breaker_hint'),
         lever(9, 9, 'B2', name='breaker2', hint='breaker_hint'),
         lever(16, 8, 'B3', name='breaker3', hint='breaker_hint'),
         gate(0, 0, inp=['B1', 'B2', 'B3'], flag_='DRILL_STOPPED', look='none', name='drill_off',
              actions=[('talk', 'drill_stops')]),
         gate(9, 0, w=2, flag_='DRILL_STOPPED', look='stonedoor', name='blast_door', hint='blast_hint'),
         sign(12, 5, 'drill_hint', look='none'),
         hazard(5, 8, 20, 8, damage=1, look='steam', name='steam3', hint='steam_hint'),
         hazard(14, 8, 24, 8, damage=1, look='steam', name='steam4', hint='steam_hint'),
         creature(6, 11, 'patrol', 'ROBOT1', hits=2, damage=1, speed=8, look='robot', hint='robot_hint'),
         creature(13, 11, 'patrol', 'ROBOT2', hits=2, damage=1, speed=8, look='robot', hint='robot_hint'),
         find(1, 12, 'history', 'Drill Bit Fragment', 'shard'),
         find(17, 3, 'diagram', 'Reservoir Wall Section', 'scroll'),
     ])

# ---- the control room ---------------------------------------------------------------------------------------------------
say('master_hint', "The master lever for the reservoir valve. Big and stiff: a job for strong arms.",
    "The master lever for the reservoir valve. It needs a gentle, steady hand.")
say('press_hint', "The counter-pressure plate. While something holds it down, the valve can move. That crate "
    "would do.")
say('press_pin_hint', "A pin.", "The counter-pressure pin. Set, it holds the pressure plate down for me.")
say('valve_gate_hint', "The reservoir valve itself. Lever and pressure, both at once.")
say('valve_open', "It's open! The reservoir is flowing into the machine. Everything is turning!",
     actions=[('ctr', 'QUEST7', 6)])
say('charter_found', "A charter, sealed by the builders. Every region's share, season by season, measured "
     "by the mill, the survey, the tides, the rows, the river and the stars.")
talk('valen7', 'VALEN', "Well. You have woken the whole system.",
     cond=('flag', 'RESERVOIR_VALVE'), alt='valen7_before', next='valen7_ask')
talk('valen7_ask', 'VALEN', J("So, Maiya: where should the water go?", "So, Luna: where should the water go?"),
     choices=[("All to the central plant", 'plan_central'),
              ("Open every gate at once", 'plan_flood'),
              ("As the charter shares it", 'plan_fair', ('item', 'CHARTER'))])
talk('valen7_before', 'VALEN', "So you stopped my drill. Impressive. Misguided, but impressive. Let's see if you can "
     "even open the valve.")
say('plan_central', "Then the forest roads lose their bog, the coast loses its tide pools, the orchard dries, "
    "and the grotto river stops again. I saw all of it, Director.", next='plan_central2')
talk('plan_central2', 'VALEN', "...I had not seen it from the ground. Ask me again.", next='valen7_ask')
say('plan_flood', "No! The bay floods, the workers' camp floods, and the coast gets it all at once. "
     "Even I know that's too much at once.", next='valen7_ask')
say('plan_fair', "Each region its share, by season and by need, read by the six wonders. The builders "
     "wrote it all down. Here.", next='plan_fair2')
talk('plan_fair2', 'VALEN', "Seasons... need... measured, not decided by an office. Every village has its own "
     "gauge in this.", next='plan_fair3')
talk('plan_fair3', 'VALEN', "I wanted order, and the old system already was order. Just not mine. Very well: we do "
     "it the builders' way. And I will answer to the villages for the drill.",
     actions=[('ctr', 'QUEST7', 7), ('restore', 8), ('collect', 'WONDER7'), ('event', 'RESTORATION')])
talk('valen7_end', 'VALEN', J("My crews will mend the wall and repair what we broke. Under the villages' council, "
                              "not over it. Thank you, Maiya. And your sister.",
                              "My crews will mend the wall and repair what we broke. Under the villages' council, "
                              "not over it. Thank you, Luna. And your sister."),
     cond=('ctr_ge', 'QUEST7', 7), alt='valen7', next='valen7_end2')
say('valen7_end2', "I did it! All seven! Well... Luna helped with the tree. Time to go home and tell "
    "everyone!", "All seven. Maiya will want to hear every bit of it. Time to go home.",
    actions=[('event', 'CHAPTER_END')])

say('reservoir_view', "The reservoir, through the glass: a lake under the whole valley, still and dark.")

g = canvas('X', ':')
put(g, 9, 13, 10, 13, ':')
put(g, 1, 1, 18, 2, 'O')
room('control_room', 'The Control Room', text(g),
     region=8, music='works', restored='control_room_restored', restored_music='ending',
     exits=[exit_(9, 13, 2, 1, 'drill_chamber', 9, 1, 'down')],
     objects=[
         lever(5, 6, 'MASTER', fine=J(0, 1), name='master', hint='master_hint'),
         *maiya(plate(14, 8, 'PRESS', weight=2, name='press', hint='press_hint'),
                block(14, 10, heavy=1, look='crate', name='press_crate', hint='press_hint')),
         *luna(lever(14, 8, 'PRESS', fine=1, name='press_pin', hint='press_pin_hint', flag_='PRESS_PIN')),
         gate(9, 4, w=2, inp=['MASTER', 'PRESS'], flag_='RESERVOIR_VALVE', look='sluicegate', name='valve_gate',
              hint='valve_gate_hint', actions=[('talk', 'valve_open')]),
         chest(2, 10, 'CHARTER', 1, flag_='CHARTER_TAKEN', actions=[('talk', 'charter_found')]),
         npc(14, 5, 'VALEN', 'valen7_end', name='valen7', only=('set', 'DRILL_STOPPED')),
         sign(4, 3, 'reservoir_view', look='none'),
         find(17, 11, 'history', "Valen's Fountain Pen", 'key'),
         find(17, 4, 'photo', 'The Reservoir', 'photo'),
     ])
