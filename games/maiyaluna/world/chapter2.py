"""
Chapter II: The Emerald Labyrinth (Emerald Forest).

Landslides and fallen trees have cut the forest research station off from
the villages, and with it the old survey of which forest roads are safe.
Mina camps at the trailhead south of the forest path. The way round is
through the old hedge labyrinth (its trail markers point the way), across
the misty ravine in the pulley basket (it needs a new rope), over the old
suspension bridge (three planks missing) and through the bog garden, where
only the deer tracks are firm ground. In the station the Forest Survey Map
waits, and its semaphore mast tells the road crews in every village which
roads are safe again.

Puzzles: the trail markers through the labyrinth, the winch rope and the
basket's timing, the bridge planks (one behind a gap only Luna fits, one
on a ledge only Maiya climbs to), the deer tracks over the bog, the
semaphore code. Hidden: Piko's hollow behind the labyrinth's hedge.
"""
import random

from worldlib import (room, exit_, npc, sign, pickup, chest, rest, secret, gate, lever, sequence, creature, trigger,
                      talk, item, chapter, counter, signal_flag, lock, bridge, platform, find, collectible, W, H,
                      maiya, luna, J, say)

# ---- items (appended to Chapter I's: the save's item numbers) ------------------------
item('ROPE', 'tool', 'Coil of Rope', 'Strong hemp rope from Mina. A pulley line is only as good as its rope.')
item('PLANK', 'material', 'Bridge Plank', 'A sound oak plank, the length of a bridge step.', stack=3)
item('SURVEYMAP', 'key|record', 'Forest Survey Map',
     "Every forest road surveyed. Signal for ROADS SAFE: right arm, left arm, then the lamp.")

collectible('WONDER2', 'wonder', 'Forest Survey Map')

counter('QUEST2')   # 0 start, 1 Mina's errand, 2 through the labyrinth, 3 over the ravine, 4 at the station,
                    # 5 the roads are safe

chapter(2, 'The Emerald Labyrinth', 'Emerald Forest', 'Forest Survey Map',
        start=('trailhead', 15, 2, 'down'), quest='QUEST2', wonder='SURVEYMAP',
        restore_title='The roads are safe again',
        restore_text='Road crews in every village\nread the station mast and\nclear the marked roads.',
        back=('station', 9, 4, 'down'),
        end_text='The valley is joined again.\nCaptain Orin sends word:\nthe lighthouse is dark.',
        goals=[(0, "Mina is camping at the Emerald Trailhead, south of the forest path. Ask her about the "
                    "research station."),
               (1, "Find the way through the Emerald Labyrinth. Its old trail markers point the way."),
               (2, "Cross the misty ravine. The pulley basket needs a new rope on its winch."),
               (3, "Mend the old suspension bridge: three planks are missing. Look high and low."),
               (4, "Reach the station across the bog garden. Find the survey map and signal the villages."),
               (5, "The roads are safe again. Sunboy has news from the coast.")])

# ---- the Emerald Trailhead -------------------------------------------------------------
say('ch2_intro', "Fallen oaks everywhere. The whole forest looks like a giant dropped its matchsticks. And Mina's "
    "tent is just over there!", "Fallen oaks everywhere. The research station is somewhere past all of it, and "
    "Mina's tent is just over there.", actions=[('ctr', 'QUEST2', 0)])

talk('mina2', 'MINA', "The basket over the ravine should run now, and the planks will be wherever the storm "
     "threw them. Look high and low.", cond=('ctr_ge', 'QUEST2', 1), alt='mina2_first')
talk('mina2_first', 'MINA', "You came! The landslide buried the old trail to the research station. The survey "
     "of every forest road is up there, and the villages need it.", next='mina2_b')
talk('mina2_b', 'MINA', "The long way goes through the old hedge labyrinth and over the misty ravine. The "
     "ravine has a pulley basket, but its rope rotted. Take mine.", next='mina2_c',
     actions=[('give', 'ROPE'), ('ctr', 'QUEST2', 1)])
say('mina2_c', "A maze, a rope and a ravine. This is the best day. I'll follow the markers. Mostly.",
    "The markers in the labyrinth were carved by the station rangers. I just have to read them.")
