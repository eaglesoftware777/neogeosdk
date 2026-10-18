"""
Chapter IV: The Clockwork Orchard (Golden Autumn Grove).

South of the crossroads lies Amberfield, under the Golden Autumn Grove,
where an ancient clockwork once watered the orchards from the terrace
reservoir. Auntie Pell keeps the trees alive with buckets. The clock tower
that drives the orchard's machines lost its gears to Smoggar's crews; with
two of its three sockets filled (the third closes a ring of gears, and a
ring of three can never turn) the tower runs again and so do the platforms
over the long irrigation ditch. Across it, the old harvester needs oil to
clear the fallen branches from the terrace path; on the terraces a
counterbalanced lift rises only when the barrel and a sister weigh its pan
down together. Up top: the reservoir's sluice, the old waterworks with the
Ancient Gear Assembly, and a copper pipe section. Back in the orchard, two
missing pipe sections close the last gaps and the water reaches every row.
"""
from worldlib import (lever, room, exit_, add_exit, npc, sign, pickup, chest, gate, gear, creature, trigger, talk,
                      item, chapter, counter, signal_flag, lock, bridge, platform, block, pan, balance,
                      water_src, valve, outlet, find, collectible, speaker, W, H, say, J, maiya, luna)

speaker('PELL', 'Auntie Pell')

item('GEAR', 'material', 'Brass Gear', 'A clock gear, its teeth worn but true.', stack=3)
item('PIPE', 'material', 'Copper Pipe Section', 'A length of irrigation pipe with a flange at each end.', stack=2)
item('OILCAN', 'tool', 'Oil Can', 'Thin machine oil. Old machines love it.')
item('GEARASSEMBLY', 'key', 'Ancient Gear Assembly', 'The fourth wonder component: a nest of gears, still turning '
     'smooth.')
collectible('WONDER4', 'wonder', 'Ancient Gear Assembly')

counter('QUEST4')   # 0 arrived, 1 Pell's gear, 2 the tower runs, 3 over the ditch, 4 the harvester, 5 the
                    # assembly, 6 the reservoir open, 7 the orchard watered

chapter(4, 'The Clockwork Orchard', 'Golden Autumn Grove', 'Ancient Gear Assembly',
        start=('grove_gate', 9, 2, 'down'), quest='QUEST4', wonder='GEARASSEMBLY',
        restore_title='The orchard drinks again',
        restore_text='Water runs between every\nrow of trees. The apples\nwill be sweet this year.',
        back=('grove_gate', 9, 7, 'down'),
        end_text='Four wonders turn again.\nDara the surveyor writes:\nthe grotto river is dry.',
        goals=[(0, "Find Auntie Pell in Amberfield and ask about the orchard's clockwork."),
               (1, "The clock tower in the orchard is missing gears. Pell had one; find another. Mind the "
                   "clockmaker's rule."),
               (2, "The tower turns the platforms over the long ditch, east of the orchard. Ride them across."),
               (3, "Oil the old harvester in the yard so it clears the branches from the terrace path."),
               (4, J("On the terraces, the lift needs weight on its pan: one barrel is not enough.",
                     "On the terraces, the lift must come up. Look at its frame.")),
               (5, "Open the reservoir sluice on the upper terrace."),
               (6, "Back in the orchard, fit the copper pipe sections where the channels are broken."),
               (7, "The orchard is watered. Auntie Pell is waiting in Amberfield.")])


def canvas(edge='T', fill='.'):
    return [[edge] * W] + [[edge] + [fill] * (W - 2) + [edge] for _ in range(H - 2)] + [[edge] * W]


def put(rows, x0, y0, x1, y1, ch):
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            rows[y][x] = ch


def text(rows):
    return [''.join(r) for r in rows]


# ---- Amberfield ---------------------------------------------------------------------------------------
say('ch4_intro', "Look at the colours! Gold, orange, red... it's like the trees caught fire, but nicely.",
     next='ch4_intro2')
