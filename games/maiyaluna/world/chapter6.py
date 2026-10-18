"""
Chapter VI: The World Tree Observatory (Sacred World Tree).

North of the forest research station rises the World Tree, older than any
village. Among its upper branches the old builders kept an observatory, for
the stars are the oldest map there is: from up there Sefa the astronomer
hopes to find where all the valley's water gathers underground.

The cable car to the canopy waits behind its brake. In the canopy, hanging
bridges swing on levers between platforms (one lever sends a bridge to the
west platform or the east, never both). On the sun terrace the stair opens
only when sunlight has fallen on each of three marked stones, as the sun
moves from dawn to dusk: two bronze mirrors carry it. In the calendar hall
four dials must be set in the tree-keepers' year, which began when the
leaves fall; they hold the dome gear. With the dome open and the precision
lens set in the telescope, Sefa charts the reservoir: the Master Navigation
Chart. Hidden: the heart of the tree, through a gap in the bark.
"""
from worldlib import (J, room, exit_, add_exit, npc, sign, pickup, chest, rest, gate, gear, creature, trigger, talk, item,
                      chapter, counter, signal_flag, lock, bridge, platform, lever, emitter, mirror, receiver, bell,
                      sequence, find, collectible, speaker, canvas, put, text)

speaker('SEFA', 'Sefa')

item('DOMEGEAR', 'key', 'Dome Gear', "The observatory dome's drive gear, kept safe by the calendar.")
item('NAVCHART', 'key|record', 'Master Navigation Chart', 'The sixth wonder component: the valley charted from '
     'the stars.')
collectible('WONDER6', 'wonder', 'Master Navigation Chart')

counter('QUEST6')   # 0 arrived, 1 Sunboy's note, 2 the canopy, 3 the sun terrace, 4 the stair, 5 the dome gear,
                    # 6 the dome open, 7 the telescope set, 8 the chart

chapter(6, 'The World Tree Observatory', 'Sacred World Tree', 'Master Navigation Chart',
        start=('tree_roots', 9, 11, 'up'), quest='QUEST6', wonder='NAVCHART', together=True,
        restore_title='The reservoir is found',
        restore_text="Far below the seven regions\nlies the valley's great\nreservoir. The way is charted.",
        back=('observatory', 9, 10, 'down'),
        end_text='Six wonders are found.\nThe last waits under the\nvalley, in the dark.',
        goals=[(0, "The World Tree at last. Sunboy is waiting by the roots."),
               (1, "Release the cable car's brake and ride up to the canopy."),
               (2, "Cross the hanging bridges to the sun terrace, north. Their levers swing them."),
               (3, "Mark the sun on the three stones of the terrace, one after another."),
               (4, "The observatory's dome gear is kept by the calendar hall, east of the bridges."),
               (5, "Fit the dome gear in the observatory's drive."),
               (6, "Set the precision lens in the telescope."),
               (7, "Talk to Sefa: the telescope is ready."),
               (8, "The reservoir is charted. Sefa has more to say.")])

# ---- the roots of the World Tree -----------------------------------------------------------------------
# the sisters meet again: the one who stayed home catches up at the foot of the tree (she travels with the
# other for this chapter only; D switches between them)
talk('ch6_intro', J('LUNA', 'MAIYA'),
     J("Maiya! There you are! Hana's fence is standing, Grandma's roof is mended, and I ran the whole new road to "
       "find you.",
       "Luna! Finally! Hana's fence is so straight you could rule lines with it. I ran all the way!"),
     next='ch6_intro2')
talk('ch6_intro2', J('MAIYA', 'LUNA'),
     J("Luna! You came! Look up. No, further. Further! I can't even see the top!",
       "Maiya! I missed you. Look up: the World Tree. They say it was old when the first builders came."),
     next='ch6_intro3')
talk('ch6_intro3', J('LUNA', 'MAIYA'),
     J("The World Tree. They say it was old when the first builders came. Sefa says this wonder needs two pairs of "
       "hands. Good thing there are two of us again.",
       "Look up. No, further. Further! I can't even see the top! ...Right. Two of us, one giant tree. Let's go."),
     next='ch6_intro4')
talk('ch6_intro4', 'SIGN', J("Luna travels with you in this chapter. D switches sisters; hold D to make the other "
                             "one wait or follow. Switch on a plate and the one you leave stays on it.",
                             "Maiya travels with you in this chapter. D switches sisters; hold D to make the other "
                             "one wait or follow. Switch on a plate and the one you leave stays on it."))
talk('sunboy6', 'SUNBOY', "Sefa's waiting at the observatory, right at the top! He sent the cable car's key... "
     "well, he said the brake lever is by the car. Same thing.", actions=[('ctr', 'QUEST6', 1)])
