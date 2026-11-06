"""
Chapter III: The Lighthouse of Seven Mirrors (Azure Coral Coast).

The valley's roads meet at the crossroads west of Willowbrook; the coast
road leads to Coral Harbour, where Captain Orin's boats stay tied up: the
old lighthouse on the headland went dark when Smoggar's crews jammed its
works, and no ship can find the harbour mouth at night.

Keeper Marra waits on the headland. The tower door opens only to sunlight on
its heliograph latch: four mirrors on posts carry the morning sun to it.
Inside, the clock that turns the lamp and works the gallery shutters has
lost its escapement wheel; Marra's spare is in the supply cave, which the
sea uncovers only at low water, and Orin can read the tide table. With the
clock running, the gallery's three mirrors send the sunlight to the latch of
the keepers' sealed chamber, where the old builders' Precision Optical
Lens is kept; set in the lamp's cradle it focuses the beam on the burner,
and the coast has its light again.
"""
from worldlib import (room, exit_, add_exit, npc, sign, pickup, chest, rest, gate, gear, creature, trigger,
                      talk, item, chapter, counter, signal_flag, lock, bridge, emitter, mirror, lens, receiver,
                      find, collectible, speaker, say, J, maiya, luna)

speaker('MARRA', 'Keeper Marra')
speaker('WORKER', 'Road Crew')

item('TIDETABLE', 'record', 'Tide Table', "The keepers' tide table: high and low water for every day of the year.")
item('ESCAPEMENT', 'key', 'Escapement Wheel', 'The toothed wheel that lets a clock tick one step at a time.')
item('OPTLENS', 'key', 'Precision Optical Lens', 'The old builders\' lens: the third of the seven wonder components.')
collectible('WONDER3', 'wonder', 'Precision Optical Lens')

counter('QUEST3')   # 0 arrived, 1 Orin's news, 2 Marra's mirrors, 3 in the tower, 4 low water, 5 the escapement,
                    # 6 the clock runs, 7 the chamber's lens, 8 the light shines

chapter(3, 'The Lighthouse of Seven Mirrors', 'Azure Coral Coast', 'Precision Optical Lens',
        start=('harbour', 17, 6, 'left'), quest='QUEST3', wonder='OPTLENS',
        restore_title='The lighthouse shines again',
        restore_text='Its beam sweeps the coral\ncoast. Tonight the boats\nwill find the harbour.',
        back=('harbour', 10, 7, 'down'),
        end_text='Captain Orin is sailing\nagain. Sunboy has found\na map of an old orchard.',
        goals=[(0, "Captain Orin is down by the harbour pier. Ask him about the dark lighthouse."),
               (1, "Climb the path north to the headland and find the lighthouse keeper."),
               (2, "Turn the mirrors on the headland so the morning sun reaches the door's latch."),
               (3, "The clock needs an escapement wheel, kept in the low-tide supply cave. Show Captain Orin "
                   "the tide table."),
               (4, "The tide is out: cross the sandbar at the tide pools, west of the harbour, to the supply "
                   "cave."),
               (5, "Bring the escapement wheel to the lighthouse clock. Luna's hands can fit it."),
               (6, "Up in the lamp gallery, send the sunlight to the latch of the sealed chamber."),
               (7, "Set the precision lens in the lamp's cradle and focus the beam on the burner."),
               (8, "The lighthouse shines. Captain Orin is waiting at the pier.")])

# ---- the valley crossroads -------------------------------------------------------------------
talk('cross_sign', 'SIGN', "VALLEY CROSSROADS. EAST: WILLOWBROOK. WEST: CORAL HARBOUR. SOUTH: GOLDEN AUTUMN GROVE. "
     "NORTH: THE CRYSTAL GROTTO.")
talk('crew_cross', 'WORKER', "The station's mast said the coast road was safe, so here we are. Every road crew in "
     "the valley owes you two a lunch.")
talk('west_locked', 'SIGN', "The road crews are still clearing fallen trees from the west road.")
talk('south_locked3', 'SIGN', "SOUTH ROAD CLOSED: A BRIDGE IS OUT. THE CREWS SAY: NOT BEFORE THE COAST IS LIT.")
talk('north_locked3', 'SIGN', "NORTH ROAD CLOSED: ROCKFALL. THE CREWS ARE ON IT.")
talk('cross_well', 'SIGN', "An old drinking fountain fed by a spring. (Hearts restored; the journey can be saved here.)")