say('ch4_intro2', "The fields are dry, though. And everyone's carrying buckets.")
talk('pell4', 'PELL', "Hear that? Water in every row. Those trees will remember you two for a hundred harvests.",
     cond=('ctr_ge', 'QUEST4', 7), alt='pell4_mid', next='pell4_end')
talk('pell4_end', 'PELL', "And a surveyor from the grotto came by, a nice young man called Dara. The river under "
     "the crystal caves has stopped. He asked for you by name!", next='pell4_end2')
say('pell4_end2', "The Crystal Grotto. North of the crossroads.", next='pell4_end3')
say('pell4_end3', "Caves, crystals and a missing river. Count me in.", actions=[('event', 'CHAPTER_END')])
talk('pell4_mid', 'PELL', "The clockmaker's rule is carved in the tower: three gears touching in a ring can never "
     "turn. Two of the sockets are right, one is a trap.", cond=('ctr_ge', 'QUEST4', 1), alt='pell4_first')
talk('pell4_first', 'PELL', "Oh, visitors! Mind the buckets. Since Smoggar's crews took the gears from the clock "
     "tower, nothing in the orchard moves: not the pumps, not the platforms, not the water.", next='pell4_b')
talk('pell4_b', 'PELL', "I hid one gear under my apron when they came. Here, take it. The orchard is east of the "
     "village, the tower in the middle of it.", actions=[('give', 'GEAR'), ('ctr', 'QUEST4', 1)])
talk('amber_sign', 'SIGN', "AMBERFIELD. NORTH: THE CROSSROADS. EAST: THE CLOCKWORK ORCHARD.")
say('cottage', "Apples drying on strings in the window, and a smell of cinnamon.")
talk('kid4', 'WORKER', "We carry water for the trees every day. My arms are this long now. Well, almost.")

g = canvas()
put(g, 9, 0, 10, 12, '=')
put(g, 9, 6, 19, 7, '=')
put(g, 1, 1, 2, 2, 'A'); put(g, 17, 1, 18, 2, 'A'); put(g, 15, 1, 15, 1, 'A')
put(g, 3, 3, 6, 3, 'R'); put(g, 3, 4, 6, 4, 'H'); g[4][5] = 'D'
put(g, 13, 3, 16, 3, 'R'); put(g, 13, 4, 16, 4, 'H'); g[4][15] = 'D'
put(g, 1, 8, 6, 9, 'F'); put(g, 13, 8, 18, 9, 'F')
for x in (3, 6, 13, 16):
    g[10][x] = 'A'
put(g, 1, 12, 2, 12, 'A'); put(g, 17, 12, 18, 12, 'A')
g[1][8] = ','; g[11][2] = ','; g[11][16] = ','; g[6][2] = ','
g[13][9] = g[13][10] = 'T'
room('grove_gate', 'Amberfield', text(g),
     region=5, music='orchard', restored='grove_gate_restored', restored_music='orchard',
     exits=[exit_(9, 0, 2, 1, 'crossroads', 9, 12, 'up'),
            exit_(19, 6, 1, 2, 'orchard', 1, 6, 'right')],
     objects=[
         npc(6, 6, 'PELL', 'pell4', name='pell'),
         npc(14, 11, 'WORKER', 'kid4', name='kid', wander=1, speed=8),
         sign(11, 2, 'amber_sign', look='signpost'),
         sign(4, 4, 'cottage', look='none'),
         find(1, 5, 'flower', 'Autumn Crocus', 'flower_violet'),
         find(18, 11, 'flower', 'Michaelmas Daisy', 'flower_violet'),
         find(7, 11, 'flower', 'Goldenrod', 'flower_orange'),
         find(18, 5, 'history', 'Harvest Festival Ribbon', 'badge'),
     ],
     triggers=[trigger('room', once='CH4_BEGUN', actions=[('talk', 'ch4_intro')])])
add_exit('crossroads', exit_(9, 13, 2, 1, 'grove_gate', 9, 1, 'down', cond=('ctr_ge', 'CHAPTER', 3),
                             locked='south_locked3'))

