"""
Chapter I: The Forgotten Watermill (Valley of Sacred Falls).

Willowbrook's fields are dry: the old watermill above the village stopped
when Smoggar's crews stripped it, and its gear train also lifted the great
irrigation sluice. Master Tovan knows the machine; its bronze cog was dumped
in the Mossy Cave past the forest path. One sister sets out (the other stays
to help the village mend its storm damage), brings it back, sets the water
running in the right order and the valley's first wonder turns again.

Puzzles, Maiya's way: the forest gate (a stone pushed onto its plate), the
miners' chimes (one chime up a vine wall), the cave chasm (a block into the
pit), the cog pushed home in the gear train, the counterweight gate (a stone
on its plate). Luna's way: the gate's old lifting catch, up in the narrow
pocket; the cleft behind the chimes; the miners' folded footbridge over the
chasm; the cog fitted by a fine hand; a gap in the mill wall past the
counterweight gate. Both: the tune, the sluices (open the high one, shut the
spill, or the safety floodgate stays down), a hidden chamber in the cave and
five collectibles.
"""
from worldlib import (room, exit_, npc, sign, pickup, chest, rest, secret, block, plate, gate, gear,
                      water_src, valve, outlet, bell, sequence, creature, trigger, talk, item, collectible,
                      speaker, chapter, flag, counter, signal_flag, lever, bridge, maiya, luna, J, say)

# ---- cast -------------------------------------------------------------------------
speaker('MAIYA', 'Maiya')
speaker('LUNA', 'Luna')
speaker('SUNBOY', 'Sunboy')
speaker('NARI', 'Elder Nari')
speaker('TOVAN', 'Master Tovan')
speaker('MINA', 'Mina')
speaker('PIKO', 'Piko')
speaker('ORIN', 'Captain Orin')
speaker('VALEN', 'Director Valen')
speaker('HANA', 'Hana')
speaker('SIGN', '')

chapter(1, 'The Forgotten Watermill', 'Valley of Sacred Falls', 'Waterwheel Mechanism',
        start=('village', 2, 4, 'down'), quest='QUEST', wonder='WHEELMECH',
        restore_title='The watermill turns again',
        restore_text='Water runs down the old\nchannels to Willowbrook.\nThe fields will drink.',
        back=('village', 9, 6, 'down'),
        end_text='Six wonders still sleep\nacross the valley. The\nEmerald Labyrinth is next.',
        goals=[(0, "Talk to Elder Nari by the well in Willowbrook."),
               (1, "Go up the north path and ask Master Tovan what the watermill needs."),
               (2, "Find the bronze cog in the Mossy Cave, past the forest path east of the village."),
               (3, J("Bring the cog to the mill and push it home in the gear train, between the axle and the "
                     "millstone.",
                     "Bring the cog to the mill. Your steady hands can fit it in the gear train.")),
               (4, J("Set the water running: open the high sluice, shut the spill. A heavy stone can hold the "
                     "counterweight plate.",
                     "Set the water running: open the high sluice, shut the spill. Look for a way past the "
                     "counterweight gate.")),
               (5, "The watermill turns again. Sunboy found something behind the wheel.")])

# ---- items (the order is the save's item numbers: add new ones at the end) ----------
item('NOTEBOOK', 'record', 'Field Notebook', J("Luna's notebook, lent for the journey: notes, maps, clues.",
                                                 'Luna keeps notes, maps and clues here.'))
item('LENS', 'tool', 'Magnifying Lens', 'Shows cracks, drafts and small marks. Use it on a wall.')
item('APPLE', 'consume', 'Apple', 'Crisp and sweet. Restores two hearts.', stack=9, param=2)
item('BANDAGE', 'consume', 'Bandage', 'Clean linen for scrapes. Restores four hearts.', stack=3, param=4)
item('COG', 'key', 'Bronze Cog', 'The missing cog of the watermill gear train.')
item('DIAGRAM', 'record', 'Sluice Diagram', 'Open the high sluice. Shut the spill. The floodgate rises only on a dry yard.')
item('WHEELMECH', 'key', 'Waterwheel Mechanism', 'The first of the seven wonder components.')