talk('brake_hint', 'LUNA', "The cable car's brake. Release it and the counterweight hauls the car up and down.")
talk('car_hint', 'LUNA', "The cable car. It waits a moment at each end.")
talk('roots_sign', 'SIGN', "THE WORLD TREE. CABLE CAR TO THE CANOPY. OBSERVATORY AT THE TOP. PLEASE DO NOT CARVE "
     "YOUR NAME IN THE BARK; IT REMEMBERS.")
talk('tree_locked', 'SIGN', "A rope across the path: CLOSED FOR REPAIRS. Not yet.")

g = canvas('T', '.')
put(g, 1, 3, 18, 6, 'p')
put(g, 6, 1, 13, 2, 'f')
put(g, 9, 7, 10, 13, '=')
put(g, 1, 1, 4, 2, 'Q'); put(g, 15, 1, 18, 2, 'Q')
put(g, 1, 8, 3, 10, 'Q'); put(g, 16, 8, 18, 10, 'Q')
for x, y in ((5, 9), (14, 11), (6, 12)):
    g[y][x] = 'r'
for x, y in ((5, 11), (13, 8), (3, 12), (16, 12)):
    g[y][x] = ','
put(g, 9, 0, 10, 0, 'f')
room('tree_roots', 'Roots of the World Tree', text(g),
     region=7, music='worldtree', restored_music='worldtree',
     exits=[exit_(9, 13, 2, 1, 'station', 16, 1, 'down'),
            exit_(9, 0, 2, 1, 'canopy', 9, 12, 'up')],
     objects=[
         lever(12, 8, 'CABLE', look='lever', name='brake', hint='brake_hint', flag_='BRAKE_RELEASED'),
         platform(9, 6, 0, -3, speed=6, wait=50, inp=['CABLE'], look='basket', name='car', hint='car_hint'),
         npc(7, 9, 'SUNBOY', 'sunboy6', name='sunboy6'),
         sign(11, 11, 'roots_sign', look='signpost'),
         find(17, 11, 'flower', 'Tree Lily', 'flower_white'),
         find(2, 11, 'flower', 'Wood Sorrel', 'flower_white'),
         find(12, 1, 'specimen', 'Golden Acorn', 'acorn'),
     ],
     triggers=[trigger('room', once='CH6_BEGUN', actions=[('talk', 'ch6_intro')]),
               trigger('enter', 6, 1, 8, 2, once='CANOPY_REACHED', actions=[('ctr', 'QUEST6', 2)])])
add_exit('station', exit_(16, 0, 2, 1, 'tree_roots', 9, 12, 'up', cond=('ctr_ge', 'CHAPTER', 5), locked='tree_locked'))

# ---- the hanging bridges --------------------------------------------------------------------------------
talk('swing_hint', 'LUNA', "This lever swings the bridges: one to the west platform, one to the east. Never both "
     "at once.")
talk('north_lever_hint', 'LUNA', "The lever for the bridge to the north platform.")
talk('bridge_a_hint', 'LUNA', "The west bridge hangs from a pulley. The lever on the middle platform works it.")
talk('bridge_b_hint', 'LUNA', "The east bridge. It swings away when the west one comes down.")
talk('bridge_c_hint', 'LUNA', "The north bridge, folded up. Its lever must be on one of the platforms.")
talk('squirrel_hint', 'LUNA', "Red squirrels, guarding their acorns. A crack of the whip and they'll scamper off.")
talk('canopy_sign', 'SIGN', "HANGING BRIDGES. ONE LEVER, TWO BRIDGES: CHOOSE WHERE YOU GO.")

