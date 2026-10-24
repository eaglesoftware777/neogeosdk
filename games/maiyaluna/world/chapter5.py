"""
Chapter V: The Crystal Grotto Enigma (Crystal Grotto).

North of the crossroads, the Grotto Road climbs to the cave where the
valley's underground river used to run. Dara, a young surveyor, camps at the
mouth: the river stopped when Smoggar's miners dammed it to flood their old
workings, and the air in the deep tunnels went bad when the ventilation
fan stopped.

Inside: daylight from a shaft, bent off three crystal faces, wakes the
crystal node that opens the inner door (the crystals were cut that way: it
is optics, not mystery). In the echo chamber the cave sings every song back
last note first, so the surveyors' recorded echo is sung in reverse. At the
whispering junction the echo of a clap tells a long tunnel from one that
loops back. In the old mine the river is diverted: shut the flood valve and
the passage to the surveyors' hall drains; open the race and the waterwheel
turns the fan, which clears the bad air. The hall keeps the Underground
Survey Instrument and the key to the outflow sluice; with the sluice open,
the river runs out of the grotto again.

Hidden: a crystal garden behind a cracked wall the lens can read.
"""
from worldlib import (room, exit_, add_exit, npc, sign, pickup, chest, rest, secret, gate, gear, creature,
                      trigger, talk, item, chapter, counter, signal_flag, lock, emitter, mirror, receiver,
                      bell, sequence, water_src, valve, outlet, find, collectible, speaker, canvas, put, text,
                      say, J, maiya, luna)

speaker('DARA', 'Dara')

item('INSTRUMENT', 'key', 'Underground Survey Instrument', 'The fifth wonder component: a brass dial that '
     'measures depth by sound.')
item('SLUICEKEY', 'key', 'Outflow Sluice Key', "The key to the grotto river's outflow sluice.")
collectible('WONDER5', 'wonder', 'Underground Survey Instrument')

counter('QUEST5')   # 0 arrived, 1 Dara's news, 2 the crystal door, 3 the echo door, 4 the junction, 5 the passage
                    # drained, 6 the air clear, 7 the instrument, 8 the river runs

chapter(5, 'The Crystal Grotto Enigma', 'Crystal Grotto', 'Underground Survey Instrument',
        start=('grotto_road', 9, 11, 'up'), quest='QUEST5', wonder='INSTRUMENT',
        restore_title='The grotto river runs again',
        restore_text='Clear water pours out of\nthe hills and down to the\nthirsty valley floor.',
        back=('grotto_road', 9, 6, 'down'),
        end_text='Five wonders found. The\nWorld Tree observatory\nwatches over the last.',
        goals=[(0, "Dara the surveyor is camped on the Grotto Road. Ask him about the river."),
               (1, "In the crystal hall, bend the light from the shaft off the crystal faces onto the node."),
               (2, "In the echo chamber the cave sings songs back last note first. Read the tablet."),
               (3, "At the whispering junction, listen to the echoes: only one tunnel goes on."),
               (4, "In the old mine, shut the valve that floods the lower passage to the surveyors' hall."),
               (5, "Search the surveyors' hall: their instrument and the outflow key."),
               (6, "Open the race valve: the waterwheel drives the fan, and the fan clears the bad air."),
               (7, "Unlock the outflow sluice, east of the fan, and let the river run."),
               (8, "The river runs again. Dara is waiting on the Grotto Road.")])

# ---- the Grotto Road ---------------------------------------------------------------------------------
say('ch5_intro', "Listen... that's the sound of a river that isn't there. Just wind in the stones.",
     next='ch5_intro2')
say('ch5_intro2', "There's a tent up by the cave. Someone with a lot of pockets.")
talk('dara5', 'DARA', "You did it! The river is back. Listen to that. My instruments have never been so happy.",
     cond=('ctr_ge', 'QUEST5', 8), alt='dara5_mid', next='dara5_end')
talk('dara5_end', 'DARA', "And you have the old survey instrument. Sefa at the World Tree observatory will want "
     "to see it. The last wonder's drawings point to the tree.", next='dara5_end2')
say('dara5_end2', "The World Tree. Up the forest road, past the research station.", next='dara5_end3')
say('dara5_end3', "I've always wanted to climb that tree!", actions=[('event', 'CHAPTER_END')])
talk('dara5_mid', 'DARA', "The crystals aren't magic, you know. They were cut, long ago, to throw daylight down "
     "the tunnels. And the echoes are a map, if you know how to listen.", cond=('ctr_ge', 'QUEST5', 1),
     alt='dara5_first')