# ---- collectibles -----------------------------------------------------------------
collectible('SUNLILY', 'flower', 'Sun Lily')
collectible('MEADOWBELL', 'flower', 'Meadow Bell')
collectible('BADGE', 'history', "Caretaker's Badge")
collectible('DIAGRAM1', 'diagram', 'Sluice Diagram')
collectible('FALLSVIEW', 'photo', 'View of Sacred Falls')
collectible('LANTERNMOSS', 'specimen', 'Lantern Moss')
collectible('WONDER1', 'wonder', 'Waterwheel Mechanism')

counter('QUEST')    # 0 start, 1 Nari's errand, 2 Tovan's request, 3 cog found, 4 cog set, 5 restored
counter('CHAPTER')  # the chapter being played, from 0

# ---- Willowbrook ---------------------------------------------------------------------
talk('intro', J('LUNA', 'MAIYA'), J("Maiya, wake up. The bean field by the channel is wilting again.",
                                    "Luna! Wake up! The bean field by the channel is wilting again!"),
     next='intro2', actions=[('give', 'NOTEBOOK'), ('give', 'LENS'), ('give', 'APPLE', 2), ('checkpoint',)])
talk('intro2', J('MAIYA', 'LUNA'), J("Then let's fix it! I'll dig a new channel. A big one. Today.",
                                     "Then something upstream is wrong. Water doesn't just stop."), next='intro3')
talk('intro3', J('LUNA', 'MAIYA'),
     J("Or ask why the old one is dry first. Elder Nari was looking for you by the well. I promised Hana I'd help "
       "mend the storm fences today, so take my notebook and the lens, and write everything down!",
       "I promised Hana I'd haul stones for her storm fence all day. You go and see Elder Nari by the well, she "
       "was looking for us. Take the lens. You're better at puzzles anyway!"),
     next='intro4')
talk('intro4', 'SIGN', J("Move with the stick. A talks, takes and pushes home. B cracks Maiya's whip: it shoos "
                         "creatures. C uses the tool you carry. START opens the notebook. Maiya climbs ivy and "
                         "pushes heavy stones.",
                         "Move with the stick. A talks, takes and fits. B: Luna's close look says what she "
                         "faces. C uses the tool you carry. START opens the notebook. Luna slips through narrow "
                         "gaps and works fine mechanisms."))

talk('nari', 'NARI', J("The valley is mending, Maiya. And still the channels stay dry.",
                          "The valley is mending, Luna. And still the channels stay dry."),
     cond=('ctr_ge', 'QUEST', 5), alt='nari_mid', next='nari_done')
talk('nari_done', 'NARI', "Listen to that water! The old builders knew what they were doing. So do you.")
talk('nari_mid', 'NARI', "Master Tovan knows that mill better than anyone. Help him, and mind the cave.",
     cond=('ctr_ge', 'QUEST', 1), alt='nari_first')
talk('nari_first', 'NARI', "There you are. The irrigation channels went dry when the old watermill stopped. "
     "Smoggar's crews took what they could carry from it.",
     next='nari_first2')
talk('nari_first2', 'NARI', "Master Tovan has been sitting by that wheel for days. Go up the north path and ask "
     "him what it needs. The well water is clean if you are tired.",
     next='nari_first3', actions=[('ctr', 'QUEST', 1)])
say('nari_first3', "Mill, north, Tovan. Got it! I'll race myself there.",
    "Mill, north, Master Tovan. I'll take notes on the way.")

talk('sunboy', 'SUNBOY', "I'm drawing the wheel's gears! Well, the ones that are still there.",
     cond=('ctr_ge', 'QUEST', 5), alt='sunboy_mid', next='sunboy_done')
talk('sunboy_done', 'SUNBOY', "Behind the wheel there's a stone passage with old drawings. Seven buildings! "
     "Come and see when you're ready.")
talk('sunboy_mid', 'SUNBOY', "A cog, a race, a sluice... Tovan uses words like a toolbox. I'm writing them all down.",
     cond=('ctr_ge', 'QUEST', 2), alt='sunboy_first')
talk('sunboy_first', 'SUNBOY', "Hi! Did you know a waterwheel turns a millstone AND can lift a gate? "
     "The river does the heavy work. That's the clever part.")

talk('hana', 'HANA', "Water in the channel! My beans are standing up straight again. Thank you!",
     cond=('flag', 'MILL_RESTORED'), alt='hana_dry')