talk('mina2_done', 'MINA', "Every road crew in the valley is out with saws and shovels. The survey made all the "
     "difference.")
talk('marker_rule', 'SIGN', "TRAIL RULE OF THE STATION RANGERS: IN THE LABYRINTH, GO THE WAY THE OAK LEAF ON EACH "
     "MARKER POINTS.")
say('camp_table', "Mina's field notes. Pressed leaves, sketches of seedlings, and one very good drawing of a "
    "beetle.")
talk('piko_trail', 'PIKO', J("Piko paws at the ivy on the trees by the old camp, sneezes, and looks back at Maiya.",
                             "Piko pushes his nose into a gap between the trees, sneezes, and looks back at Luna."),
     cond=('not', 'NOOK_PLANK'), alt='piko_trail2')
talk('piko_trail2', 'PIKO', "Piko trots ahead, tail high, then waits for you to catch up.")
say('nook_hint', "A gap between the trunks. Too narrow for my shoulders. But that ivy on the trees...",
    "A gap between the trunks. Too narrow for most people, but I can squeeze through.")
talk('landslide', 'SIGN', "A landslide of rock and roots buries the old trail to the station.")
talk('south_locked', 'SIGN', "A fallen oak lies across the south trail. The forest crews will need a while.")

room('trailhead', 'The Emerald Trailhead', [
    "TTTTTTTTTTTTTTT==TTT",
    "TT..,..TTTTT...==.TT",
    "T..E....TT....==...T",
    "T..ff.,......===..,T",
    "T..ff....=====.....T",
    "T,......==....,....T",
    "TTTnTTT.=.....=====.",
    "T,..TTT.=.....=..==.",
    "T..,.vv.=....b.....T",
    "TTTTTTT.=...bbb..,.T",
    "T.......=.....b....T",
    "T..,....=..........T",
    "TTTTTT..==.....,...T",
    "TTTTTTTT==TTTTTTTTTT"],
    region=3, music='labyrinth',
    exits=[exit_(15, 0, 2, 1, 'forest', 15, 12, 'up'),
           exit_(8, 13, 2, 1, 'labyrinth', 7, 1, 'down'),
           exit_(19, 6, 1, 2, 'station', 1, 8, 'right', cond=('flag', 'ROADS_OPEN'), locked='landslide')],
    objects=[
        npc(5, 3, 'MINA', 'mina2', name='mina2'),
        npc(12, 10, 'PIKO', 'piko_trail', name='piko', wander=1, speed=10),
        sign(3, 2, 'camp_table', look='none'),
        sign(10, 12, 'marker_rule', look='signpost'),
        gate(17, 6, w=2, h=2, flag_='ROADS_OPEN', look='rocks', name='landslide', hint='nook_hint'),
        pickup(2, 8, 'PLANK', 1, flag_='NOOK_PLANK', look='plank', hint='nook_hint'),
        find(1, 5, 'flower', 'Wood Anemone', 'flower_white'),
        find(3, 8, 'flower', 'Bluebell', 'flower_violet'),
        find(17, 11, 'flower', 'Foxglove', 'flower_red'),
        find(18, 3, 'specimen', 'Turkey Tail Fungus', 'moss'),
        find(1, 11, 'history', 'Ranger Station Token', 'coin'),
        pickup(16, 9, 'APPLE', 1, flag_='APPLE_TRAILHEAD'),
    ],
    triggers=[trigger('room', once='CH2_BEGUN', actions=[('talk', 'ch2_intro')])])

# ---- the Emerald Labyrinth: a hedge maze of 2-wide paths, from a fixed seed --------------
# Cells are 2 x 2 tiles with a hedge between: 6 across (x = 1, 4, ... 16), 4 down (y = 1, 4, 7, 10).
CW, CH = 6, 4
ENTRY, EXIT = (2, 0), (5, 2)