talk('dara5_first', 'DARA', "Hello! I'm Dara, I survey caves. The grotto river stopped last year: the old miners' "
     "dam flooded the workings, and the fan that cleared the air in the deep tunnels died.", next='dara5_b',
     actions=[('ctr', 'QUEST5', 1)])
talk('dara5_b', 'DARA', "The surveyors' hall is down there with all their records. I can't get past the bad "
     "air. You two look like you could.")
talk('road_sign5', 'SIGN', "GROTTO ROAD. NORTH: THE CRYSTAL GROTTO. SOUTH: THE VALLEY CROSSROADS.")
talk('north_locked5', 'SIGN', "NORTH ROAD CLOSED: ROCKFALL. THE CREWS SAY: ONCE THE ORCHARD IS WATERED.")

g = canvas('#', '.')
put(g, 9, 0, 10, 0, 'e'); put(g, 9, 1, 10, 13, '=')
put(g, 1, 1, 3, 3, 'T'); put(g, 16, 1, 18, 3, 'T'); put(g, 1, 9, 2, 12, 'T'); put(g, 17, 10, 18, 12, 'T')
put(g, 5, 4, 6, 5, 'f'); g[4][4] = 'E'
for x, y in ((13, 5), (14, 6), (5, 9), (12, 11), (6, 12)):
    g[y][x] = 'r'
for x, y in ((4, 2), (15, 4), (3, 7), (16, 8), (7, 10)):
    g[y][x] = ','
room('grotto_road', 'The Grotto Road', text(g),
     region=6, music='grotto', restored='grotto_road_restored', restored_music='restored', theme='valley',
     exits=[exit_(9, 13, 2, 1, 'crossroads', 9, 1, 'down'),
            exit_(9, 0, 2, 1, 'crystal_hall', 9, 12, 'up')],
     objects=[
         npc(7, 6, 'DARA', 'dara5', name='dara'),
         sign(11, 12, 'road_sign5', look='signpost'),
         find(17, 5, 'flower', 'Mountain Avens', 'flower_white'),
         find(2, 7, 'flower', 'Alpine Pink', 'flower_red'),
         find(15, 12, 'specimen', 'Rock Crystal', 'crystal'),
     ],
     triggers=[trigger('room', once='CH5_BEGUN', actions=[('talk', 'ch5_intro')])])
add_exit('crossroads', exit_(9, 0, 2, 1, 'grotto_road', 9, 12, 'up', cond=('ctr_ge', 'CHAPTER', 4),
                             locked='north_locked5'))

# ---- the crystal hall ---------------------------------------------------------------------------------
say('lightshaft_hint', "A shaft to the surface. Daylight falls straight down it.")
say('facet_hint', "A crystal face as flat as a mirror. A turns it; light leaves it at a right angle, like "
     "at the lighthouse.")
say('node_hint', "A crystal set in the wall, on a stone base. Light on it will work the door: the "
     "builders cut the crystals to carry it.")
say('crystal_door_hint', "A door of stone, worked by the crystal node.")
say('crystal_door_open', "The node glows and the door slides! It's like the lighthouse latch, only "
     "prettier.", actions=[('ctr', 'QUEST5', 2)])
say('crack5_hint', "Light shows through a crack in this wall. The lens would show me where it's thin.")
say('garden_found', "A whole garden of crystals, lit from above. Nobody has been here for a hundred years.")

g = canvas('X', 'g')
put(g, 9, 13, 10, 13, 'g')
for x, y in ((6, 5), (7, 5), (14, 7), (15, 11), (5, 12), (13, 1), (17, 6), (6, 11)):
    g[y][x] = 'K'