g = canvas('p', 'p')
put(g, 7, 10, 12, 12, 'f'); put(g, 9, 13, 10, 13, 'f')
put(g, 8, 5, 11, 7, 'f'); put(g, 9, 8, 9, 9, 'B')
put(g, 1, 4, 4, 8, 'f'); put(g, 15, 4, 18, 8, 'f')
put(g, 7, 1, 12, 2, 'f'); put(g, 9, 0, 10, 0, 'f')
g[6][0] = 'n'; g[6][19] = 'f'
g[9][2] = g[10][2] = 'v'; put(g, 1, 11, 3, 12, 'f')
room('canopy', 'The Hanging Bridges', text(g),
     region=7, music='worldtree', restored_music='worldtree',
     exits=[exit_(9, 13, 2, 1, 'tree_roots', 9, 1, 'down'),
            exit_(9, 0, 2, 1, 'sun_terrace', 9, 12, 'up'),
            exit_(19, 6, 1, 1, 'calendar_hall', 1, 6, 'right'),
            exit_(0, 6, 1, 1, 'tree_heart', 18, 7, 'left')],
     objects=[
         lever(10, 7, 'SWING', look='lever', name='swing', hint='swing_hint', flag_='SWUNG_WEST'),
         bridge(5, 6, w=3, inp=['SWING'], look='planks', name='bridge_a', hint='bridge_a_hint'),
         bridge(12, 6, w=3, inp=['SWING'], invert=1, look='planks', name='bridge_b', hint='bridge_b_hint'),
         lever(2, 5, 'NORTH', look='lever', name='north_lever', hint='north_lever_hint', flag_='NORTH_DOWN'),
         bridge(9, 3, h=2, inp=['NORTH'], look='planks', name='bridge_c', hint='bridge_c_hint'),
         sign(8, 11, 'canopy_sign', look='signpost'),
         creature(16, 5, 'wander', 'SQUIRREL1', hits=1, damage=1, speed=14, look='squirrel', hint='squirrel_hint'),
         find(17, 8, 'flower', 'Canopy Orchid', 'flower_violet'),
         find(2, 12, 'photo', 'A Woodpecker at Work', 'photo'),
         find(12, 1, 'photo', 'Above the Canopy', 'photo'),
         find(1, 4, 'flower', 'Air Plant Bloom', 'flower_red'),
     ],
     triggers=[trigger('enter', 7, 1, 6, 2, once='NORTH_REACHED', cond=('ctr_eq', 'QUEST6', 2),
                       actions=[('ctr', 'QUEST6', 3)])])

# ---- the heart of the tree (hidden) --------------------------------------------------------------------------
talk('heart_spring', 'SIGN', "Sap-sweet water seeps from the living wood into a little basin. (Hearts restored; "
     "the journey can be saved here.)")
talk('heart_rings', 'LUNA', "Growth rings, hundreds of them, wide in the wet years and thin in the dry ones. The "
     "tree kept a diary of the valley's weather.")

g = canvas('Q', 'f')
g[7][19] = 'f'
put(g, 5, 4, 6, 5, 'Q'); put(g, 13, 8, 14, 9, 'Q')
room('tree_heart', 'The Heart of the Tree', text(g),
     region=7, music='secret', restored_music='secret',
     exits=[exit_(19, 7, 1, 1, 'canopy', 1, 6, 'right')],
     objects=[
         rest(9, 6, text='heart_spring', look='none'),
         sign(4, 10, 'heart_rings', look='tablet'),
         find(2, 2, 'history', "Tree-Keeper's Carving", 'shard'),
         find(17, 2, 'specimen', 'Amber with a Leaf', 'crystal'),
         find(2, 11, 'diagram', 'Growth Ring Chart', 'scroll'),
         find(16, 11, 'flower', 'Ghost Orchid', 'flower_white'),
     ])

# ---- the sun terrace --------------------------------------------------------------------------------------------
talk('terrace_tablet', 'SIGN', "MARK THE SUN AT DAWN, AT NOON AND AT DUSK UPON THE THREE STONES, AND THE STAIR "
     "WILL KNOW THE DAY.")
talk('low_sun_hint', 'LUNA', "The sun, low through a gap in the leaves. Its light crosses the whole terrace.")
talk('bronze_hint', 'LUNA', "A bronze mirror on a pivot. A turns it, like the lighthouse mirrors.")
talk('stone_hint', 'LUNA', "A marked stone. When sunlight falls on it, it remembers: the builders cut a little "
     "catch that drops into place.")
talk('stair_hint', 'LUNA', "The stair to the observatory. Its gate wants all three stones marked.")
talk('stone_a_lit', 'LUNA', "Sunlight on the dawn stone: it is marked.", cond=('not', 'STONE_A'),
     actions=[('set', 'STONE_A')])
talk('stone_b_lit', 'LUNA', "Sunlight on the noon stone: it is marked.", cond=('not', 'STONE_B'),
     actions=[('set', 'STONE_B')])
talk('stone_c_lit', 'LUNA', "Sunlight on the dusk stone: it is marked.", cond=('not', 'STONE_C'),
     actions=[('set', 'STONE_C')])
talk('stair_open', 'MAIYA', "Dawn, noon and dusk, and the stair clicks open. Clever old builders.",
     cond=('ctr_eq', 'QUEST6', 3), alt='stair_open2', actions=[('ctr', 'QUEST6', 4)])
talk('stair_open2', 'MAIYA', "Dawn, noon and dusk, and the stair clicks open. Up to the observatory!")