talk('hana_dry', 'HANA', "Every morning I carry buckets from the well. The beans drink faster than I can walk.")

talk('village_sign', 'SIGN', "WILLOWBROOK. VALLEY OF SACRED FALLS. NORTH: THE OLD WATERMILL. EAST: THE EMERALD "
     "FOREST.")
say('home_door', "Home. Breakfast later. Adventure first!", "Home. Maiya is out at Hana's fence already.")
say('nari_door', "Elder Nari's house. It smells of old paper and cinnamon.")
talk('well', 'SIGN', "You drink the cool well water and feel rested. (Hearts restored. The valley will remember this "
     "spot as your rest point.)")

talk('valen', 'VALEN', "Ah, the young lady who fixed the mill. I am Valen. I once kept Smoggar's works running.",
     next='valen2')
talk('valen2', 'VALEN', "Imagine all seven of these old machines under one central office: clean water on a "
     "schedule, jobs, order. The valley could be modern at last.", next='valen3')
talk('valen3', J('MAIYA', 'LUNA'), J("Decided by whom, mister? The old channels reach every village the same!",
                                     "A schedule decided by whom, Director? The old channels reach every village "
                                     "the same."),
     next='valen4')
talk('valen4', 'VALEN', "Efficiency decides, young lady. We will talk again. I hear there are six more wonders.")

room('village', 'Willowbrook', [
    "TTTTTTTTT==TTcTTTTTT",
    "TRRRR....==..c.RRRRT",
    "TRRRR.,..==..c.RRRRT",
    "THDHH....==..c.HHDHT",
    "T.=......==..c...=.T",
    "T.===========k====.T",
    "=..,.....==..c......",
    "=..o.....====k======",
    "T.,......==..c....,T",
    "TxxxxxxxF==Fxcxxxx.T",
    "TFFFFFFFF==FFcFFFF.T",
    "TcccccccckkcccFFFF.T",
    "TFFFFFFFF==FFFFFF,.T",
    "TTTTTTTTTTTTTTTTTTTT"],
    region=1, music='village', restored='village_restored', restored_music='restored',
    exits=[exit_(9, 0, 2, 1, 'mill', 9, 12, 'up'),
           exit_(19, 6, 1, 2, 'forest', 1, 6, 'right'),
           exit_(0, 6, 1, 2, 'crossroads', 18, 6, 'left', cond=('ctr_ge', 'CHAPTER', 2), locked='west_locked')],
    objects=[
        npc(6, 7, 'NARI', 'nari', name='nari'),
        npc(11, 3, 'SUNBOY', 'sunboy', name='sunboy', wander=1, speed=6),
        npc(15, 10, 'HANA', 'hana', name='hana'),
        npc(12, 8, 'VALEN', 'valen', name='valen', only=('set', 'MILL_RESTORED')),
        rest(3, 7, text='well', look='well'),
        sign(8, 4, 'village_sign', look='signpost'),
        sign(2, 3, 'home_door', look='none'),
        sign(17, 3, 'nari_door', look='none'),
        pickup(18, 12, 'APPLE', 1, flag_='APPLE_VILLAGE'),
        pickup(1, 8, collect='MEADOWBELL', flag_='MEADOWBELL_TAKEN', look='flower_blue'),
    ],
    triggers=[trigger('room', once='INTRO_DONE', actions=[('talk', 'intro')])])

# ---- the watermill yard --------------------------------------------------------------
talk('tovan', 'TOVAN', "Hear that? Water singing in the race. The old girl is turning again.",
     cond=('ctr_ge', 'QUEST', 5), alt='tovan_4', next='tovan_done')
talk('tovan_done', 'TOVAN', "You opened the high sluice and shut the spill. That's real engineering, not luck. "
     "Your brother is poking about behind the wheel, by the way.")
talk('tovan_4', 'TOVAN', "The cog sits sweet. Now the water: the race needs the high sluice open and the spill "
     "shut, or the safety floodgate stays down. The sluice platform is behind the counterweight gate.",
     cond=('ctr_ge', 'QUEST', 4), alt='tovan_3')
talk('tovan_3', 'TOVAN', J("You found it! Bring it to the empty housing between the axle and the millstone. Line "
                           "it up and give it one firm push: it's a snug old fit.",
                           "You found it! Bring it to the empty housing between the axle and the millstone. "
                           "Gently. It wants a fine hand."), cond=('item', 'COG'), alt='tovan_2')