g[6][0] = 'g'; g[10][19] = g[11][19] = 'g'
room('crystal_hall', 'The Crystal Hall', text(g),
     region=6, music='grotto', restored_music='grotto',
     exits=[exit_(9, 13, 2, 1, 'grotto_road', 9, 1, 'down'),
            exit_(19, 10, 1, 2, 'echo_chamber', 1, 7, 'right'),
            exit_(0, 6, 1, 1, 'crystal_garden', 18, 7, 'left')],
     objects=[
         emitter(3, 1, 'down', look='sunbeam', name='shaft', hint='lightshaft_hint'),
         mirror(3, 9, '/', look='facet', name='f1', hint='facet_hint'),
         mirror(10, 9, '\\', look='facet', name='f2', hint='facet_hint'),
         mirror(10, 3, '\\', look='facet', name='f3', hint='facet_hint'),
         receiver(16, 3, 'NODE', look='node', name='node', hint='node_hint'),
         gate(18, 10, h=2, inp=['NODE'], flag_='CRYSTAL_DOOR', look='stonedoor', name='crystal_door',
              hint='crystal_door_hint', actions=[('talk', 'crystal_door_open')]),
         secret(1, 6, 'GARDEN_FOUND', tool='LENS', solid=1, look='crack', hint='crack5_hint',
                actions=[('talk', 'garden_found')]),
         find(13, 12, 'specimen', 'Amethyst Point', 'crystal'),
         find(1, 12, 'photo', 'Light in the Crystal Hall', 'photo'),
         find(17, 1, 'history', "Crystal Cutter's Chisel", 'shard'),
     ])

# ---- the crystal garden (hidden) ------------------------------------------------------------------------
talk('garden_spring', 'SIGN', "Drops fall from the crystals into a clear pool. (Hearts restored; the journey can be "
     "saved here.)")

g = canvas('X', 'g')
g[7][19] = 'g'
for x, y in ((3, 2), (4, 2), (8, 3), (14, 2), (15, 3), (2, 9), (6, 11), (12, 10), (16, 11), (10, 6)):
    g[y][x] = 'K'
put(g, 8, 8, 9, 9, '~')
room('crystal_garden', 'The Crystal Garden', text(g),
     region=6, music='secret', restored_music='secret',
     exits=[exit_(19, 7, 1, 1, 'crystal_hall', 1, 6, 'right')],
     objects=[
         rest(11, 8, text='garden_spring', look='none'),
         find(2, 4, 'flower', 'Cave Moonwort', 'flower_white'),
         find(17, 5, 'flower', 'Glow Fern', 'flower_white'),
         find(4, 11, 'specimen', 'Fluorite Cluster', 'crystal'),
         find(15, 9, 'photo', 'The Crystal Garden', 'photo'),
         find(11, 2, 'flower', 'Pale Cave Orchid', 'flower_violet'),
     ])

# ---- the echo chamber -----------------------------------------------------------------------------------
talk('echo_tablet', 'SIGN', "SURVEYORS' TABLET: THIS CAVE SINGS EVERY SONG BACK, LAST NOTE FIRST. TO OUR SONG IT "
     "SANG: LOW, MIDDLE, LOW, HIGH. THE DOOR OPENS TO OUR SONG.")
say('stal_low', "A long stalactite: the lowest note.")
say('stal_mid', "A middling stalactite: the middle note.")
say('stal_high', "A short stalactite: the highest note.")
say('echo_door_hint', "A stone door with a tuning fork for a latch. It wants the surveyors' song.")
say('echo_solved', "High, low, middle, low: their song, the cave's answer turned around.",
     actions=[('ctr', 'QUEST5', 3)])
say('bat_hint', "Cave bats. They only flap about when disturbed. A crack of my whip sends them back to the roof.",
    "Cave bats. They only flap about when disturbed. I'll tiptoe past.")

g = canvas('X', 'g')
g[7][0] = g[8][0] = 'g'; put(g, 9, 0, 10, 0, 'g')
for x, y in ((3, 10), (16, 10), (2, 2), (17, 2)):
    g[y][x] = 'K'
room('echo_chamber', 'The Echo Chamber', text(g),
     region=6, music='grotto', restored_music='grotto', lit=0,
     exits=[exit_(0, 7, 1, 2, 'crystal_hall', 18, 10, 'left'),
            exit_(9, 0, 2, 1, 'echo_junction', 9, 12, 'up', cond=('flag', 'ECHO_DOOR'), locked='echo_door_hint')],
     objects=[
         bell(5, 4, 0, look='stalactite', name='low', hint='stal_low'),
         bell(10, 3, 1, look='stalactite', name='mid', hint='stal_mid'),
         bell(15, 4, 2, look='stalactite', name='high', hint='stal_high'),
         sequence('song', ['high', 'low', 'mid', 'low'], 'ECHO', flag_='ECHO_SONG', actions=[('talk', 'echo_solved')]),
         gate(9, 0, w=2, inp=['ECHO'], flag_='ECHO_DOOR', look='stonedoor', name='echo_door', hint='echo_door_hint'),
         sign(10, 9, 'echo_tablet', look='tablet'),
         creature(6, 8, 'wander', 'BAT1', hits=1, damage=1, speed=12, look='bat', hint='bat_hint'),
         creature(14, 7, 'wander', 'BAT2', hits=1, damage=1, speed=12, look='bat', hint='bat_hint'),
         find(1, 12, 'flower', 'Bat Flower', 'flower_violet'),
         find(18, 12, 'specimen', 'Cave Pearl', 'shell'),
     ])