g = canvas('T', ':')
put(g, 9, 13, 10, 13, ':'); put(g, 9, 0, 10, 0, ':')
for x, y in ((3, 3), (15, 3), (13, 12), (4, 12)):
    g[y][x] = 'A'
room('sun_terrace', 'The Sun Terrace', text(g),
     region=7, music='worldtree', restored_music='worldtree',
     exits=[exit_(9, 13, 2, 1, 'canopy', 9, 1, 'down'),
            exit_(9, 0, 2, 1, 'observatory', 9, 12, 'up', cond=('flag', 'STAIRS_OPEN'), locked='stair_hint')],
     objects=[
         emitter(1, 7, 'right', name='low_sun', hint='low_sun_hint'),
         mirror(8, 7, '/', name='bronze1', hint='bronze_hint'),
         mirror(8, 11, '/', name='bronze2', hint='bronze_hint'),
         receiver(8, 2, 'SUN_A', look='markstone', name='stone_a', hint='stone_hint', actions=[('talk', 'stone_a_lit')]),
         receiver(3, 11, 'SUN_B', look='markstone', name='stone_b', hint='stone_hint',
                  actions=[('talk', 'stone_b_lit')]),
         receiver(16, 11, 'SUN_C', look='markstone', name='stone_c', hint='stone_hint',
                  actions=[('talk', 'stone_c_lit')]),
         signal_flag('STONE_A', 'MA', name='mark_a'),
         signal_flag('STONE_B', 'MB', name='mark_b'),
         signal_flag('STONE_C', 'MC', name='mark_c'),
         gate(9, 0, w=2, inp=['MA', 'MB', 'MC'], flag_='STAIRS_OPEN', look='stonedoor', name='stair', hint='stair_hint',
              actions=[('talk', 'stair_open')]),
         sign(12, 5, 'terrace_tablet', look='sundial'),
         find(17, 1, 'flower', 'Sky Blue Gentian', 'flower_violet'),
         find(1, 12, 'history', 'Bronze Sundial Pin', 'badge'),
     ])

# ---- the calendar hall ---------------------------------------------------------------------------------------------
talk('calendar_mural', 'SIGN', "PAINTED ROUND THE WALL: THE TREE-KEEPERS COUNTED THEIR YEAR FROM THE DAY THE LEAVES "
     "FALL. THEN THE SEASONS, EACH IN ITS TURN.")
talk('dial_seed_hint', 'LUNA', "A dial with a sprouting seed: spring, sowing time.")
talk('dial_sun_hint', 'LUNA', "A dial with a blazing sun: summer, ripening time.")
talk('dial_leaf_hint', 'LUNA', "A dial with a falling leaf: autumn, harvest.")
talk('dial_snow_hint', 'LUNA', "A dial with a snowflake: winter, the rest.")
talk('calendar_done', 'LUNA', "Leaf, snow, seed, sun: their year, from the falling leaves. Something unlatched in the "
     "alcove.")
talk('alcove_hint', 'LUNA', "A locked alcove. The calendar dials work its latch.")
talk('domegear_found', 'MAIYA', "A big gear wrapped in waxed cloth, with DOME painted on it. Found you!",
     actions=[('ctr', 'QUEST6', 5)])

g = canvas('Q', 'f')
g[6][0] = 'f'
put(g, 14, 9, 18, 12, 'Q'); put(g, 15, 10, 17, 11, 'f'); g[9][16] = 'f'
room('calendar_hall', 'The Calendar Hall', text(g),
     region=7, music='worldtree', restored_music='worldtree',
     exits=[exit_(0, 6, 1, 1, 'canopy', 18, 6, 'left')],
     objects=[
         bell(4, 3, 0, look='dial_snow', name='snow', hint='dial_snow_hint'),
         bell(8, 3, 1, look='dial_seed', name='seed', hint='dial_seed_hint'),
         bell(12, 3, 2, look='dial_leaf', name='leaf', hint='dial_leaf_hint'),
         bell(16, 3, 1, look='dial_sun', name='sun', hint='dial_sun_hint'),
         sequence('year', ['leaf', 'snow', 'seed', 'sun'], 'CAL', flag_='CALENDAR_SET',
                  actions=[('talk', 'calendar_done')]),
         gate(16, 9, inp=['CAL'], flag_='ALCOVE_OPEN', look='woodgate', name='alcove', hint='alcove_hint'),
         chest(16, 11, 'DOMEGEAR', 1, flag_='DOMEGEAR_TAKEN', actions=[('talk', 'domegear_found')]),
         sign(9, 8, 'calendar_mural', look='tablet'),
         creature(5, 9, 'wander', 'SQUIRREL2', hits=1, damage=1, speed=14, look='squirrel', hint='squirrel_hint'),
         find(1, 12, 'diagram', 'Seasonal Calendar Wheel', 'scroll'),
         find(1, 1, 'history', "Tree-Keeper's Almanac", 'scroll'),
         find(11, 12, 'flower', 'Autumn Snowflake Bloom', 'flower_white'),
     ])