room('crossroads', 'Valley Crossroads', [
    "TTTTTTTTT==TTTTTTTTT",
    "T.......,==...,....T",
    "T..,.....==........T",
    "T...TT...==...TT...T",
    "T...TT...==...TT..,T",
    "T..,....====.......T",
    "=========....=======",
    "=========....=======",
    "T.......====...,...T",
    "T...TT...==...TT...T",
    "T...TT...==...TT...T",
    "T..,.....==.....,..T",
    "T........==........T",
    "TTTTTTTTT==TTTTTTTTT"],
    region=1, music='village',
    exits=[exit_(19, 6, 1, 2, 'village', 1, 6, 'right'),
           exit_(0, 6, 1, 2, 'harbour', 18, 5, 'left')],
    objects=[
        sign(13, 8, 'cross_sign', look='signpost'),
        rest(11, 6, text='cross_well', look='well'),
        npc(16, 5, 'WORKER', 'crew_cross', name='crew'),
        find(1, 1, 'flower', 'Cornflower', 'flower_violet'),
        find(18, 12, 'flower', 'Red Poppy', 'flower_red'),
        find(2, 11, 'history', 'Old Milestone Plate', 'badge'),
    ])

# ---- Coral Harbour ------------------------------------------------------------------------------
say('ch3_intro', "Smell that? Salt and seaweed. Look at all the boats, though. Nobody's sailing.",
     next='ch3_intro2')
say('ch3_intro2', "There's Captain Orin on the pier. Time to find out why.")
talk('orin3', 'ORIN', "Look at her up there. The lighthouse is shining again! My boats go out tonight, and they'll "
     "come home too.", cond=('ctr_ge', 'QUEST3', 8), alt='orin3_lens', next='orin3_end')
talk('orin3_end', 'ORIN', "And your brother came running with an old drawing of an orchard full of clockwork. He "
     "says it's the fourth wonder.", next='orin3_end2')
say('orin3_end2', "Clockwork apples? I have to see that.", next='orin3_end3')
say('orin3_end3', "Golden Autumn Grove. South of the crossroads. Thank you, Captain.",
     actions=[('event', 'CHAPTER_END')])
talk('orin3_lens', 'ORIN', "The tide is out, but not for long. The sandbar runs from the tide pools, west of here, "
     "to the old supply cave.", cond=('ctr_ge', 'QUEST3', 4), alt='orin3_tide')
talk('orin3_tide', 'ORIN', "Marra's tide table? Let me see. Ha! Low water is right about... now. The sandbar to "
     "the supply cave is dry. Go, before the sea changes its mind.", cond=('item', 'TIDETABLE'),
     alt='orin3_first', actions=[('set', 'LOW_TIDE'), ('ctr', 'QUEST3', 4)])
talk('orin3_first', 'ORIN', "Ahoy, girls. You see a harbour full of boats and nobody sailing? The lighthouse on the "
     "headland went dark when Smoggar's crews jammed its works.", next='orin3_b')
talk('orin3_b', 'ORIN', "Without the light, the reef eats boats at night. Keeper Marra is up on the headland, "
     "north of here. She knows that tower like her own hands.", actions=[('ctr', 'QUEST3', 1)])
talk('fisher', 'WORKER', "I mend nets while the boats stay in. My nets have never been so tidy. I'd rather be "
     "fishing.")
talk('harbour_sign', 'SIGN', "CORAL HARBOUR. MIND THE REEF. NO SAILING WITHOUT THE LIGHT.")
say('harbour_house', "A fisher's cottage. Shells along the windowsill, a kettle on the stove.")