talk('tovan_2', 'TOVAN', "The Mossy Cave is past the forest path, east of the village. Smoggar's crews dumped "
     "the parts they could not sell there.", cond=('ctr_ge', 'QUEST', 2), alt='tovan_1')
talk('tovan_1', 'TOVAN', "Nari sent you? Good. Look at this wheel: sound timbers, true axle. But the gear train "
     "has a gap where the bronze cog was.", next='tovan_1b')
talk('tovan_1b', 'TOVAN', "No cog, no millstone. No millstone, no lift on the irrigation sluice. And no water "
     "for Willowbrook. The cog was carried off to the Mossy Cave, past the forest path.",
     next='tovan_1c', actions=[('ctr', 'QUEST', 2)])
say('tovan_1c', "A cave! Finally, something to do. I'll bring it back, Master Tovan. In one piece. Probably.",
    "I'll bring it back, Master Tovan. In one piece. And I'll note where every tooth goes.")

say('gear_socket', "An empty housing between the axle gear and the millstone gear. The cog would mesh both.")
talk('race_sign', 'SIGN', "THE MILL RACE. HIGH SLUICE ABOVE THE FALLS. SPILL SLUICE DRAINS TO THE YARD. KEEP THE "
     "YARD DRY OR THE FLOODGATE WILL NOT RISE.")
say('spill_hint', "This valve lets the race spill onto the yard. While water runs here, the floodgate by the "
    "wheel stays shut.")
say('high_hint', "The high sluice. It holds the whole race back from the wheel.")
say('flood_hint', "A safety floodgate. A float under it drops it whenever the yard is wet.")
say('plate_hint', "A counterweight plate. Whatever stands on it holds that gate up.",
    "A counterweight plate. Whatever stands on it holds that gate up. Not me, though: I need to be on the "
    "other side.")
say('plate_teach', "Feel that? The gate lifts while I stand here, and drops when I step off. Something heavy "
    "could stand here instead... like that loose millstone block!",
    "The gate lifts while I stand here, and drops when I step off. But look: there's a crack in the wall by the "
    "falls, just my size. I can slip round!")
say('millstone_hint', "An old millstone block. Heavy enough to hold that plate down, if I push it there.")
say('wall_gap_hint', "A crack between the stones. Too narrow for Maiya, never mind a big millstone.",
    "A crack between the wall stones. I can just squeeze through to the sluice platform.")
talk('wheel_secret', 'SUNBOY', "Look! Behind the wheel, a stone passage. Drawings of seven great machines... "
     "and this one is the first!", next='wheel_secret2')
say('wheel_secret2', "Seven wonders, built to share water fairly between every village? And six of them are "
    "still asleep!",
    "Seven wonders, built to share water fairly between every village. And six of them are still asleep.",
    next='wheel_secret3')
say('wheel_secret3', "Then I wake them up. One at a time. Carefully. ...Did I say that? Luna will never believe "
    "I said carefully!",
    "Then I'll wake them up, one at a time. Maiya will want to hear every part of it.",
    actions=[('event', 'CHAPTER_END')])