def make_maze(seed=2026):
    rng = random.Random(seed)
    grid = [['C'] * W for _ in range(H)]
    open_ = {}
    for cy in range(CH):
        for cx in range(CW):
            open_[(cx, cy)] = set()
            for dy in range(2):
                for dx in range(2):
                    grid[1 + cy * 3 + dy][1 + cx * 3 + dx] = '='
    seen, stack = {ENTRY}, [ENTRY]
    while stack:
        cx, cy = stack[-1]
        nbs = [(cx + dx, cy + dy, d) for dx, dy, d in ((1, 0, 'E'), (-1, 0, 'W'), (0, 1, 'S'), (0, -1, 'N'))
               if 0 <= cx + dx < CW and 0 <= cy + dy < CH and (cx + dx, cy + dy) not in seen]
        if not nbs:
            stack.pop()
            continue
        nx, ny, d = rng.choice(nbs)
        open_[(cx, cy)].add(d)
        open_[(nx, ny)].add({'E': 'W', 'W': 'E', 'S': 'N', 'N': 'S'}[d])
        for k in range(2):      # knock the hedge between them out, both tiles
            if d in 'EW':
                wx = 1 + max(cx, nx) * 3 - 1
                grid[1 + cy * 3 + k][wx] = '='
            else:
                wy = 1 + max(cy, ny) * 3 - 1
                grid[wy][1 + cx * 3 + k] = '='
        seen.add((nx, ny))
        stack.append((nx, ny))
    # the way in (north) and out (east)
    grid[0][1 + ENTRY[0] * 3] = grid[0][2 + ENTRY[0] * 3] = '='
    for k in range(2):
        grid[1 + EXIT[1] * 3 + k][W - 2] = '='
        grid[1 + EXIT[1] * 3 + k][W - 1] = '='
    return grid, open_


def maze_path(open_):
    step = {'E': (1, 0), 'W': (-1, 0), 'S': (0, 1), 'N': (0, -1)}
    prev, todo = {ENTRY: None}, [ENTRY]
    while todo:
        c = todo.pop(0)
        for d in open_[c]:
            n = (c[0] + step[d][0], c[1] + step[d][1])
            if n not in prev:
                prev[n] = (c, d)
                todo.append(n)
    path, c = [], EXIT
    while prev[c]:
        p, d = prev[c]
        path.append((p, d))
        c = p
    return path[::-1], prev


GRID, OPEN = make_maze()
PATH, _ = maze_path(OPEN)
on_path = {c for c, _ in PATH} | {EXIT}
dead_ends = [c for c in OPEN if len(OPEN[c]) == 1 and c not in (ENTRY, EXIT) and c not in on_path]
# the hollow behind the hedge: a dead end on the bottom row, reached through the south hedge
HOLLOW_CELL = next((c for c in dead_ends if c[1] == CH - 1), None) or next(c for c in OPEN if c[1] == CH - 1 and c not in on_path)
hx = 1 + HOLLOW_CELL[0] * 3
GRID[12][hx] = '='
GRID[13][hx] = '='
MAZE_ROWS = [''.join(r) for r in GRID]
WORDS = {'E': 'EAST', 'W': 'WEST', 'S': 'SOUTH', 'N': 'NORTH'}

maze_objects = []
# a marker in every cell of the true way where the hedges fork, in the cell's top-left corner
k = 0
for (c, d) in PATH:
    if len(OPEN[c]) >= 3 or c == ENTRY:
        talk(f'marker{k}', 'SIGN', f"A RANGER'S TRAIL MARKER. A CARVED OAK LEAF POINTS {WORDS[d]}.")
        maze_objects.append(sign(1 + c[0] * 3, 1 + c[1] * 3, f'marker{k}', look='signpost'))
        k += 1
# finds in the dead ends; the farthest one keeps the second plank
far = sorted(dead_ends, key=lambda c: -(abs(c[0] - ENTRY[0]) + abs(c[1] - ENTRY[1])))
FIND_SPOTS = [('flower', 'Wild Rose', 'flower_red'), ('flower', 'Lady Fern', 'flower_white'),
              ('photo', 'The Labyrinth from Above', 'photo'), ('history', 'Old Hedge Shears', 'shard'),
              ('flower', 'Woodruff', 'flower_white'), ('diagram', 'Hedge Planting Plan', 'scroll')]