room('harbour', 'Coral Harbour', [
    "########==##########",
    "#RRRR...==....RRRR.#",
    "#HHDH...==....HDHH.#",
    "#.......==.........#",
    "#...,...=====......#",
    "#...........=....,.=",
    "#ssss.......=......=",
    "ssssss......=sssssss",
    "sssssssffffffssssss#",
    "OOOOOOOffffffOOOOOO#",
    "OOUUUOOffffffOOUUUO#",
    "OOUUUOOOOffOOOOUUUO#",
    "OOOOOOOOOffOOOOOOOO#",
    "OOOOOOOOOOOOOOOOOOOO"],
    region=4, music='coast', restored='harbour_restored', restored_music='coast',
    exits=[exit_(19, 5, 1, 2, 'crossroads', 1, 6, 'right'),
           exit_(8, 0, 2, 1, 'headland', 7, 12, 'up'),
           exit_(0, 7, 1, 2, 'tidepools', 18, 7, 'left')],
    objects=[
        npc(10, 9, 'ORIN', 'orin3', name='orin'),
        npc(4, 7, 'WORKER', 'fisher', name='fisher'),
        sign(6, 3, 'harbour_sign', look='signpost'),
        sign(3, 2, 'harbour_house', look='none'),
        find(18, 1, 'flower', 'Sea Thrift', 'flower_red'),
        find(1, 3, 'flower', 'Sea Kale', 'flower_white'),
        find(12, 10, 'history', "Ship's Bell", 'coin'),
        find(17, 4, 'photo', 'Boats in Coral Harbour', 'photo'),
        find(9, 12, 'specimen', 'Sea Glass', 'crystal'),
    ],
    triggers=[trigger('room', once='CH3_BEGUN', actions=[('talk', 'ch3_intro')])])

# ---- the Lighthouse Headland ---------------------------------------------------------------------
talk('marra3', 'MARRA', "The tower door is open and I can hear the clock. Bless you both.",
     cond=('flag', 'DOOR_OPEN'), alt='marra3_first')
talk('marra3_first', 'MARRA', "Visitors! Orin sent you? Good. The lighthouse door has a heliograph latch: it opens "
     "only when sunlight falls on that brass plate by the wall.", next='marra3_b',
     actions=[('ctr', 'QUEST3', 2)])
talk('marra3_b', 'MARRA', "The builders set four mirrors on posts to carry the morning sun to it. Smoggar's crews "
     "knocked them all askew. My old knees can't do the turning anymore.", next='marra3_c')
say('marra3_c', "Light bounces off a mirror like a ball off a wall. I just follow the beam!",
    "Light leaves a mirror at the same angle it arrives. I just follow the beam.")
say('sun_hint', "A gap in the rocks where the morning sun shines through, low over the hills.")
say('mirror_hint', "A mirror on a post. A turns it a quarter round. The light goes off at a right angle.")
say('latch_hint', "The heliograph latch: a brass plate. When sunlight falls on it, the door unlocks.")
say('tower_door_hint', "The lighthouse door. Locked tight until the latch sees the sun.")
say('door_opens', "Click! The door swung open by itself. Sunlight and mirrors. I take back everything "
     "I said about mirrors.",
    luna_text="Click! The door swung open by itself. Sunlight and mirrors, just as Marra said.", actions=[('ctr', 'QUEST3', 3)])

room('headland', 'Lighthouse Headland', [
    "OOOOOO#####OOOOOOOOO",
    "OOOOss.....sllllllsO",
    "OOOss.......llllll.O",
    "OOss........llllll.O",
    "Oss.........llllll.O",
    "Os..........llldll.O",
    "Os.....,..........sO",
    "Os................sO",
    "Os..r.............sO",
    "Os..rr....,.......sO",
    "Os............r...sO",
    "Os.,..............sO",
    "Oss...............sO",
    "OOsss==ssssssssssOOO"],
    region=4, music='coast', restored_music='coast',
    exits=[exit_(5, 13, 2, 1, 'harbour', 8, 1, 'down'),
           exit_(15, 5, 1, 1, 'lighthouse', 9, 12, 'up', cond=('flag', 'DOOR_OPEN'), locked='tower_door_hint')],
    objects=[
        emitter(2, 12, 'up', name='sun', hint='sun_hint'),
        mirror(2, 7, '\\', name='h1', hint='mirror_hint'),
        mirror(8, 7, '\\', name='h2', hint='mirror_hint'),
        mirror(8, 3, '\\', name='h3', hint='mirror_hint'),
        mirror(5, 3, '/', name='h4', hint='mirror_hint'),
        receiver(11, 3, 'LATCH', name='latch', hint='latch_hint'),
        gate(15, 5, inp=['LATCH'], flag_='DOOR_OPEN', look='woodgate', name='door', hint='tower_door_hint',
             actions=[('talk', 'door_opens')]),
        npc(10, 6, 'MARRA', 'marra3', name='marra'),
        find(17, 12, 'flower', 'Sea Holly', 'flower_violet'),
        find(5, 1, 'flower', 'Sea Campion', 'flower_white'),
        find(16, 9, 'photo', 'The Lighthouse at Dusk', 'photo'),
    ])