room('mill', 'The Forgotten Watermill', [
    "###########ww#######",
    "#PPPWW....~~~~#....#",
    "#PPPWWuuuuuuu:#....#",
    "#MdMWW....u:::n....#",
    "#.=.:::...u#:L#....#",
    "#.=...c...u........#",
    "#.=...k............#",
    "#.=...ccckkccc.....#",
    "#.=========..c.....#",
    "#........==..c.....#",
    "#........==..k.....#",
    "#........==..c.....#",
    "#........==..c.....#",
    "#########==##c######"],
    region=1, music='mill', restored='mill_restored', restored_music='restored',
    exits=[exit_(9, 13, 2, 1, 'village', 9, 1, 'down')],
    objects=[
        npc(8, 9, 'TOVAN', 'tovan', name='tovan'),
        npc(7, 4, 'SUNBOY', 'wheel_secret', name='sunboy_mill', only=('set', 'MILL_RESTORED')),
        sign(14, 5, 'race_sign', look='signpost'),
        water_src(12, 2, name='falls'),
        valve(11, 2, open_=0, name='high_sluice', look='valve', hint='high_hint'),
        valve(10, 4, open_=1, name='spill_sluice', look='valve', hint='spill_hint'),
        outlet(10, 5, 'SPILL', name='spill', look='none'),
        gate(8, 2, inp=['SPILL'], invert=1, look='floodgate', name='floodgate', hint='flood_hint'),
        outlet(6, 2, 'WHEEL', name='wheel_race', look='none'),
        gear(4, 4, 'axle', role=1, mesh=['socket'], inp=['WHEEL'], look='gear_big'),
        gear(5, 4, 'socket', role=0, mesh=['axle', 'stone'], item_='COG', fine=J(0, 1), look='gear',
             hint='gear_socket', flag_='COG_SET'),
        gear(6, 4, 'stone', role=2, mesh=['socket'], out=['MILL'], look='gear_big'),
        water_src(6, 5, inp=['WHEEL'], name='tailrace'),
        gate(13, 8, inp=['MILL'], flag_='SLUICE_OPEN', look='sluicegate', name='irrigation_sluice'),
        outlet(13, 12, 'IRRIGATION', name='irrigation', look='none'),
        gate(12, 4, inp=['PLATE'], look='woodgate', name='counter_gate'),
        plate(15, 6, 'PLATE', weight=2, look='plate', name='counter_plate', hint='plate_hint'),
        *maiya(block(15, 8, heavy=1, look='block', name='mill_stone', hint='millstone_hint')),
        *luna(sign(14, 4, 'wall_gap_hint', look='none', name='wall_gap')),
        pickup(17, 2, collect='FALLSVIEW', flag_='FALLSVIEW_TAKEN', look='photo'),
        # once restored, the race stays full below the floodgate whatever the valves say, and a
        # divider keeps that water from the spill (or the floodgate would shut on it, and open, and shut)
        signal_flag('MILL_RESTORED', 'DONE', name='restored'),
        water_src(7, 2, inp=['DONE'], name='race_kept'),
        gate(9, 2, inp=['DONE'], invert=1, look='none', name='race_divider'),
    ],
    triggers=[trigger('watch', once='COG_SET_TALK', cond=('flag', 'COG_SET'), actions=[('ctr', 'QUEST', 4)]),
              # the first time a sister stands on the counterweight plate: how to keep it down
              trigger('enter', 15, 6, once='PLATE_TAUGHT', actions=[('talk', 'plate_teach')]),
              # the first time water reaches the village channel: once, whatever happens after
              trigger('watch', once='MILL_RESTORED', cond=('signal', 'IRRIGATION'),
                      actions=[('set', 'SLUICE_OPEN'), ('restore', 1), ('ctr', 'QUEST', 5),
                               ('collect', 'WONDER1'), ('give', 'WHEELMECH'), ('event', 'RESTORATION')])])

# ---- the forest path ------------------------------------------------------------------
talk('mina', 'MINA', "The forest is breathing again now the channels run. You can almost hear the roots drink!",
     cond=('flag', 'MILL_RESTORED'), alt='mina_gate')
talk('mina_gate', 'MINA', J("Oh! Hello. I'm Mina, I study plants. Someone shut the old forest gate with a "
                            "weight plate. A heavy stone on the plate would hold it open.",
                            "Oh! Hello. I'm Mina, I study plants. Someone shut the old forest gate and left its "
                            "lifting gear up in the rocks."), next='mina_gate2', cond=('not', 'FOREST_GATE'),
     alt='mina_lily')
talk('mina_gate2', 'MINA', J("I've seen Sun Lilies glowing gold past the gate, up in the clearing to the "
                             "north-east. They follow the sun all day!",
                             "There's a gap in the rocks up there, too narrow for most people. The gate's catch "
                             "is beyond it, and Sun Lilies glowing gold. They follow the sun all day!"))
talk('mina_lily', 'MINA', "A Sun Lily turns its face to follow the sun across the sky. Plants are patient "
     "engineers.")
talk('forest_sign', 'SIGN', "EMERALD FOREST PATH. EAST: THE MOSSY CAVE. WEST: WILLOWBROOK.")
say('block_hint', "A dressed stone. Heavy, but I can shift it!")
say('fplate_hint', "A pressure plate wired to the gate. It needs more weight than me: a stone, say.",
    "A pressure plate wired to the gate. It needs far more weight than me.")