plank_cell = far[0] if far and far[0] != HOLLOW_CELL else (far[1] if len(far) > 1 else (0, 0))
maze_objects.append(pickup(2 + plank_cell[0] * 3, 2 + plank_cell[1] * 3, 'PLANK', 1, flag_='MAZE_PLANK', look='plank'))
spots = [c for c in far if c not in (plank_cell, HOLLOW_CELL)]
for c, (cat, title, look) in zip(spots, FIND_SPOTS):
    maze_objects.append(find(2 + c[0] * 3, 2 + c[1] * 3, cat, title, look))

say('wasp_hint', "Wasps from a nest in the hedge. They only sting when you get close. A crack of my whip and "
    "they buzz off.", "Wasps from a nest in the hedge. They only sting when you get close: I'll give them room.")
say('hollow_hedge', "Cold air comes through this hedge, and Piko keeps sniffing at it. It is thin here: a "
    "closer look with the lens might show the way.")
say('hollow_found', "A tunnel through the hedge! Piko, you clever thing.")

wasp_cells = [c for c, _ in PATH[1:3]]
for i, c in enumerate(wasp_cells):
    maze_objects.append(creature(2 + c[0] * 3, 1 + c[1] * 3, 'wander', f'WASP{i + 1}', hits=2, damage=1, speed=10,
                                 look='wasp', hint='wasp_hint'))
maze_objects.append(secret(hx, 12, 'HOLLOW_FOUND', solid=1, look='hedgegate', hint='hollow_hedge',
                           actions=[('talk', 'hollow_found')]))

room('labyrinth', 'The Emerald Labyrinth', MAZE_ROWS, region=3, music='labyrinth',
     exits=[exit_(1 + ENTRY[0] * 3, 0, 2, 1, 'trailhead', 8, 12, 'up'),
            exit_(W - 1, 1 + EXIT[1] * 3, 1, 2, 'ravine', 1, 6, 'right'),
            exit_(hx, 13, 1, 1, 'fox_hollow', 9, 1, 'down')],
     objects=maze_objects,
     triggers=[trigger('enter', W - 3, 1 + EXIT[1] * 3, 2, 2, once='MAZE_SOLVED', actions=[('ctr', 'QUEST2', 2)])])

# ---- Piko's hollow (hidden) --------------------------------------------------------------
talk('hollow_spring', 'SIGN', "A spring bubbles up between mossy stones, cold and clear. (Hearts restored, and the "
     "journey can be saved here.)")
say('hollow_den', "An old fox den under the roots. No wonder Piko knew the way.")

room('fox_hollow', "Piko's Hollow", [
    "CCCCCCCCC=CCCCCCCCCC",
    "CC,.....,.,.....,.CC",
    "C..,..TT.....TT..,.C",
    "C.,...TT..,..TT....C",
    "C..........o.......C",
    "C.,..,.......,..,..C",
    "C....rr........b...C",
    "C..,.rr..,..,..bb.,C",
    "C.........,........C",
    "CC.,..,......,..,.CC",
    "CCC..............CCC",
    "CCCC,..,.,...,..CCCC",
    "CCCCCCC......CCCCCCC",
    "CCCCCCCCCCCCCCCCCCCC"],
    region=3, music='secret',
    exits=[exit_(9, 0, 1, 1, 'labyrinth', hx, 11, 'up')],
    objects=[
        rest(11, 4, text='hollow_spring', look='none'),
        sign(5, 6, 'hollow_den', look='none'),
        find(2, 1, 'flower', 'Primrose', 'flower_white'),
        find(17, 2, 'flower', 'Wood Violet', 'flower_violet'),
        find(4, 11, 'flower', 'Bird Cherry Blossom', 'flower_white'),
        find(16, 8, 'specimen', 'Fox Moss', 'moss'),
        find(9, 11, 'photo', "Piko's Hollow", 'photo'),
        chest(14, 9, 'APPLE', 3, flag_='HOLLOW_CHEST'),
    ])

