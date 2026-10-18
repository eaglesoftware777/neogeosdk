"""
The finds spread over the valley beyond each chapter's own: wild flowers, history,
engineering drawings and photograph spots, so the notebook's targets (70, 35, 28, 21, 14 and the
seven wonders) are all there to find. Each lies on open ground of a room declared earlier;
tools/check_world.c walks to every one.
"""
from worldlib import add_object, find

FINDS = [
    ('village', 8, 10, 'flower', "Lady's Bedstraw", 'flower_orange'),
    ('forest', 17, 5, 'flower', 'Yarrow', 'flower_white'),
    ('trailhead', 9, 6, 'flower', 'Red Campion', 'flower_red'),
    ('ravine', 13, 1, 'flower', 'Field Scabious', 'flower_violet'),
    ('old_bridge', 3, 12, 'flower', 'Ox-eye Daisy', 'flower_white'),
    ('station', 3, 5, 'flower', 'Cowslip', 'flower_orange'),
    ('crossroads', 8, 10, 'flower', 'Meadowsweet', 'flower_white'),
    ('harbour', 11, 1, 'flower', 'Ragged Robin', 'flower_red'),
    ('headland', 9, 12, 'flower', 'Tansy', 'flower_orange'),
    ('tidepools', 17, 6, 'flower', 'Knapweed', 'flower_violet'),
    ('grove_gate', 12, 7, 'flower', 'Field Pansy', 'flower_violet'),
    ('orchard', 2, 8, 'flower', 'Speedwell', 'flower_violet'),
    ('ditch', 18, 3, 'flower', 'Chicory', 'flower_violet'),
    ('harvester_yard', 8, 1, 'flower', 'Wild Thyme', 'flower_violet'),
    ('terraces', 10, 11, 'flower', "Bird's-foot Trefoil", 'flower_orange'),
    ('grotto_road', 12, 1, 'flower', 'Self-heal', 'flower_violet'),
    ('tree_roots', 6, 2, 'flower', 'Honeysuckle', 'flower_white'),
    ('canopy', 11, 10, 'flower', 'Teasel Bloom', 'flower_violet'),
    ('sun_terrace', 18, 7, 'flower', "Viper's Bugloss", 'flower_violet'),
    ('forest', 4, 12, 'history', "Rangers' Whistle", 'badge'),
    ('ravine', 2, 8, 'history', 'Old Ferry Ticket', 'note'),
    ('harbour', 15, 7, 'history', "Fisher's Net Float", 'shard'),
    ('harvester_yard', 9, 6, 'history', 'Harvest Sickle Blade', 'shard'),
    ('old_mine', 2, 12, 'history', 'Mine Token', 'coin'),
    ('observatory', 1, 8, 'history', 'Observatory Key Tag', 'key'),
    ('village', 14, 5, 'diagram', 'Village Channel Map', 'scroll'),
    ('mill', 1, 11, 'diagram', 'Mill Race Section', 'scroll'),
    ('lighthouse', 2, 1, 'diagram', 'Lighthouse Clock Plan', 'scroll'),
    ('orchard', 10, 7, 'diagram', 'Orchard Pipe Layout', 'scroll'),
    ('old_mine', 10, 8, 'diagram', 'Grotto Fan Gearing', 'scroll'),
    ('tree_roots', 4, 7, 'diagram', 'Cable Car Drive', 'scroll'),
    ('sluice_gallery', 18, 8, 'diagram', 'Sluice Gallery Plan', 'scroll'),
    ('drill_chamber', 4, 2, 'diagram', 'Drill Interlock Wiring', 'scroll'),
    ('village', 6, 1, 'photo', 'Willowbrook at Sunrise', 'photo'),
    ('harbour', 9, 5, 'photo', 'The Harbour Pier', 'photo'),
    ('orchard', 9, 10, 'photo', 'The Clock Tower', 'photo'),
]

for _room, _x, _y, _cat, _title, _look in FINDS:
    add_object(_room, find(_x, _y, _cat, _title, _look))