say('narrow_hint', "A gap between the rocks. My shoulders won't fit, that's for sure.",
    "A gap between the rocks. I can squeeze through.")
say('fcatch_hint', "The old gate's lifting catch.", "The old gate's lifting catch. A little spring holds it: "
    "careful fingers could free it.")

room('forest', 'Emerald Forest Path', [
    "TTTTTTTTTTTTTTTTTTTT",
    "TT.,.,.TTTTTTTTTTTTT",
    "TT,.,..TTTTTTTTTTTTT",
    "TT..,.,TTTTTT..TTTTT",
    "TTTTnTTT....TT..TTTT",
    "T.......b...T......T",
    "====================",
    "====================",
    "T....b......T......T",
    "T...r.......T..h...T",
    "T........,..T.hh...T",
    "T..b.....,..T.....,T",
    "T.....,.....T......T",
    "TTTTTTTTTTTTTTT==TTT"],
    region=2, music='forest', restored_music='forest',
    exits=[exit_(0, 6, 1, 2, 'village', 18, 6, 'left'),
           exit_(19, 6, 1, 2, 'cave_mouth', 1, 7, 'right'),
           exit_(15, 13, 2, 1, 'trailhead', 15, 1, 'down', cond=('ctr_ge', 'CHAPTER', 1), locked='south_locked')],
    objects=[
        npc(5, 5, 'MINA', 'mina', name='mina', only=('clear', 'CH2_BEGUN')),
        sign(2, 8, 'forest_sign', look='signpost'),
        gate(12, 6, h=2, flag_='FOREST_GATE', inp=['FPLATE'], look='woodgate', name='forest_gate'),
        plate(9, 10, 'FPLATE', weight=3, sticky=1, name='forest_plate', actions=[('set', 'FOREST_GATE')],
              hint='fplate_hint'),
        *maiya(block(7, 9, heavy=1, name='forest_stone', hint='block_hint'),
               pickup(13, 3, collect='SUNLILY', flag_='SUNLILY_TAKEN', look='flower_gold'),
               chest(18, 5, 'BANDAGE', 1, flag_='FOREST_CHEST')),
        *luna(lever(5, 1, 'FCATCH', fine=1, look='lever', name='forest_catch', hint='fcatch_hint',
                    flag_='FOREST_CATCH', actions=[('set', 'FOREST_GATE')]),
              pickup(3, 1, collect='SUNLILY', flag_='SUNLILY_TAKEN', look='flower_gold'),
              chest(6, 2, 'BANDAGE', 1, flag_='FOREST_CHEST')),
        pickup(17, 11, 'APPLE', 1, flag_='APPLE_FOREST'),
    ])

# ---- the cave mouth -------------------------------------------------------------------
talk('tablet', 'SIGN', "A miners' tablet. Carved notes run down it like water down a hill: LOW... HIGH... "
     "MIDDLE. Under them: RING US IN ORDER AND THE DOOR WILL LISTEN.")
say('door_hint', "A counterweighted stone door. Its latch is a little pendulum tuned to those chimes.")
say('chime_low', "The long pipe: the lowest note.")
say('chime_mid', "The middle pipe.")
say('chime_high', "The short pipe up on the ledge: the highest note.")
say('cleft_hint', "A cleft in the rock. Not for my shoulders.", "A cleft in the rock, up to the ledge. Just wide "
    "enough for me.")
say('beetle_hint', "Rust beetles. They nest in old machine oil and nip when frightened. A crack of my whip "
    "shoos them off.", "Rust beetles. They nest in old machine oil and nip when frightened. Best to walk round "
    "them.")
say('door_open', "It opened! I knew it would. Well, I hoped it would.", "It opened. Low, high, middle: just as "
    "the tablet said.")