# ---- the Misty Ravine ------------------------------------------------------------------
say('winch_need', "The pulley winch. Its rope rotted through: the basket can't move without a new one.")
say('winch_hint', "The winch for the pulley basket. With a sound rope on the drum, the basket runs back and forth "
    "over the ravine by itself.")
say('basket_hint', "The basket waits a moment at each side. Step in when it is next to me.")
say('winch_done', "Rope's on! Look at it go. All aboard the basket!", "There. The rope runs true, and the basket "
    "goes back and forth by itself.")
talk('ravine_sign', 'SIGN', "MISTY RAVINE FERRY. ONE BASKET, TWO PERSONS. KEEP YOUR HANDS INSIDE.")
say('ledge_vines', "Ivy all the way up to that shelf, and something wooden lying up there. Easy climb!",
    "Ivy up to that shelf: I can't climb it. But the rock below has a crack I could squeeze through, and "
    "something wooden is lying up there.")
say('ch2_ravine', "The station rangers' markers were right. There is the ravine, and the old basket.",
     actions=[('ctr', 'QUEST2', 2)])

room('ravine', 'The Misty Ravine', [
    "##########pp######==",
    "#..,......pp......==",
    "#...,..,..pp...,...#",
    "#.........pp.......#",
    "#.,.......pp.......#",
    "#.........pp.......#",
    "=.........pp.......#",
    "=......,..pp...,...#",
    "#.........pp..#v####",
    "#....,....pp..#....#",
    "#.........pp..n.,..#",
    "#..,......pp..#....#",
    "#.........pp..#....#",
    "##########pp########"],
    region=3, music='labyrinth',
    exits=[exit_(0, 6, 1, 2, 'labyrinth', W - 2, 1 + EXIT[1] * 3, 'left'),
           exit_(18, 0, 2, 1, 'old_bridge', 17, 12, 'up')],
    objects=[
        lock(9, 5, 'ROPE', used=1, flag_='WINCH_ROPED', look='winch', name='winch', hint='winch_hint',
             text='winch_need', actions=[('talk', 'winch_done')]),
        signal_flag('WINCH_ROPED', 'ROPE_ON', name='rope_on'),
        platform(10, 6, 1, 0, speed=6, wait=50, inp=['ROPE_ON'], look='basket', name='basket', hint='basket_hint'),
        sign(8, 8, 'ravine_sign', look='signpost'),
        sign(16, 7, 'ledge_vines', look='none'),
        pickup(17, 11, 'PLANK', 1, flag_='LEDGE_PLANK', look='plank'),
        find(2, 2, 'flower', 'Marsh Marigold', 'flower_orange'),
        find(12, 11, 'flower', 'Water Avens', 'flower_red'),
        find(5, 11, 'diagram', 'Pulley Ferry Drawing', 'scroll'),
        find(18, 5, 'photo', 'Mist over the Ravine', 'photo'),
    ],
    triggers=[trigger('room', once='RAVINE_SEEN', cond=('ctr_ge', 'QUEST2', 1), actions=[('talk', 'ch2_ravine')]),
              trigger('enter', 12, 1, 8, 12, once='RAVINE_CROSSED', actions=[('ctr', 'QUEST2', 3)])])

# ---- the Old Suspension Bridge -------------------------------------------------------------
say('plank_need', "The ropes are still sound. It just needs a plank here. There should be three somewhere.")
say('plank_hint', "A gap in the bridge deck. One plank would close it.")
say('bridge_done', "Three planks, one bridge! Across I go. Carefully. Very carefully.",
    "Three planks, one bridge. Now: slowly, and not looking down.",
     actions=[('ctr', 'QUEST2', 4)])
talk('bridge_sign', 'SIGN', "OLD SUSPENSION BRIDGE. BUILT BY THE FOREST RANGERS. ONE PERSON AT A TIME IN A STORM.")