# ---- the clock room ---------------------------------------------------------------------------------
say('clock_hint', "The clock's escapement: the toothed wheel that lets the weight fall one tick at a "
     "time. The housing is empty.")
say('drum_hint', "The weight drum. The clock weight hangs on it and wants to turn it.")
say('drive_hint', "The lamp drive. It turns the lamp and works the shutters in the gallery above.")
talk('clock_note', 'SIGN', "KEEPER'S NOTE: SPARE ESCAPEMENT IN THE SUPPLY CAVE. LOW WATER ONLY. ASK ORIN TO READ "
     "THE TABLE, HE NEVER GETS IT WRONG.")
say('tide_found', "Marra's tide table. Captain Orin will know what to make of it.",
     actions=[('ctr', 'QUEST3', 3)])
say('clock_runs', "Tick... tock... it's running! Something clanked upstairs, too.")

room('lighthouse', 'The Clock Room', [
    "llllllllllllllllllll",
    "l::::::::::::::::S:l",
    "l::::::::::::::::::l",
    "l::J:::::::::::::::l",
    "l::J:::::::::::::::l",
    "l::::::::::::::::::l",
    "l::::::::::::::::::l",
    "l::::::::::::::::::l",
    "l::::::::::::::E:::l",
    "l::::::::::::::E:::l",
    "l::::::::::::::::::l",
    "l::::::::::::::::::l",
    "l::::::::::::::::::l",
    "lllllllllddlllllllll"],
    region=4, music='lighthouse', restored_music='lighthouse',
    exits=[exit_(9, 13, 2, 1, 'headland', 15, 6, 'down'),
           exit_(17, 1, 1, 1, 'lamp_room', 17, 11, 'up')],
    objects=[
        gear(8, 5, 'drum', role=1, mesh=['escapement'], look='gear_big', hint='drum_hint'),
        gear(9, 5, 'escapement', role=0, mesh=['drum', 'drive'], item_='ESCAPEMENT', fine=J(0, 1), look='gear',
             hint='clock_hint', flag_='ESC_SET'),
        gear(10, 5, 'drive', role=2, mesh=['escapement'], out=['CLOCK'], look='gear_big', hint='drive_hint'),
        chest(4, 9, 'TIDETABLE', 1, flag_='TIDE_CHEST', actions=[('talk', 'tide_found')]),
        sign(13, 8, 'clock_note', look='note'),
        find(17, 11, 'diagram', 'Escapement Drawing', 'scroll'),
        find(2, 11, 'history', "Keeper's Lantern", 'shard'),
    ],
    triggers=[trigger('watch', once='CLOCK_RUNNING', cond=('signal', 'CLOCK'),
                      actions=[('ctr', 'QUEST3', 6), ('talk', 'clock_runs')])])

# ---- the lamp gallery ----------------------------------------------------------------------------------
say('window_hint', "Sunlight through the gallery window. The shutters move with the clock.")
say('shutter_hint', "The gallery shutters. The clock opens them.")
say('hatch_latch_hint', "Another heliograph latch, for the hatch in the floor: the keepers' sealed chamber.")
say('hatch_hint', "A hatch in the floor, locked by the latch on the far wall.")
say('cradle_hint', "The lamp's lens cradle. It's empty: the lens that belongs here is gone.")
say('cradle_need', "The cradle is empty. Whatever lens belongs here must be somewhere in this tower.")
say('glass_hint', "The condenser glass. Light that passes through comes out focused, hot enough to light "
     "the burner.")
say('burner_hint', "The lamp's burner. Only focused light will make it catch.")
say('cradle_set', "The precision lens sits in the cradle as if it was made for it. It was, a long time ago.",
     actions=[('ctr', 'QUEST3', 7)])