# ---- the whispering junction ------------------------------------------------------------------------------
talk('stake_a', 'SIGN', "SURVEY STAKE A. A CLAP HERE COMES BACK AT ONCE, SHORT AND SHARP.")
talk('stake_b', 'SIGN', "SURVEY STAKE B. A CLAP HERE COMES BACK TWICE, QUICKLY.")
talk('stake_c', 'SIGN', "SURVEY STAKE C. A CLAP HERE COMES BACK LATE, LONG AND LOW.")
talk('junction_note', 'SIGN', "DARA'S NOTE: A LATE, LONG ECHO MEANS A LONG OPEN TUNNEL. A QUICK ONE MEANS THE "
     "TUNNEL TURNS BACK ON ITSELF.")
say('junction_back', "Hey! I'm back where I started. That tunnel just goes round in a circle.",
    "Back where I started. That tunnel loops round: the echo was right.")

g = canvas('X', 'g')
g[0][3] = 'e'; g[0][9] = g[0][10] = 'e'; g[0][16] = 'e'
put(g, 9, 13, 10, 13, 'g')
put(g, 6, 1, 7, 6, 'X'); put(g, 12, 1, 13, 6, 'X')
room('echo_junction', 'The Whispering Junction', text(g),
     region=6, music='grotto', restored_music='grotto', lit=0,
     exits=[exit_(9, 13, 2, 1, 'echo_chamber', 9, 1, 'down'),
            exit_(3, 0, 1, 1, 'echo_junction', 16, 11, 'up'),
            exit_(9, 0, 2, 1, 'echo_junction', 3, 11, 'up'),
            exit_(16, 0, 1, 1, 'old_mine', 2, 11, 'right')],
     objects=[
         sign(4, 1, 'stake_a', look='signpost'),
         sign(11, 1, 'stake_b', look='signpost'),
         sign(17, 1, 'stake_c', look='signpost'),
         sign(12, 11, 'junction_note', look='note'),
         find(1, 8, 'flower', 'Liverwort Bloom', 'flower_white'),
         find(18, 8, 'history', 'Surveyor Chain Link', 'coin'),
     ])

# ---- the old mine --------------------------------------------------------------------------------------------
say('valve_flood_hint', "This valve sends the river down to the lower passage. Open, it floods the way "
     "to the surveyors' hall.")
say('valve_race_hint', "The race valve: it would send the river east, to the old waterwheel.")
say('wheel_hint', "A waterwheel geared to the ventilation fan. Water in the race would turn it.")
say('fan_hint', "The ventilation fan. When it turns it pushes fresh air down the east tunnel.")
say('bad_air', "Bad air: stale, heavy, smelling of damp. I can't go through until the fan clears it.")
say('mine_flood_hint', "Flooded to the roof. The river is pouring in from the valve above.")
say('outflow_hint', "The outflow sluice: the river's way out of the grotto. Locked.")
say('outflow_need', "The outflow sluice is locked. The surveyors must have kept the key.")
say('air_clear', "Whoosh! Fresh air! I can breathe all the way down to my toes.",
     actions=[('ctr', 'QUEST5', 7)])
say('mine_arrive', "The old mine. The river runs in over there: that's where Smoggar's miners turned it "
     "into their workings.", actions=[('ctr', 'QUEST5', 4)])
say('river_free', "Listen! It's running out of the hills! Every drop of it!")