room('old_bridge', 'The Old Suspension Bridge', [
    "TTTTTTTTT==TTTTTTTTT",
    "T...,....==......,.T",
    "T..,.....==...,....T",
    "T.....,..==........T",
    "#########.##########",
    "pppppppppppppppppppp",
    "pppppppppppppppppppp",
    "pppppppppppppppppppp",
    "#########.##########",
    "T........=.........T",
    "T..,.....=.....,...T",
    "T........=======...T",
    "T...,.......,...==.T",
    "TTTTTTTTTTTTTTTTT==T"],
    region=3, music='labyrinth',
    exits=[exit_(17, 13, 2, 1, 'ravine', 18, 1, 'down'),
           exit_(9, 0, 2, 1, 'station', 9, 12, 'up')],
    objects=[
        lock(9, 7, 'PLANK', used=1, flag_='PLANK_A', name='gap_a', hint='plank_hint', text='plank_need'),
        bridge(9, 7, flag_='PLANK_A', look='planks', name='deck_a'),
        lock(9, 6, 'PLANK', used=1, flag_='PLANK_B', name='gap_b', hint='plank_hint', text='plank_need'),
        bridge(9, 6, flag_='PLANK_B', look='planks', name='deck_b'),
        lock(9, 5, 'PLANK', used=1, flag_='PLANK_C', name='gap_c', hint='plank_hint', text='plank_need',
             actions=[('talk', 'bridge_done')]),
        bridge(9, 5, flag_='PLANK_C', look='planks', name='deck_c'),
        sign(11, 9, 'bridge_sign', look='signpost'),
        find(2, 2, 'flower', 'Harebell', 'flower_violet'),
        find(16, 10, 'history', 'Ranger Bridge Plaque', 'badge'),
        find(17, 2, 'specimen', 'Old Man of the Woods', 'moss'),
    ])

# ---- the Forest Research Station -----------------------------------------------------------
talk('bog_sign', 'SIGN', "STATION BOG GARDEN. DEER NEVER STEP WHERE THE GROUND SINKS: KEEP TO THEIR TRACKS. THE "
     "REEDS GROW ONLY IN THE WET.")
talk('mast_sign', 'SIGN', "SIGNAL MAST. RIGHT ARM, LEFT ARM, LAMP. THE CODES ARE IN THE SURVEY.")
say('arm_r', "The lever for the mast's right arm.")
say('arm_l', "The lever for the mast's left arm.")
say('arm_lamp', "The lever for the signal lamp at the top of the mast.")
say('semaphore_done', "Right arm, left arm, lamp: ROADS SAFE. Every crew from here to the coast can see the mast!",
    actions=[('set', 'SEMAPHORE_SENT')])
talk('sunboy2', 'SUNBOY', "I ran all the way along the new road! Captain Orin sent a message from the coast: the "
     "old lighthouse has gone dark, and ships can't find the harbour.", next='sunboy2_b')
say('sunboy2_b', "Then the coast is next. Race you to the sea, Sunboy! ...Walking. I meant walking.",
    "Then the coast is next. Another wonder. Tell Maiya I'll write it all down for her.",
    actions=[('event', 'CHAPTER_END')])
talk('valen2_station', 'VALEN', "A survey of every road, kept current by rangers on foot. Charming. With an office "
     "and engineers, one could do this for the whole valley at once.", next='valen2_b')
say('valen2_b', "The rangers know which roads the deer use, and where the ground sinks. Does your office know "
    "that?", "The rangers knew which roads the deer use, and where the ground sinks. Could your office know that?",
    next='valen2_c')
talk('valen2_c', 'VALEN', "A fair question, young lady. I will think about it.")

def station_rows():
    rows = [list(r) for r in [
        "TTTTTTTTTTTTTTTT==TT",
        "T..RRRRRR..........T",
        "T..RRRRRR..........T",
        "T..HHdHHH..........T",
        "T....=.............T",
        "T....=======.......T",
        "T..................T",
        "T..................T",
        "=..................T",
        "=..................T",
        "T..................T",
        "T..................T",
        "T...........=......T",
        "TTTTTTTTT==TTTTTTTTT"]]
    for y in range(6, 12):
        for x in range(1, 19):
            rows[y][x] = 'q'
    # the deer's way over the bog, and the short tracks that go nowhere
    track = [(10, 11), (10, 10), (10, 9), (11, 9), (12, 9), (12, 8), (12, 7), (13, 7), (14, 7), (14, 6),
             (1, 9), (2, 9), (3, 9), (3, 8), (3, 7), (3, 6),
             (5, 11), (5, 10), (6, 10), (16, 10), (16, 9), (7, 7), (8, 7)]
    for x, y in track:
        rows[y][x] = '_'
    return [''.join(r) for r in rows]