# ---- the Clockwork Orchard ------------------------------------------------------------------------------
say('pipe_need', "A section of the copper pipe is missing here. The water can't get past the gap.")
say('pipe_hint', "A broken length of irrigation pipe. A new section would close it.")
say('pipe_set', "Clank, twist, done! Pipe fitting is easy. Lifting the pipe was the hard part.")
say('orchard_dry', "The channels between the rows are bone dry. The water must come from the terraces "
     "above.")
talk('barred_gate', 'SIGN', "A barred gate to the terraces. It can only be opened from the terrace side.")
say('wasp_hint4', "Wasps drunk on fallen apples. Grumpy, but a crack of the whip clears them off.")
talk('tower_sign', 'SIGN', "THE ORCHARD CLOCK TOWER. IT TURNS THE PUMPS, THE PLATFORMS AND THE SEASONS (THE LAST "
     "ONE IS A JOKE).")
talk('orchard_watered', 'PELL', "Water! Water in the rows!", actions=[('ctr', 'QUEST4', 7)])
say('gear_found', "A brass gear, packed in straw. Someone hid it from the crews, too.")

g = canvas()
put(g, 5, 0, 13, 3, 'l'); g[3][9] = 'd'
put(g, 1, 1, 2, 2, 'A'); put(g, 16, 1, 17, 2, 'A')
put(g, 9, 4, 9, 12, '=')
put(g, 0, 6, 18, 7, '='); g[6][19] = 'T'
put(g, 4, 5, 19, 5, 'c'); put(g, 4, 5, 4, 11, 'c')
put(g, 4, 8, 16, 8, 'c'); put(g, 4, 11, 16, 11, 'c')
g[5][9] = 'k'; g[6][4] = g[7][4] = 'k'; g[8][9] = 'k'; g[11][9] = 'k'
for x in range(5, 17):
    if x != 9:
        g[10][x] = 'A' if x % 2 else g[10][x]
        g[12][x] = 'A' if x % 2 else g[12][x]
put(g, 10, 9, 19, 9, '='); put(g, 10, 10, 19, 10, '=')
for x in (5, 7):
    g[9][x] = 'A'
g[3][19] = g[4][19] = '='
g[4][18] = '.'
room('orchard', 'The Clockwork Orchard', text(g),
     region=5, music='orchard', restored='orchard_restored', restored_music='orchard',
     exits=[exit_(0, 6, 1, 2, 'grove_gate', 18, 6, 'left'),
            exit_(9, 3, 1, 1, 'clocktower', 9, 12, 'up'),
            exit_(19, 9, 1, 2, 'ditch', 1, 6, 'right'),
            exit_(19, 3, 1, 2, 'terraces', 18, 3, 'right', cond=('flag', 'RESERVOIR_OPEN'), locked='barred_gate')],
     objects=[
         signal_flag('RESERVOIR_OPEN', 'RES', name='res'),
         water_src(19, 5, inp=['RES'], name='inflow'),
         lock(14, 5, 'PIPE', used=1, flag_='PIPE_MAIN', look='none', name='gap_main', hint='pipe_hint',
              text='pipe_need', actions=[('talk', 'pipe_set')]),
         lock(4, 10, 'PIPE', used=1, flag_='PIPE_SOUTH', look='none', name='gap_south', hint='pipe_hint',
              text='pipe_need', actions=[('talk', 'pipe_set')]),
         outlet(16, 8, 'ROW_A', name='row_a', look='none'),
         outlet(16, 11, 'ROW_B', name='row_b', look='none'),
         gate(0, 13, inp=['ROW_A', 'ROW_B'], flag_='ORCHARD_WATERED', look='none', name='watered'),
         sign(3, 4, 'tower_sign', look='signpost'),
         chest(18, 2, 'GEAR', 1, flag_='ORCHARD_GEAR', look='crate', actions=[('talk', 'gear_found')]),
         creature(12, 9, 'wander', 'WASP3', hits=2, damage=1, speed=10, look='wasp', hint='wasp_hint4'),
         creature(6, 6, 'wander', 'WASP4', hits=2, damage=1, speed=10, look='wasp', hint='wasp_hint4'),
         find(3, 1, 'flower', 'Heather', 'flower_violet'),
         find(16, 12, 'specimen', 'Russet Apple Seed', 'acorn'),
         find(1, 12, 'photo', 'The Orchard in Autumn', 'photo'),
         find(18, 12, 'history', 'Old Cider Press Plate', 'badge'),
     ],
     triggers=[trigger('room', once='ORCHARD_SEEN', actions=[('talk', 'orchard_dry')]),
               trigger('watch', once='ORCHARD_RESTORED', cond=('flag', 'ORCHARD_WATERED'),
                       actions=[('restore', 5), ('collect', 'WONDER4'), ('talk', 'orchard_watered'),
                                ('event', 'RESTORATION')])])