g = canvas('X', 'g')
put(g, 1, 1, 18, 2, 'X')
put(g, 1, 3, 18, 3, 'c')
put(g, 8, 4, 8, 11, 'c'); g[9][8] = 'k'
put(g, 15, 4, 15, 12, 'X'); g[7][15] = g[8][15] = 'g'
g[11][0] = g[12][0] = 'g'; g[13][8] = 'g'
room('old_mine', 'The Old Mine', text(g),
     region=6, music='grotto', restored='old_mine_restored', restored_music='restored', lit=0,
     exits=[exit_(0, 11, 1, 2, 'echo_junction', 16, 1, 'left'),
            exit_(8, 13, 1, 1, 'survey_hall', 9, 1, 'down')],
     objects=[
         water_src(1, 3, name='river'),
         valve(8, 5, open_=1, name='flood_valve', hint='valve_flood_hint'),
         valve(10, 3, open_=0, name='race_valve', hint='valve_race_hint'),
         outlet(8, 11, 'FLOOD', name='flood', look='none'),
         outlet(13, 3, 'WHEEL', name='race', look='none'),
         gear(13, 4, 'wheel', role=1, inp=['WHEEL'], mesh=['fan'], look='gear_big', hint='wheel_hint'),
         gear(14, 4, 'fan', role=2, mesh=['wheel'], out=['VENT'], look='fan', hint='fan_hint'),
         gate(15, 7, h=2, inp=['VENT'], flag_='AIR_CLEAR', look='haze', name='bad_air', hint='bad_air',
              actions=[('talk', 'air_clear')]),
         gate(8, 12, inp=['FLOOD'], invert=1, look='flood', name='flooded', hint='mine_flood_hint'),
         lock(17, 3, 'SLUICEKEY', used=1, flag_='OUTFLOW_OPEN', look='valve', name='outflow', hint='outflow_hint',
              text='outflow_need'),
         outlet(18, 3, 'RIVER', name='river_out', look='none'),
         creature(4, 7, 'wander', 'BAT3', hits=1, damage=1, speed=12, look='bat', hint='bat_hint'),
         find(17, 12, 'flower', 'Stonecrop', 'flower_orange'),
         find(3, 5, 'diagram', 'Mine Ventilation Plan', 'scroll'),
         find(12, 11, 'history', "Miner's Lamp", 'shard'),
     ],
     triggers=[trigger('room', once='MINE_SEEN', actions=[('talk', 'mine_arrive')]),
               trigger('watch', once='RIVER_FREE', cond=('signal', 'RIVER'),
                       actions=[('restore', 6), ('ctr', 'QUEST5', 8), ('collect', 'WONDER5'), ('talk', 'river_free'),
                                ('event', 'RESTORATION')])])

# ---- the surveyors' hall --------------------------------------------------------------------------------------
say('instrument_found', "The surveyors' instrument: a brass dial that measures depth by the time an echo "
     "takes. The builders' mark again. The fifth wonder.")
say('key_found', "A big iron key on a hook labelled OUTFLOW. That's what I need!",
     actions=[('ctr', 'QUEST5', 6)])
say('hall_arrive', "The surveyors' hall. Dry, at last: the flood valve did it.",
     actions=[('ctr', 'QUEST5', 5)])
talk('survey_records', 'SIGN', "SURVEY RECORDS: EVERY TUNNEL OF THE GROTTO, MEASURED BY ECHO. THE RIVER'S OLD BED "
     "RUNS EAST AND OUT TO THE VALLEY.")

g = canvas('X', ':')
put(g, 9, 0, 10, 0, ':')
put(g, 2, 2, 5, 2, 'J'); put(g, 14, 2, 17, 2, 'J')
put(g, 3, 6, 4, 7, 'E'); put(g, 15, 6, 16, 7, 'E')
room('survey_hall', "The Surveyors' Hall", text(g),
     region=6, music='secret', restored_music='secret',
     exits=[exit_(9, 0, 2, 1, 'old_mine', 7, 12, 'up')],
     objects=[
         chest(9, 6, 'INSTRUMENT', 1, flag_='INSTRUMENT_TAKEN', actions=[('talk', 'instrument_found')]),
         chest(12, 10, 'SLUICEKEY', 1, flag_='SLUICEKEY_TAKEN', look='crate', actions=[('talk', 'key_found')]),
         sign(9, 3, 'survey_records', look='tablet'),
         find(2, 11, 'diagram', 'Echo Sounding Table', 'scroll'),
         find(17, 11, 'diagram', 'River Bed Survey', 'scroll'),
         find(17, 4, 'history', "Surveyor's Plumb Bob", 'badge'),
     ],
     triggers=[trigger('room', once='HALL_SEEN', actions=[('talk', 'hall_arrive')])])