room('cave_mouth', 'The Mossy Cave', [
    "####################",
    "##....#########ee###",
    "##....n########::###",
    "###v##n########::###",
    "#..v..........:::.##",
    "#..............::..#",
    "#..................#",
    "=..................#",
    "=..........,.......#",
    "#...,........r.....#",
    "#..................#",
    "#..........,.......#",
    "#....r.........,...#",
    "####################"],
    region=2, music='cave', restored_music='cave',
    exits=[exit_(0, 7, 1, 2, 'forest', 18, 6, 'left'),
           exit_(15, 1, 2, 1, 'cave_inner', 9, 12, 'up')],
    objects=[
        sign(13, 4, 'tablet', look='tablet'),
        bell(8, 6, 0, name='low', look='chime', hint='chime_low'),
        bell(12, 9, 1, name='mid', look='chime', hint='chime_mid'),
        bell(4, 1, 2, name='high', look='chime', hint='chime_high'),
        sequence('tune', ['low', 'high', 'mid'], 'DOOR', flag_='CAVE_DOOR', actions=[('talk', 'door_open')]),
        gate(15, 2, w=2, inp=['DOOR'], flag_='CAVE_DOOR', look='stonedoor', name='stone_door', hint='door_hint'),
        creature(10, 8, 'wander', 'BEETLE1', hits=2, damage=1, speed=10, look='beetle', hint='beetle_hint'),
        creature(6, 11, 'wander', 'BEETLE2', hits=2, damage=1, speed=12, look='beetle', hint='beetle_hint'),
    ])

# ---- inside the cave -------------------------------------------------------------------
say('cog_found', "The cog! Heavier than it looks. Good thing I'm strong.", "The cog! Heavy, but every tooth is "
    "whole. Master Tovan will be glad.", actions=[('ctr', 'QUEST', 3)])
talk('miner_note', 'SIGN', "A miner's note, pinned under a stone: THE SLUICE DIAGRAM IS ON THE TABLE. DON'T "
     "LET THEM MELT THE COG.")
say('crack_hint', "Cold air through these cracks... there's a space behind. A closer look with the lens might "
    "show the loose stones.")
say('pit_hint', "A narrow chasm. If that block went in, it would fill it like a cork.",
    "A narrow chasm. The miners left a footbridge folded up on the far side, and its catch is right here.")
say('fbridge_catch', "A footbridge catch.", "The miners' footbridge catch. A tiny pin, and down it comes.")
say('chamber', "A caretaker's alcove. Someone looked after these machines, long ago.")

room('cave_inner', 'Inside the Mossy Cave', [
    "XXXXXXXXXXXXXXXXXXXX",
    "XggXXggggggggggggggX",
    "XggXXggggggggggggggX",
    "XgggggggggggggggggXX",
    "XXXXXggggggggggggXXX",
    "XXXXXXXggggggXXXXXXX",
    "XXXXXXXXXggXXXXXXXXX",
    "XXXXXXXXXXpXXXXXXXXX",
    "XXXXXXXXXggXXXXXXXXX",
    "XXXXXgggggggggXXXXXX",
    "XXXXgggggggggggXXXXX",
    "XXXgggggggggggggXXXX",
    "XXXggggggggggggggXXX",
    "XXXXXXXXXggXXXXXXXXX"],
    region=2, music='cave', restored_music='cave', lit=0,
    exits=[exit_(9, 13, 2, 1, 'cave_mouth', 15, 3, 'down')],
    objects=[
        *maiya(block(10, 9, heavy=1, name='cave_block', hint='pit_hint')),
        *luna(bridge(10, 7, inp=['FOOTBRIDGE'], flag_='CAVE_BRIDGE', look='planks', name='footbridge', hint='pit_hint'),
              lever(9, 8, 'FOOTBRIDGE', fine=1, look='lever', name='bridge_catch', hint='fbridge_catch',
                    flag_='CAVE_CATCH', actions=[('set', 'CAVE_BRIDGE')])),
        chest(16, 2, 'COG', 1, flag_='COG_CHEST', actions=[('talk', 'cog_found')]),
        pickup(8, 1, 'DIAGRAM', 1, collect='DIAGRAM1', flag_='DIAGRAM_TAKEN', look='scroll'),
        sign(12, 1, 'miner_note', look='note'),
        secret(3, 3, 'CHAMBER_FOUND', tool='LENS', solid=1, look='crack', hint='crack_hint',
               actions=[('talk', 'chamber')]),
        pickup(1, 1, collect='BADGE', flag_='BADGE_TAKEN', look='badge'),
        pickup(16, 12, collect='LANTERNMOSS', flag_='MOSS_TAKEN', look='moss'),
        pickup(4, 11, 'APPLE', 1, flag_='APPLE_CAVE'),
    ])