# ---- the clock tower ----------------------------------------------------------------------------------------
talk('rule_sign', 'SIGN', "CARVED OVER THE CLOCKWORK: THE CLOCKMAKER'S RULE. THREE GEARS TOUCHING IN A RING CAN "
     "NEVER TURN.")
say('weight_hint', "The weight drum. It turns the first gear: if the gears let it.")
say('socket_a_hint', "A gear socket touching the weight drum's gear.")
say('socket_b_hint', "A gear socket between the first socket and the tower's main shaft.")
say('socket_c_hint', "This socket touches both of the others. Three gears in a ring... the rule!")
say('shaft_hint', "The main shaft: from here the clock drives the pumps and the ditch platforms.")
say('tower_runs', "It's turning! Clunk, clunk, clunk. That's the best sound I've heard all day.",
     actions=[('ctr', 'QUEST4', 2)])

g = canvas('l', 'f')
g[13][9] = g[13][10] = 'd'
put(g, 2, 9, 3, 10, 'J'); put(g, 16, 9, 17, 10, 'J')
room('clocktower', 'The Clock Tower', text(g),
     region=5, music='lighthouse', restored_music='lighthouse',
     exits=[exit_(9, 13, 2, 1, 'orchard', 9, 4, 'down')],
     objects=[
         gear(7, 4, 'weight', role=1, mesh=['sock_a'], look='gear_big', hint='weight_hint'),
         gear(8, 4, 'sock_a', role=0, mesh=['weight', 'sock_b', 'sock_c'], item_='GEAR', look='gear',
              hint='socket_a_hint', flag_='TOWER_A'),
         gear(9, 4, 'sock_b', role=0, mesh=['sock_a', 'sock_c', 'shaft'], item_='GEAR', look='gear',
              hint='socket_b_hint', flag_='TOWER_B'),
         gear(9, 3, 'sock_c', role=0, mesh=['sock_a', 'sock_b'], item_='GEAR', look='gear', hint='socket_c_hint'),
         gear(10, 4, 'shaft', role=2, mesh=['sock_b'], out=['TOWER'], look='gear_big', hint='shaft_hint'),
         sign(13, 2, 'rule_sign', look='tablet'),
         find(17, 2, 'diagram', 'Tower Gear Train', 'scroll'),
         find(2, 12, 'history', "Clockmaker's Loupe", 'lensitem'),
     ],
     triggers=[trigger('watch', once='TOWER_RUNNING', cond=('signal', 'TOWER'), actions=[('talk', 'tower_runs')])])

# ---- the long ditch ----------------------------------------------------------------------------------------
say('ditch_still', "The platforms are stuck. They must run from the clock tower.")
say('platform_hint', "A ferry platform on a chain. It waits a moment at each end: step on when it's beside "
     "you.")
talk('ditch_sign', 'SIGN', "LONG DITCH FERRY. RUNS WITH THE TOWER CLOCK. CHANGE PLATFORMS ON THE ISLAND.")