# ---- the observatory -------------------------------------------------------------------------------------------------
talk('weights_hint', 'LUNA', "The dome's weights: they want to turn the dome, through the drive gear.")
talk('domesock_hint', 'LUNA', "An empty housing for the dome's drive gear.")
talk('dome_hint', 'LUNA', "The dome's slit. When the drive turns, it opens to the sky.")
talk('scope_hint', 'LUNA', "The great telescope. Its objective cradle is empty... and it's the same size as the "
     "lighthouse cradle.")
talk('scope_need', 'LUNA', "The telescope needs an objective lens, a big precise one.")
talk('scope_set', 'LUNA', "The precision lens fits the telescope exactly. The builders made their instruments to "
     "share parts.", actions=[('ctr', 'QUEST6', 7)])
talk('dome_opens', 'MAIYA', "The dome is opening! I can see the sky!", actions=[('ctr', 'QUEST6', 6)])
talk('sefa6', 'SEFA', "There it is. Seven channels from seven wonders, all pointing to one great cavern under the "
     "valley floor: the reservoir.", cond=('flag', 'CHART_DRAWN'), alt='sefa6_ready', next='sefa6_end')
talk('sefa6_end', 'SEFA', "Take the chart. And girls: Director Valen's crews have been seen going down beneath the "
     "old mill with drills and pumps. Be careful.", next='sefa6_end2')
talk('sefa6_end2', 'LUNA', "Behind the watermill... the passage Sunboy found! Everything leads back to Willowbrook.",
     next='sefa6_end3')
talk('sefa6_end3', 'MAIYA', "Then that's where we're going. All the way down.", actions=[('event', 'CHAPTER_END')])
talk('sefa6_ready', 'SEFA', "The dome turns and the telescope sees! Now, the old builders aligned the seven wonders "
     "with the stars. Let me take the bearings...", cond=('flag', 'SCOPE_SET'), alt='sefa6_first', next='sefa6_ready2')
talk('sefa6_ready2', 'SEFA', "...north star, the twins, the plough's pointer... and every bearing crosses at the "
     "same place. Here, the chart.",
     actions=[('set', 'CHART_DRAWN'), ('give', 'NAVCHART'), ('ctr', 'QUEST6', 8), ('restore', 7),
              ('collect', 'WONDER6'), ('event', 'RESTORATION')])
talk('sefa6_first', 'SEFA', "Welcome to the top of the world! I am Sefa. Astronomy is the oldest map there is: the "
     "builders charted the valley from the stars.", next='sefa6_b')
talk('sefa6_b', 'SEFA', "But Smoggar's crews took the dome's drive gear, and the telescope's objective lens. If we "
     "can see the sky again, I can find where the valley's water gathers.")

g = canvas('l', ':')
put(g, 9, 13, 10, 13, ':')
put(g, 2, 9, 3, 11, 'J'); put(g, 16, 9, 17, 11, 'J')
room('observatory', 'The Observatory', text(g),
     region=7, music='worldtree', restored_music='restored',
     exits=[exit_(9, 13, 2, 1, 'sun_terrace', 9, 1, 'down')],
     objects=[
         gear(7, 3, 'weights', role=1, mesh=['domesock'], look='gear_big', hint='weights_hint'),
         gear(8, 3, 'domesock', role=0, mesh=['weights', 'dome'], item_='DOMEGEAR', fine=1, look='gear',
              hint='domesock_hint', flag_='DOMEGEAR_SET'),
         gear(9, 3, 'dome', role=2, mesh=['domesock'], out=['DOME'], look='gear_big', hint='dome_hint'),
         lock(11, 6, 'OPTLENS', used=0, flag_='SCOPE_SET', look='telescope', name='telescope', hint='scope_hint',
              text='scope_need', actions=[('talk', 'scope_set')]),
         npc(13, 8, 'SEFA', 'sefa6', name='sefa'),
         find(17, 1, 'diagram', 'Star Bearings of the Seven Wonders', 'scroll'),
         find(2, 1, 'history', 'Brass Astrolabe', 'compass'),
         find(5, 12, 'photo', 'The Valley by Starlight', 'photo'),
     ],
     triggers=[trigger('watch', once='DOME_OPEN', cond=('signal', 'DOME'), actions=[('talk', 'dome_opens')])])