room('lamp_room', 'The Lamp Gallery', [
    "llllllllllllllllllll",
    "l::::::::::::::::::l",
    "l::::::::::::::::::l",
    "l::::::::::::::::::l",
    "l::::::::::::::::::l",
    "l::::::::::::::::::l",
    "l::::::::::::::::::l",
    "l::::::::::::::::::l",
    "l::::::::::::::::::l",
    "l::::::::::::::::::l",
    "l::::::::::::::::::l",
    "l::::::::::::::::::l",
    "l::::::::::::::::S:l",
    "llllllllllllllllllll"],
    region=4, music='lighthouse', restored_music='lighthouse',
    exits=[exit_(17, 12, 1, 1, 'lighthouse', 17, 2, 'down'),
           exit_(3, 12, 1, 1, 'sealed_chamber', 9, 2, 'down', cond=('flag', 'HATCH_OPEN'), locked='hatch_hint')],
    objects=[
        signal_flag('CLOCK_RUNNING', 'CLOCK', name='clock'),
        emitter(18, 6, 'left', name='window', hint='window_hint'),
        gate(17, 6, inp=['CLOCK'], look='shutter', name='shutters', hint='shutter_hint'),
        mirror(12, 6, '/', name='m1', hint='mirror_hint'),
        mirror(12, 2, '/', name='m2', hint='mirror_hint'),
        mirror(12, 10, '\\', name='m3', hint='mirror_hint'),
        receiver(4, 2, 'HLATCH', name='hatch_latch', hint='hatch_latch_hint'),
        gate(3, 12, inp=['HLATCH'], flag_='HATCH_OPEN', look='hatch', name='hatch', hint='hatch_hint'),
        lock(9, 10, 'OPTLENS', used=0, flag_='CRADLE_SET', look='cradle', name='cradle', hint='cradle_hint',
             text='cradle_need', actions=[('talk', 'cradle_set')]),
        lens(7, 10, look='lens', name='glass', hint='glass_hint'),
        receiver(5, 10, 'LAMP', focused=1, name='burner', hint='burner_hint'),
        find(2, 4, 'diagram', 'Mirror Angle Table', 'scroll'),
    ],
    triggers=[trigger('watch', once='LIGHT_RESTORED', cond=('signal', 'LAMP'),
                      actions=[('restore', 4), ('ctr', 'QUEST3', 8), ('collect', 'WONDER3'),
                               ('event', 'RESTORATION')])])

# ---- the keepers' sealed chamber -------------------------------------------------------------------------
say('lens_found', "Wrapped in oilcloth in a stone box: a lens, heavy as a brick and clear as water.",
     next='lens_found2', actions=[('ctr', 'QUEST3', 7)])
say('lens_found2', "The builders' mark is on the rim. The same mark as on Sunboy's drawings. This is one "
     "of the seven.")
talk('chamber_tablet', 'SIGN', "CARVED IN THE WALL: SEVEN WORKS FOR ONE VALLEY. LIGHT FOR THE SAILOR, WATER FOR THE "
     "FARMER, A ROAD FOR EVERY FOOT.")

room('sealed_chamber', "The Keepers' Chamber", [
    "XXXXXXXXXSXXXXXXXXXX",
    "X::::::::::::::::::X",
    "X::::::::::::::::::X",
    "X::J::::::::::::J::X",
    "X::J::::::::::::J::X",
    "X::::::::::::::::::X",
    "X::::::::::::::::::X",
    "X::::::::::::::::::X",
    "X::::::::::::::::::X",
    "X::E::::::::::::E::X",
    "X::::::::::::::::::X",
    "X::::::::::::::::::X",
    "X::::::::::::::::::X",
    "XXXXXXXXXXXXXXXXXXXX"],
    region=4, music='secret', lit=1,
    exits=[exit_(9, 0, 1, 1, 'lamp_room', 3, 11, 'up')],
    objects=[
        chest(9, 7, 'OPTLENS', 1, flag_='OPTLENS_TAKEN', actions=[('talk', 'lens_found')]),
        sign(9, 3, 'chamber_tablet', look='tablet'),
        find(2, 11, 'history', 'Brass Sextant', 'compass'),
        find(17, 11, 'diagram', 'Lens Grinding Notes', 'scroll'),
        find(17, 1, 'history', "Builders' Seal", 'badge'),
    ])

# ---- the tide pools --------------------------------------------------------------------------------------
talk('tide_high', 'SIGN', "The sea covers the way to the cave mouth. Only at low water does the sandbar show.")
say('crab_hint', "Shore crabs. They pinch when you step too close. A crack of my whip sends them sideways.",
    "Shore crabs. They pinch when you step too close. I'll go round them.")