g = canvas()
put(g, 5, 0, 8, 13, '~'); put(g, 11, 0, 14, 13, '~')
g[6][0] = g[7][0] = '='; g[6][19] = g[7][19] = '='
g[3][9] = 'A'; g[10][10] = 'A'
room('ditch', 'The Long Ditch', text(g),
     region=5, music='orchard', restored_music='orchard',
     exits=[exit_(0, 6, 1, 2, 'orchard', 18, 9, 'left'),
            exit_(19, 6, 1, 2, 'harvester_yard', 1, 6, 'right')],
     objects=[
         signal_flag('TOWER_RUNNING', 'TOWER', name='tower'),
         platform(5, 6, 3, 0, speed=6, wait=40, inp=['TOWER'], look='basket', name='ferry_west', hint='platform_hint'),
         platform(11, 7, 3, 0, speed=6, wait=40, inp=['TOWER'], look='basket', name='ferry_east', hint='platform_hint'),
         sign(3, 4, 'ditch_sign', look='signpost'),
         find(10, 2, 'flower', 'Water Mint', 'flower_violet'),
         find(17, 11, 'flower', 'Purple Loosestrife', 'flower_violet'),
         find(2, 11, 'photo', 'Dragonflies over the Ditch', 'photo'),
     ],
     triggers=[trigger('room', once='DITCH_SEEN', cond=('not', 'TOWER_RUNNING'), actions=[('talk', 'ditch_still')]),
               trigger('enter', 15, 1, 4, 12, once='DITCH_CROSSED', actions=[('ctr', 'QUEST4', 3)])])

# ---- the harvester yard ---------------------------------------------------------------------------------------
say('harvester_hint', "The harvester's oil cap. Everything inside is dry as dust: no wonder it seized.")
say('harvester_need', "The harvester has seized up. It needs oil before it will turn.")
say('harvester_runs', "It's alive! It's chewing through the branches like they're breakfast!",
     actions=[('ctr', 'QUEST4', 4)])
say('branches_hint', "Storm-felled branches, a whole tangle. Only the harvester could clear that.")
say('oil_found', "An oil can, half full. The harvester will like this.")
talk('yard_sign', 'SIGN', "HARVESTER YARD. SOUTH: THE TERRACES AND THE RESERVOIR.")

g = canvas()
put(g, 2, 1, 5, 1, 'R'); put(g, 2, 2, 5, 2, 'H'); g[2][4] = 'D'
put(g, 7, 3, 12, 5, 'I')
put(g, 10, 6, 10, 12, '='); put(g, 0, 6, 9, 7, '=')
g[6][0] = g[7][0] = '='
put(g, 1, 10, 7, 12, 'A'); put(g, 13, 10, 18, 12, 'A')
for x in (3, 5, 15, 17):
    g[9][x] = 'A'
g[13][9] = g[13][10] = '='
put(g, 9, 12, 10, 12, '=')
room('harvester_yard', 'The Harvester Yard', text(g),
     region=5, music='orchard', restored_music='orchard',
     exits=[exit_(0, 6, 1, 2, 'ditch', 18, 6, 'left'),
            exit_(9, 13, 2, 1, 'terraces', 9, 12, 'down')],
     objects=[
         lock(13, 4, 'OILCAN', used=1, flag_='HARVESTER_OILED', look='valve', name='oil_cap', hint='harvester_hint',
              text='harvester_need', actions=[('set', 'BRANCHES_CLEARED'), ('talk', 'harvester_runs')]),
         gate(9, 12, w=2, flag_='BRANCHES_CLEARED', look='rocks', name='branches', hint='branches_hint'),
         chest(3, 4, 'OILCAN', 1, flag_='OIL_CHEST', look='crate', actions=[('talk', 'oil_found')]),
         sign(15, 7, 'yard_sign', look='signpost'),
         pickup(17, 7, 'PIPE', 1, flag_='YARD_PIPE', look='pipeitem'),
         pickup(2, 8, 'GEAR', 1, flag_='YARD_GEAR', look='cogitem'),
         find(18, 1, 'flower', 'Wild Marjoram', 'flower_violet'),
         find(13, 8, 'diagram', 'Harvester Drawing', 'scroll'),
         find(1, 9, 'history', 'Copper Harvest Token', 'coin'),
     ])

# ---- the orchard terraces -----------------------------------------------------------------------------------
say('lift_hint', "The terrace lift. Its counterweight pan is down here: enough weight on it and the lift "
     "comes up to this level.")