room('station', 'The Forest Research Station', station_rows(),
    region=3, music='labyrinth', restored='station_restored', restored_music='restored',
    exits=[exit_(9, 13, 2, 1, 'old_bridge', 9, 1, 'down'),
           exit_(5, 3, 1, 1, 'station_inside', 9, 12, 'up'),
           exit_(0, 8, 1, 2, 'trailhead', 18, 6, 'left')],
    objects=[
        sign(2, 12, 'bog_sign', look='signpost'),
        sign(15, 4, 'mast_sign', look='signpost'),
        lever(12, 1, 'ARM_R', look='lever', name='arm_r', hint='arm_r'),
        lever(13, 1, 'ARM_L', look='lever', name='arm_l', hint='arm_l'),
        lever(14, 1, 'LAMP', look='lever', name='lamp', hint='arm_lamp'),
        sequence('semaphore', ['arm_r', 'arm_l', 'lamp'], 'SIGNALLED', flag_='SEMAPHORE_SEQ',
                 actions=[('talk', 'semaphore_done')]),
        npc(10, 4, 'SUNBOY', 'sunboy2', name='sunboy2', only=('set', 'ROADS_OPEN')),
        npc(16, 12, 'VALEN', 'valen2_station', name='valen2', only=('set', 'ROADS_OPEN')),
        find(16, 9, 'flower', 'Bog Cotton', 'flower_white'),
        find(5, 11, 'flower', 'Sundew', 'flower_red'),
        find(18, 1, 'photo', 'The Signal Mast', 'photo'),
    ],
    triggers=[trigger('watch', once='ROADS_RESTORED', cond=('flag', 'SEMAPHORE_SENT'),
                      actions=[('set', 'ROADS_OPEN'), ('restore', 3), ('ctr', 'QUEST2', 5), ('collect', 'WONDER2'),
                               ('event', 'RESTORATION')])])

# ---- inside the station ------------------------------------------------------------------------
say('map_case', "The survey map! Every road, every bridge, every place the ground is soft. And the signal codes "
    "in the margin.", actions=[('ctr', 'QUEST2', 4)])
talk('ranger_log', 'SIGN', "RANGER'S LOG: STORM AGAIN. THE ARMS OF THE MAST SAY ROADS CLOSED UNTIL WE CAN SURVEY. "
     "DEER TRACKS STILL THE SAFEST WAY OVER THE BOG.")
say('station_desk', "Instruments everywhere: a barometer, a rain gauge, a drawer full of pressed leaves.")

room('station_inside', 'Inside the Research Station', [
    "HHHHHHHHHHHHHHHHHHHH",
    "HJJJJJffffffffJJJJJH",
    "HffffffffffffffffffH",
    "HfEEffffffffffffEEfH",
    "HfEEffffffffffffEEfH",
    "HffffffffffffffffffH",
    "HffffffJJJJJJffffffH",
    "HffffffffffffffffffH",
    "HfEEEfffffffffEEEffH",
    "HffffffffffffffffffH",
    "HffffffffffffffffffH",
    "HJJffffffffffffffJJH",
    "HffffffffffffffffffH",
    "HHHHHHHHHddHHHHHHHHH"],
    region=3, music='secret', lit=1,
    exits=[exit_(9, 13, 2, 1, 'station', 5, 4, 'down')],
    objects=[
        chest(9, 4, 'SURVEYMAP', 1, flag_='MAP_TAKEN', actions=[('talk', 'map_case')]),
        sign(3, 3, 'station_desk', look='none'),
        sign(16, 8, 'ranger_log', look='note'),
        find(2, 9, 'diagram', 'Signal Mast Plan', 'scroll'),
        find(17, 5, 'history', 'Ranger Compass', 'compass'),
        find(10, 10, 'diagram', 'Bog Garden Survey', 'scroll'),
    ])