talk('pools_sign', 'SIGN', "TIDE POOLS. LOOK, DON'T TAKE. (THE CRABS HAVE ASKED.)")

room('tidepools', 'The Tide Pools', [
    "####################",
    "OOOOOOOOOOOsss.s.ss#",
    "OOOOOOOOOOss~~sssss#",
    "OOOOOOOOOOss~~ssr.s#",
    "OOOOOOOOOOOssssssss#",
    "XXOOOOOOOOOOsss~~ss#",
    "XXOOOOOOOOOssss~~ss=",
    "esOOOOOOOOssssssss.=",
    "XXOOOOOOOOOssssssss#",
    "XXOOOOOOOOOOss~~sss#",
    "OOOOOOOOOOOsss~~s.s#",
    "OOOOOOOOOOOOssssss.#",
    "OOOOOOOOOOOOOsssss.#",
    "####################"],
    region=4, music='coast', restored_music='coast',
    exits=[exit_(19, 6, 1, 2, 'harbour', 1, 7, 'right'),
           exit_(0, 7, 1, 1, 'sea_cave', 18, 7, 'left', cond=('flag', 'LOW_TIDE'), locked='tide_high')],
    objects=[
        bridge(2, 7, w=4, flag_='LOW_TIDE', look='sandbar', name='bar_a'),
        bridge(6, 7, w=4, flag_='LOW_TIDE', look='sandbar', name='bar_b'),
        sign(17, 1, 'pools_sign', look='signpost'),
        creature(14, 4, 'wander', 'CRAB1', hits=2, damage=1, speed=8, look='crab', hint='crab_hint'),
        creature(16, 10, 'wander', 'CRAB2', hits=2, damage=1, speed=8, look='crab', hint='crab_hint'),
        find(18, 2, 'flower', 'Sea Lavender', 'flower_violet'),
        find(14, 12, 'flower', 'Beach Rose', 'flower_red'),
        find(13, 1, 'specimen', 'Spiral Shell', 'shell'),
        find(11, 10, 'flower', 'Yellow Horned Poppy', 'flower_orange'),
    ])

# ---- the low-tide supply cave ----------------------------------------------------------------------------
say('esc_found', "Marra's spare escapement wheel, oiled and wrapped. The keepers planned for everything.",
     actions=[('ctr', 'QUEST3', 5)])
talk('cave_supplies', 'SIGN', "LIGHTHOUSE STORES: OIL, WICKS, CANVAS, ROPE. TAKE ONLY WHAT THE LIGHT NEEDS.")

room('sea_cave', 'The Low-Tide Cave', [
    "XXXXXXXXXXXXXXXXXXXX",
    "XXggggXXXXXggggggXXX",
    "Xggg~~ggggggg~~gggXX",
    "Xgg~~~ggggggg~~ggggX",
    "XggggggggXXggggggggX",
    "XgggggggXXXXgggg~~gX",
    "Xg~~gggggXXggggg~~gX",
    "Xg~~gggggggggggggggg",
    "XggggggggggggggggggX",
    "XgggXXgggggg~~~ggggX",
    "XgggXXgggggg~~~ggggX",
    "XXggggggggggggggggXX",
    "XXXggggXXXXXXggggXXX",
    "XXXXXXXXXXXXXXXXXXXX"],
    region=4, music='cave', restored_music='cave', lit=0,
    exits=[exit_(19, 7, 1, 1, 'tidepools', 1, 7, 'right')],
    objects=[
        chest(4, 4, 'ESCAPEMENT', 1, flag_='ESC_TAKEN', look='crate', actions=[('talk', 'esc_found')]),
        sign(7, 8, 'cave_supplies', look='note'),
        creature(14, 8, 'wander', 'CRAB3', hits=2, damage=1, speed=8, look='crab', hint='crab_hint'),
        find(2, 2, 'specimen', 'Red Coral Branch', 'crystal'),
        find(17, 3, 'flower', 'Sea Aster', 'flower_violet'),
        find(15, 11, 'photo', 'Light in the Sea Cave', 'photo'),
        find(12, 1, 'history', 'Old Coin of the Coast', 'coin'),
    ])