say('pan_hint', "The lift's pan. One barrel alone isn't heavy enough. Two would do it.",
    "The lift's pan. Those barrels are far too heavy for me to roll. That crank on the lift's frame, though...")
say('counter_hint', "The other pan, for lowering things. Weight here pulls the lift back down.")
say('barrel_hint', "A barrel of rainwater, heavy. I can roll it.", "A barrel of rainwater. Far too heavy for me.")
say('lift_teach', "The lift came up! With both barrels on the pan, it stays up. Up I go!")
say('crank_hint', "A little crank with a fiddly ratchet. Not my kind of thing.",
    "A hand crank on the lift's frame, with a ratchet. Turned slowly, the lift climbs and stays.")
say('crank_done', "The lift's up.", "Click, click, click... the lift is up, and the ratchet holds it.")
say('sluice_hint', "The reservoir sluice. The channel runs down to the orchard.")
say('reservoir_open', "Listen to it rush! The orchard gate up here unbars now, too.",
     actions=[('ctr', 'QUEST4', 6)])
say('assembly_found', "In the old waterworks: a nest of gears that turns as smooth as silk. The builders' "
     "mark again. The fourth wonder.", actions=[('ctr', 'QUEST4', 5)])

g = canvas()
put(g, 1, 6, 18, 7, '#'); g[6][9] = g[7][9] = 'p'
for x in range(1, 19, 2):
    g[1][x] = 'A'
put(g, 1, 2, 17, 2, 'c')
g[3][19] = g[4][19] = '='
put(g, 9, 8, 9, 12, '='); g[13][9] = g[13][10] = '='
for x in (1, 3, 5, 13, 15, 17):
    g[11][x] = 'A'
room('terraces', 'The Orchard Terraces', text(g),
     region=5, music='orchard', restored='terraces_restored', restored_music='orchard',
     exits=[exit_(9, 13, 2, 1, 'harvester_yard', 9, 11, 'up'),
            exit_(19, 3, 1, 2, 'orchard', 18, 3, 'right')],
     objects=[
         block(4, 9, heavy=1, look='barrel', name='barrel', hint='barrel_hint'),
         *maiya(block(6, 9, heavy=1, look='barrel', name='barrel2', hint='barrel_hint')),
         pan(4, 10, 'liftpan', w=2, hint='pan_hint'),
         pan(14, 10, 'counterpan', w=2, hint='counter_hint'),
         balance('lift_balance', 'liftpan', 'counterpan', 'LIFT', threshold=5),
         *maiya(bridge(9, 6, h=2, inp=['LIFT'], look='planks', name='lift', hint='lift_hint')),
         *luna(bridge(9, 6, h=2, inp=['CRANK'], look='planks', name='lift', hint='lift_hint'),
               lever(10, 8, 'CRANK', fine=1, look='lever', name='crank', hint='crank_hint', flag_='LIFT_CRANKED',
                     actions=[('talk', 'crank_done')])),
         water_src(1, 2, name='reservoir'),
         valve(5, 2, open_=0, name='sluice', hint='sluice_hint'),
         outlet(17, 2, 'DOWN', name='down', look='none'),
         chest(14, 4, 'GEARASSEMBLY', 1, flag_='ASSEMBLY_TAKEN', actions=[('talk', 'assembly_found')]),
         pickup(3, 4, 'PIPE', 1, flag_='TERRACE_PIPE', look='pipeitem'),
         find(16, 4, 'flower', 'Autumn Gentian', 'flower_violet'),
         find(17, 12, 'flower', 'Wild Strawberry Flower', 'flower_white'),
         find(1, 12, 'photo', 'The Valley from the Terraces', 'photo'),
         find(2, 4, 'diagram', 'Reservoir Sluice Plan', 'scroll'),
     ],
     triggers=[trigger('watch', once='LIFT_TAUGHT', cond=('signal', 'LIFT'), actions=[('talk', 'lift_teach')]),
               trigger('watch', once='RESERVOIR_OPEN', cond=('signal', 'DOWN'), actions=[('talk', 'reservoir_open')])])
