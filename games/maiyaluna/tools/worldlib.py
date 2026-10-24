"""
Maiya & Luna world authoring: the vocabulary the world files use, and the
writer that turns them into C tables for the SDK's RPG engine (ng_rpg.h).

A world file (world/*.py) calls room(), the object helpers, talk() and the
item / flag / collectible declarations below. tools/world.py imports them,
checks them and writes scenes/ml_world_data.h; tools/art.py reads the same
rooms to paint their backgrounds, so a map and its picture never disagree.

Maps are 20 x 14 characters (one 320 x 224 screen of 16-pixel tiles). The
top row sits under the status line: keep it scenery or an exit.

Journeys: the player sets out as Maiya or as Luna, alone, and the two journeys
pass through the same rooms with puzzles of their own (the sisters meet only
in Chapter VI). maiya(...) and luna(...) mark objects, exits and triggers that
exist in one journey only; J(maiya=..., luna=...) is a value (a text, a speaker,
a node name, a goal...) that differs between them. A conversation written with
a J anywhere in it becomes two nodes, NAME@maiya and NAME@luna, and every
reference to NAME takes its own journey's; nodes leading to them are split the
same way. The writer emits one world per journey (ml_worlds[ML_J_MAIYA],
ml_worlds[ML_J_LUNA]), sharing every table that is the same in both.
"""

import textwrap

W, H = 20, 14

# ---- tiles ----------------------------------------------------------------
SOLID, WATER, PIT, NARROW, CLIMB, HAZARD, DOOR, LEDGE = 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80

# char: (texture, tile flags, carries a water channel)
LEGEND = {
    '.': ('grass', 0, 0), ',': ('flowers', 0, 0), '=': ('path', 0, 0), ':': ('stone', 0, 0),
    'f': ('wood', 0, 0), 'F': ('field', 0, 0), 'S': ('steps', 0, 0), 'B': ('bridge', 0, 0),
    'g': ('cavefloor', 0, 0), 'e': ('cavemouth', DOOR, 0), 'd': ('doorway', DOOR, 0),
    'T': ('tree', SOLID, 0), 'b': ('bush', SOLID, 0), 'r': ('rock', SOLID, 0), '#': ('cliff', SOLID, 0),
    'X': ('cavewall', SOLID, 0), 'R': ('roof', SOLID, 0), 'H': ('wall', SOLID, 0), 'D': ('door', SOLID, 0),
    'x': ('fence', SOLID, 0), 'o': ('well', SOLID, 0), 'M': ('millwall', SOLID, 0), 'W': ('wheel', SOLID, 0),
    'P': ('millroof', SOLID, 0), 'K': ('crystal', SOLID, 0), 'Y': ('stall', SOLID, 0),
    '~': ('water', WATER, 0), 'w': ('falls', SOLID, 0), 'p': ('pit', PIT, 0), 'n': ('narrow', NARROW, 0),
    'v': ('vines', CLIMB, 0), 'h': ('thorns', HAZARD, 0), 'L': ('ledge', LEDGE, 0),
    'c': ('channel', SOLID, 1), 'k': ('channelbridge', 0, 1), 'u': ('race', SOLID, 1),
    's': ('sand', 0, 0), 'O': ('sea', WATER, 0), 'C': ('hedge', SOLID, 0), 'm': ('metal', 0, 0),
    'I': ('iron', SOLID, 0), 'l': ('tower', SOLID, 0), 'Z': ('pipe', SOLID, 0), 'Q': ('bark', SOLID, 0),
    'A': ('fruittree', SOLID, 0), 'V': ('rails', 0, 0), 'q': ('mire', HAZARD, 0), '_': ('tracks', 0, 0),
    'U': ('boat', SOLID, 0), 'E': ('desk', SOLID, 0), 'J': ('shelf', SOLID, 0),
}

# the painter's light for each region (tools/ml_tiles.py THEMES)
REGION_THEME = {1: 'valley', 2: 'valley', 3: 'forest', 4: 'coast', 5: 'autumn', 6: 'grotto', 7: 'tree', 8: 'works'}

# ---- object types (ng_rpg.h order) ------------------------------------------
TYPES = ['NONE', 'NPC', 'SIGN', 'PICKUP', 'CHEST', 'REST', 'LOCK', 'SECRET', 'BLOCK', 'PLATE', 'LEVER',
         'GATE', 'BRIDGE', 'GEAR', 'WATER_SRC', 'VALVE', 'OUTLET', 'EMITTER', 'MIRROR', 'LENS', 'RECEIVER',
         'BELL', 'SEQUENCE', 'PAN', 'BALANCE', 'PLATFORM', 'HAZARD', 'CREATURE', 'SIGNAL_FLAG']

CONDS = {'always': 0, 'flag': 1, 'not': 2, 'item': 3, 'ctr_ge': 4, 'ctr_eq': 5, 'leader': 6, 'signal': 7,
         'collected': 8}
ACTS = {'set': 1, 'clear': 2, 'give': 3, 'take': 4, 'ctr': 5, 'add': 6, 'talk': 7, 'checkpoint': 8,
        'restore': 9, 'heal': 10, 'teleport': 11, 'collect': 12, 'event': 13, 'open': 14}
FACES = {'right': 0, 'down': 1, 'left': 2, 'up': 3}

# ---- registries --------------------------------------------------------------
ROOMS = []           # in declaration order: the room number
FLAGS = ['NONE']     # flag 0 means "none"
COUNTERS = []
ITEMS = []           # (name, flags, max_stack, param, weight, title, text)
COLLECT = []         # (name, category, title)
SPEAKERS = []        # (name, label)
NODES = {}           # dialogue: name -> dict
NODE_ORDER = []
EVENTS = []          # game events (NGR_A_EVENT a)
CHAPTERS = []

TEXT_W, TEXT_LINES = 28, 4

# ---- journeys ------------------------------------------------------------------
JOURNEYS = ('maiya', 'luna')


class J:
    """A value that differs between Maiya's journey and Luna's."""

    def __init__(self, maiya, luna):
        self.maiya, self.luna = maiya, luna

    def of(self, j):
        return self.maiya if j == 'maiya' else self.luna


def pick(v, j):
    """v with every J in it replaced by journey j's value."""
    if isinstance(v, J):
        return pick(v.of(j), j)
    if isinstance(v, Obj):
        o = Obj(); o.update({k: pick(x, j) for k, x in v.items()}); return o
    if isinstance(v, dict):
        return {k: pick(x, j) for k, x in v.items()}
    if isinstance(v, list):
        return [pick(x, j) for x in v]
    if isinstance(v, tuple):
        return tuple(pick(x, j) for x in v)
    return v


def has_j(v):
    if isinstance(v, J):
        return True
    if isinstance(v, dict):
        return any(has_j(x) for x in v.values())
    if isinstance(v, (list, tuple)):
        return any(has_j(x) for x in v)
    return False


def _only(j, things):
    out = []
    for t in things:
        t = Obj(t) if isinstance(t, Obj) else dict(t)
        t['journey'] = j
        out.append(t)
    return out


def maiya(*things):
    """Objects, exits or triggers that are there in Maiya's journey only."""
    return _only('maiya', things)


def luna(*things):
    """Objects, exits or triggers that are there in Luna's journey only."""
    return _only('luna', things)


def in_journey(thing, j):
    return thing.get('journey') in (None, j)


def flag(name):
    if name not in FLAGS:
        FLAGS.append(name)
    return FLAGS.index(name)


def counter(name):
    if name not in COUNTERS:
        COUNTERS.append(name)
    return COUNTERS.index(name)


def item(name, kind, title, text, stack=1, param=0, weight=0):
    bits = {'key': 0x01, 'consume': 0x02, 'tool': 0x04, 'collect': 0x08, 'material': 0x10, 'record': 0x20}
    f = 0
    for k in kind.split('|'):
        f |= bits[k]
    ITEMS.append((name, f, stack, param, weight, title, text))


def item_id(name):
    for i, it in enumerate(ITEMS):
        if it[0] == name:
            return i
    raise SystemExit(f"world: no item {name}")


def collectible(name, category, title):
    COLLECT.append((name, category, title))


def collect_id(name):
    for i, c in enumerate(COLLECT):
        if c[0] == name:
            return i
    raise SystemExit(f"world: no collectible {name}")


def speaker(name, label):
    SPEAKERS.append((name, label))


def speaker_id(name):
    for i, s in enumerate(SPEAKERS):
        if s[0] == name:
            return i
    raise SystemExit(f"world: no speaker {name}")


def event(name):
    if name not in EVENTS:
        EVENTS.append(name)
    return EVENTS.index(name) + 1


def chapter(number, title, place, reward, start, quest, wonder, restore_title, restore_text, back, end_text,
            goals, together=False):
    """A chapter: where it starts (room, x, y, facing), the counter of its
    quest stages, the wonder component it gives, the restoration card, where
    the party goes after it (back), the end-of-chapter text, the notebook's
    goal per quest stage: [(stage, text), ...] (the last one reached shows),
    and whether both sisters travel in it (together; otherwise the one who set
    out travels alone)."""
    CHAPTERS.append(dict(number=number, title=title, place=place, reward=reward, start=start, quest=quest,
                         wonder=wonder, restore_title=restore_title, restore_text=restore_text, back=back,
                         end_text=end_text, goals=list(goals), together=together))


# ---- dialogue ------------------------------------------------------------------
def talk(name, who, text, next=None, cond=None, alt=None, actions=(), choices=()):
    """A conversation node. Long text becomes several pages: the actions run on
    the first, the choices are offered on the last. With a J in any argument it
    becomes NAME@maiya and NAME@luna (see Journeys)."""
    if has_j((who, text, next, cond, alt, list(actions), list(choices))):
        for j in JOURNEYS:
            talk(f"{name}@{j}", pick(who, j), pick(text, j), pick(next, j), pick(cond, j), pick(alt, j),
                 pick(list(actions), j), pick(list(choices), j))
        return name
    if name in NODES:
        raise SystemExit(f"world: dialogue {name} twice")
    pages = paginate(text)
    names = [name] + [f"{name}__{k}" for k in range(1, len(pages))]
    for k, page in enumerate(pages):
        last = k == len(pages) - 1
        NODES[names[k]] = dict(who=who, text=page, next=(next if last else names[k + 1]),
                               cond=cond if k == 0 else None, alt=alt if k == 0 else None,
                               actions=list(actions) if k == 0 else [], choices=list(choices) if last else [])
        NODE_ORDER.append(names[k])
    return name


def say(name, text, luna_text=None, **kw):
    """A line the travelling sister says herself: Maiya in her journey, Luna in
    hers (luna_text: Luna's words when they differ)."""
    return talk(name, J('MAIYA', 'LUNA'), J(text, luna_text if luna_text is not None else text), **kw)


def wrap(text, width, lines):
    out = textwrap.wrap(text.upper(), width)
    if len(out) > lines:
        raise SystemExit(f"world: {text!r} is longer than {lines} lines of {width}")
    return '\n'.join(out)


def paginate(text):
    text = text.upper()
    lines = []
    for para in text.split('\n'):
        lines += textwrap.wrap(para, TEXT_W) or ['']
    pages = [lines[i:i + TEXT_LINES] for i in range(0, len(lines), TEXT_LINES)]
    for p in pages:
        for ln in p:
            bad = [c for c in ln if not (32 <= ord(c) < 127) or c in '"\\']
            if bad:
                raise SystemExit(f"world: bad character {bad} in {ln!r}")
    return ['\n'.join(p) for p in pages]


# ---- drawing maps in code ------------------------------------------------------------
def canvas(edge='T', fill='.'):
    """A room's map to draw on: `edge` all round, `fill` inside."""
    return [[edge] * W] + [[edge] + [fill] * (W - 2) + [edge] for _ in range(H - 2)] + [[edge] * W]


def put(rows, x0, y0, x1, y1, ch):
    """Fill the rectangle (x0, y0)-(x1, y1), corners included."""
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            rows[y][x] = ch


def text(rows):
    return [''.join(r) for r in rows]


# ---- rooms and objects ---------------------------------------------------------------
class Obj(dict):
    pass


def room(name, title, rows, region=0, music='village', bg=None, restored=None, restored_music=None,
         exits=(), objects=(), triggers=(), flags=0, lit=1, theme=None):
    rows = [r for r in rows]
    if len(rows) != H or any(len(r) != W for r in rows):
        raise SystemExit(f"world: room {name} map must be {H} rows of {W}")
    for y, r in enumerate(rows):
        for x, c in enumerate(r):
            if c not in LEGEND:
                raise SystemExit(f"world: room {name} ({x},{y}): unknown map character {c!r}")
    if any(r['name'] == name for r in ROOMS):
        raise SystemExit(f"world: room {name} twice")
    ROOMS.append(dict(name=name, title=title, rows=rows, region=region, music=music, bg=bg or name,
                      restored=restored, restored_music=restored_music or music, exits=list(exits),
                      objects=list(objects), triggers=list(triggers), flags=flags, lit=lit,
                      theme=theme or REGION_THEME.get(region, 'valley')))


def add_object(room_name, obj):
    """An object placed in a room declared earlier (the finds spread over the valley, say)."""
    for r in ROOMS:
        if r['name'] == room_name:
            r['objects'].append(obj)
            return
    raise SystemExit(f"world: add_object: no room {room_name}")


def add_exit(room_name, ex):
    """A later chapter opens a way out of a room declared before it (the crossroads, say)."""
    for r in ROOMS:
        if r['name'] == room_name:
            r['exits'].append(ex)
            return
    raise SystemExit(f"world: add_exit: no room {room_name}")


def exit_(x, y, w, h, to, tx, ty, face, cond=None, locked=None):
    return dict(x=x, y=y, w=w, h=h, to=to, tx=tx, ty=ty, face=face, cond=cond, locked=locked, journey=None)


def _o(type_, x, y, name=None, w=1, h=1, p=(), flag_=None, text=None, inp=(), out=(), actions=(), look=None,
       hint=None, **extra):
    o = Obj(type=type_, x=x, y=y, w=w, h=h, p=list(p) + [0] * (7 - len(p)), flag=flag_, text=text,
            inp=list(inp), out=list(out), actions=list(actions), name=name, look=look, hint=hint, journey=None)
    o.update(extra)
    return o


def npc(x, y, who, text, name=None, wander=0, speed=8, dx=0, dy=0, look=None, hint=None, only=None):
    """only=('set', FLAG): there once FLAG is set; ('clear', FLAG): until it is."""
    when, fl = (0, None) if not only else ({'set': 1, 'clear': 2}[only[0]], only[1])
    return _o('NPC', x, y, name, p=[wander, speed, dx & 0xFF, dy & 0xFF, when], flag_=fl, text=text,
              look=look or who, hint=hint)


def sign(x, y, text, look='sign', name=None, hint=None):
    return _o('SIGN', x, y, name, text=text, look=look, hint=hint)


def pickup(x, y, item_=None, count=1, collect=None, flag_=None, look=None, name=None, hint=None):
    return _o('PICKUP', x, y, name, p=['ITEM:' + item_ if item_ else 0xFF, count, 'COLL:' + collect if collect else 0xFF],
              flag_=flag_, look=look or (item_ or collect), hint=hint)


FIND_LOOKS = {'flower': 'flower_red', 'history': 'coin', 'diagram': 'scroll', 'photo': 'photo', 'specimen': 'moss'}


def find(x, y, category, title, look=None):
    """A collectible lying about (a flower, a find, a photograph spot...): declared and placed at once."""
    key = f"FIND{len(COLLECT)}"
    collectible(key, category, title)
    return pickup(x, y, collect=key, flag_=key + '_TAKEN', look=look or FIND_LOOKS[category])


def chest(x, y, item_=None, count=1, collect=None, flag_=None, look='chest', name=None, actions=(), hint=None):
    return _o('CHEST', x, y, name, p=['ITEM:' + item_ if item_ else 0xFF, count, 'COLL:' + collect if collect else 0xFF],
              flag_=flag_, look=look, actions=actions, hint=hint)


def rest(x, y, text=None, look='well', name=None, hint=None):
    return _o('REST', x, y, name, text=text, look=look, hint=hint)


def secret(x, y, flag_, tool=None, solid=1, look='crack', name=None, actions=(), hint=None):
    return _o('SECRET', x, y, name, p=['ITEM:' + tool if tool else 0, solid], flag_=flag_, look=look, actions=actions, hint=hint)


def block(x, y, heavy=1, look='block', name=None, hint=None):
    return _o('BLOCK', x, y, name, p=[heavy], look=look, hint=hint)


def plate(x, y, out, weight=3, sticky=0, look='plate', name=None, actions=(), hint=None):
    return _o('PLATE', x, y, name, p=[weight, sticky], out=[out], look=look, actions=actions, hint=hint)


def lever(x, y, out, fine=0, look='lever', name=None, actions=(), hint=None, flag_=None):
    """A toggles it. With flag_, it stays where it was left when the room is entered again."""
    return _o('LEVER', x, y, name, p=[0, 0, 0, 0, 0, 0, fine], out=[out], look=look, actions=actions, hint=hint,
              flag_=flag_)


def gate(x, y, w=1, h=1, inp=(), invert=0, flag_=None, look='gate', name=None, actions=(), hint=None):
    return _o('GATE', x, y, name, w=w, h=h, p=[invert], flag_=flag_, inp=list(inp), look=look, actions=actions, hint=hint)


def lock(x, y, item_, used=1, flag_=None, look='none', name=None, actions=(), hint=None, text=None):
    """Solid until item_ is used on it (A); used=1 takes the item. text: what is said without it."""
    return _o('LOCK', x, y, name, p=['ITEM:' + item_, used], flag_=flag_, look=look, actions=actions, hint=hint,
              text=text)


DIRS = {'right': 0, 'down': 1, 'left': 2, 'up': 3}


def emitter(x, y, dir, inp=(), look='sunbeam', name=None, hint=None):
    """Light leaves it towards dir; with inp, only while those signals are on."""
    return _o('EMITTER', x, y, name, p=[DIRS[dir]], inp=list(inp), look=look, hint=hint)


def mirror(x, y, slant='/', look='mirror', name=None, hint=None):
    """A turns it between '/' and '\\'."""
    return _o('MIRROR', x, y, name, p=[0 if slant == '/' else 1], look=look, hint=hint)


def lens(x, y, look='lens', name=None, hint=None):
    return _o('LENS', x, y, name, look=look, hint=hint)


def receiver(x, y, out, focused=0, look='receiver', name=None, actions=(), hint=None):
    """Drives out while lit (focused=1: only light that went through a lens)."""
    return _o('RECEIVER', x, y, name, p=[focused], out=[out], look=look, actions=actions, hint=hint)


def pan(x, y, name, w=1, h=1, look='pan', hint=None):
    return _o('PAN', x, y, name, w=w, h=h, look=look, hint=hint)


def balance(name, left, right, out, threshold=1):
    """Drives out while the weight on pan `left` minus pan `right` is at least threshold."""
    return _o('BALANCE', 0, 0, name, p=['OBJ:' + left, 'OBJ:' + right, threshold], out=[out], look='none')


def platform(x, y, dx, dy, speed=8, wait=30, inp=(), look='basket', name=None, hint=None):
    """Moves dx, dy tiles and back (speed: sixteenths of a pixel a frame), waiting `wait` frames at each
    end; with inp, only while those signals are on."""
    return _o('PLATFORM', x, y, name, p=[dx & 0xFF, dy & 0xFF, speed, wait], inp=list(inp), look=look, hint=hint)


def bridge(x, y, w=1, h=1, inp=(), invert=0, flag_=None, look='planks', name=None, hint=None):
    """Floor over water or a drop while its inputs are on (invert=1: while they are off)."""
    return _o('BRIDGE', x, y, name, w=w, h=h, p=[invert], inp=list(inp), flag_=flag_, look=look, hint=hint)


def gear(x, y, name, role=0, turn=0, mesh=(), item_=None, inp=(), out=(), look='gear', hint=None, fine=0, flag_=None):
    """role 1 driver, 2 target, 0 an empty socket for item_ (with flag_, the gear stays for good)."""
    return _o('GEAR', x, y, name, p=[role, turn] + ['OBJ:' + m for m in mesh] + [0xFF] * (3 - len(mesh)) +
              ['ITEM:' + item_ if item_ else 0, fine], inp=list(inp), out=list(out), look=look, hint=hint, flag_=flag_)


def water_src(x, y, inp=(), look='spring', name=None):
    return _o('WATER_SRC', x, y, name, inp=list(inp), look=look)


def valve(x, y, open_=0, look='valve', name=None, hint=None):
    return _o('VALVE', x, y, name, p=[open_], look=look, hint=hint)


def outlet(x, y, out, look='outlet', name=None, actions=(), hint=None):
    return _o('OUTLET', x, y, name, out=[out], look=look, actions=actions, hint=hint)


def bell(x, y, tone, look='chime', name=None, hint=None):
    return _o('BELL', x, y, name, p=[tone], look=look, hint=hint)


def sequence(name, steps, out, flag_=None, actions=()):
    return _o('SEQUENCE', 0, 0, name, p=['OBJ:' + s for s in steps] + [0xFF] * (6 - len(steps)), out=[out],
              flag_=flag_, actions=actions, look='none')


def creature(x, y, kind, flag_, hits=2, damage=1, speed=10, radius=4, look='beetle', name=None, actions=(), hint=None):
    kinds = {'guard': 0, 'wander': 1, 'patrol': 2, 'chase': 3}
    return _o('CREATURE', x, y, name, p=[kinds[kind], hits, damage, speed, radius], flag_=flag_, look=look,
              actions=actions, hint=hint)


def hazard(x, y, period, on, damage=1, look='steam', name=None, hint=None):
    return _o('HAZARD', x, y, name, p=[period, on, damage], look=look, hint=hint)


def signal_flag(flag_, out, name=None):
    return _o('SIGNAL_FLAG', 0, 0, name, flag_=flag_, out=[out], look='none')


def trigger(when, x=0, y=0, w=1, h=1, once=None, cond=None, actions=()):
    kinds = {'enter': 0, 'step': 1, 'interact': 2, 'room': 3, 'watch': 4}
    return dict(when=kinds[when], x=x, y=y, w=w, h=h, once=once, cond=cond, actions=list(actions), journey=None)


# ---- writing C ------------------------------------------------------------------------
def room_id(name):
    for i, r in enumerate(ROOMS):
        if r['name'] == name:
            return i
    raise SystemExit(f"world: no room {name}")


def node_ref(name, j):
    """The node a reference to `name` reaches in journey j."""
    if name is None or '@' in name:
        return name
    v = f"{name}@{j}"
    return v if v in NODES else name


def node_id(name, j=None):
    if name is None:
        return 0xFFFF
    if j is not None:
        name = node_ref(name, j)
    if name not in NODES:
        raise SystemExit(f"world: no dialogue {name}" + (f" in {j}'s journey" if j else ""))
    return NODE_ORDER.index(name)


def _node_refs(n):
    return ([n['next'], n['alt']] + [c[1] for c in n['choices']] +
            [a[1] for a in n['actions'] if a[0] == 'talk'])


def _resolved(n, j):
    m = dict(n)
    m['next'], m['alt'] = node_ref(n['next'], j), node_ref(n['alt'], j)
    m['choices'] = [tuple([c[0], node_ref(c[1], j)] + list(c[2:])) for c in n['choices']]
    m['actions'] = [tuple([a[0], node_ref(a[1], j)] + list(a[2:])) if a[0] == 'talk' else a for a in n['actions']]
    return m


def finish_dialogue():
    """Split every shared node that leads to journey nodes into its own two,
    point each journey node at its own journey's nodes, and drop shared nodes
    that both journeys have their own of."""
    changed = True
    while changed:
        changed = False
        for name in list(NODE_ORDER):
            if '@' in name:
                continue
            n = NODES[name]
            if any(node_ref(r, 'maiya') != node_ref(r, 'luna') for r in _node_refs(n) if r):
                for j in JOURNEYS:
                    v = f"{name}@{j}"
                    if v not in NODES:
                        NODES[v] = _resolved(n, j)
                        NODE_ORDER.append(v)
                changed = True
            if all(f"{name}@{j}" in NODES for j in JOURNEYS):
                del NODES[name]
                NODE_ORDER.remove(name)
                changed = True
    for name in NODE_ORDER:
        if '@' in name:
            j = name.split('@')[1].split('__')[0]
            NODES[name] = _resolved(NODES[name], j)


def tiles_of(r):
    t, ch = [], []
    exit_tiles = set()
    for e in r['exits']:
        for yy in range(e['y'], e['y'] + e['h']):
            for xx in range(e['x'], e['x'] + e['w']):
                exit_tiles.add((xx, yy))
    for y, row in enumerate(r['rows']):
        for x, c in enumerate(row):
            _, f, wet = LEGEND[c]
            if (x, y) in exit_tiles:
                f |= DOOR
            t.append(f)
            ch.append(wet)
    return t, ch


class Writer:
    def __init__(self):
        self.out = []
        self.text = []
        self.arrays = {}

    def text_id(self, s):
        if s not in self.text:
            self.text.append(s)
        return self.text.index(s)

    def array(self, ctype, name, body):
        """static const ctype name[] = { body }; once per distinct body (the journeys share what they can)."""
        key = (ctype, body)
        if key not in self.arrays:
            self.arrays[key] = name
            self.out.append(f"static const {ctype} {name}[] = {{ {body} }};")
        return self.arrays[key]

    def cond(self, c, signals=None):
        if c is None:
            return "{ 0, 0, 0, 0 }"
        op = c[0]
        if op not in CONDS:
            raise SystemExit(f"world: condition {c}")
        a, b = 0, 0
        if op in ('flag', 'not'):
            a = flag(c[1])
        elif op == 'item':
            a, b = item_id(c[1]), (c[2] if len(c) > 2 else 1)
        elif op in ('ctr_ge', 'ctr_eq'):
            a, b = counter(c[1]), c[2]
        elif op == 'leader':
            a = {'MAIYA': 0, 'LUNA': 1}[c[1]]
        elif op == 'signal':
            if signals is None or c[1] not in signals:
                raise SystemExit(f"world: signal {c[1]} outside its room")
            a = signals.index(c[1])
        elif op == 'collected':
            a = collect_id(c[1])
        return f"{{ {CONDS[op]}, 0, {a}u, {b}u }}"

    def actions(self, name, acts, objnames=None, j=None):
        if not acts:
            return "0"
        rows = []
        for a in acts:
            op = a[0]
            if op not in ACTS:
                raise SystemExit(f"world: action {a}")
            x, y = 0, 0
            if op in ('set', 'clear'):
                x = flag(a[1])
            elif op in ('give', 'take'):
                x, y = item_id(a[1]), (a[2] if len(a) > 2 else 1)
            elif op in ('ctr', 'add'):
                x, y = counter(a[1]), a[2]
            elif op == 'talk':
                x = node_id(a[1], j)
            elif op == 'restore':
                x = a[1]
            elif op == 'heal':
                y = a[1] if len(a) > 1 else 0
            elif op == 'teleport':
                x, y = room_id(a[1]), a[3] * 256 + a[2]
            elif op == 'collect':
                x = collect_id(a[1])
            elif op == 'event':
                x, y = event(a[1]), (a[2] if len(a) > 2 else 0)
            elif op == 'open':
                if objnames is None or a[1] not in objnames:
                    raise SystemExit(f"world: open {a[1]}: no such object in the room")
                x = objnames.index(a[1])
            rows.append(f"{{ {ACTS[op]}, 0, {x}u, {y}u }}")
        rows.append("{ 0, 0, 0u, 0u }")
        return self.array("NGRpgAction", name, ", ".join(rows))

    def resolve_p(self, v, objnames):
        if isinstance(v, str):
            kind, _, ref = v.partition(':')
            if kind == 'ITEM':
                return item_id(ref)
            if kind == 'COLL':
                return collect_id(ref)
            if kind == 'OBJ':
                if ref not in objnames:
                    raise SystemExit(f"world: no object {ref} in the room")
                return objnames.index(ref)
            raise SystemExit(f"world: parameter {v}")
        return v & 0xFF

    def room_in(self, ri, r, j, tiles_name, ch_name):
        """Journey j's version of room r: its own objects, exits and triggers."""
        tag = f"ml_r{ri}{j[0]}"
        objects = [pick(ob, j) for ob in r['objects'] if in_journey(ob, j)]
        exits = [pick(e, j) for e in r['exits'] if in_journey(e, j)]
        triggers = [pick(t, j) for t in r['triggers'] if in_journey(t, j)]
        signals = []
        for ob in objects:
            for s_ in ob['inp'] + ob['out']:
                if s_ not in signals:
                    signals.append(s_)
        if len(signals) > 32:
            raise SystemExit(f"world: room {r['name']} has more than 32 signals in {j}'s journey")
        objnames = [ob['name'] for ob in objects]
        if len(objnames) > 32:
            raise SystemExit(f"world: room {r['name']} has more than 32 objects in {j}'s journey")
        named = [n for n in objnames if n]
        if len(named) != len(set(named)):
            raise SystemExit(f"world: room {r['name']}: two objects share a name in {j}'s journey")
        ex = []
        for e in exits:
            locked = node_id(e['locked'], j) if e['locked'] else 0xFFFF
            ex.append(f"{{ {e['x']}, {e['y']}, {e['w']}, {e['h']}, {room_id(e['to'])}, {e['tx']}, {e['ty']}, "
                      f"{FACES[e['face']]}, {self.cond(e['cond'], signals)}, {locked}u }}")
        exits_name = self.array("NGRpgExit", f"{tag}_exits", ", ".join(ex)) if ex else "0"
        obs, looks, hints = [], [], []
        for oi, ob in enumerate(objects):
            acts = self.actions(f"{tag}_o{oi}_a", ob['actions'], objnames, j)
            pv = [self.resolve_p(v, objnames) for v in ob['p']]
            inm = sum(1 << signals.index(s_) for s_ in ob['inp'])
            outm = sum(1 << signals.index(s_) for s_ in ob['out'])
            fl = flag(ob['flag']) if ob['flag'] else 0
            txt = node_id(ob['text'], j) if ob['text'] else 0xFFFF
            obs.append(f"{{ NGR_O_{ob['type']}, {ob['x']}, {ob['y']}, {ob['w']}, {ob['h']}, "
                       f"{{ {', '.join(str(v) for v in pv)} }}, {fl}u, {txt}u, 0x{inm:08X}u, 0x{outm:08X}u, {acts} }}")
            looks.append(f"ML_LOOK_{ob['look'].upper()}")
            hints.append(str(node_id(ob['hint'], j)) + 'u' if ob['hint'] else '0xFFFFu')
        objs_name = looks_name = hints_name = "0"
        if obs:
            objs_name = self.array("NGRpgObjectDef", f"{tag}_objs", "\n    " + ",\n    ".join(obs))
            looks_name = self.array("uint8_t", f"{tag}_looks", ", ".join(looks))
            hints_name = self.array("uint16_t", f"{tag}_hints", ", ".join(hints))
        trs = []
        for ti, tr in enumerate(triggers):
            acts = self.actions(f"{tag}_t{ti}_a", tr['actions'], objnames, j)
            once = flag(tr['once']) if tr['once'] else 0
            trs.append(f"{{ {tr['x']}, {tr['y']}, {tr['w']}, {tr['h']}, {tr['when']}, 0, {once}u, "
                       f"{self.cond(tr['cond'], signals)}, {acts} }}")
        trigs_name = self.array("NGRpgTrigger", f"{tag}_trigs", ", ".join(trs)) if trs else "0"
        room_c = (f"{{ {W}, {H}, {r['region']}, {r['flags']}, {tiles_name}, {ch_name}, {exits_name}, {objs_name}, "
                  f"{trigs_name}, {len(ex)}, {len(obs)}, {len(trs)}, 0, ML_MUS_{r['music'].upper()}, "
                  f"ML_BG_{r['bg'].upper()} }}")
        return room_c, looks_name, hints_name

    def write(self, path):
        finish_dialogue()
        o = self.out
        meta = []
        rooms = {j: [] for j in JOURNEYS}
        looks_all = {j: [] for j in JOURNEYS}
        hints_all = {j: [] for j in JOURNEYS}
        for ri, r in enumerate(ROOMS):
            tag = f"ml_r{ri}"
            t, ch = tiles_of(r)
            o.append(f"/* {r['name']}: {r['title']} */")
            tiles_name = self.array("uint8_t", f"{tag}_tiles", ", ".join(str(v) for v in t))
            ch_name = self.array("uint8_t", f"{tag}_ch", ", ".join(str(v) for v in ch)) if any(ch) else "0"
            for j in JOURNEYS:
                rc, lk, hn = self.room_in(ri, r, j, tiles_name, ch_name)
                rooms[j].append(rc)
                looks_all[j].append(lk)
                hints_all[j].append(hn)
            rest = f"ML_BG_{r['restored'].upper()}" if r['restored'] else f"ML_BG_{r['bg'].upper()}"
            meta.append(f"{{ {self.text_id(r['title'].upper())}u, {rest}, ML_MUS_{r['restored_music'].upper()}, {r['lit']} }}")

        def both(rows_of):
            return "{\n  { " + " },\n  { ".join(",\n    ".join(rows_of[j]) for j in JOURNEYS) + " } }"
        o.append("const NGRpgRoomDef ml_rooms[ML_JOURNEYS][ML_ROOMS] = " + both(rooms) + ";")
        o.append("const MLRoomMeta ml_room_meta[ML_ROOMS] = {\n    " + ",\n    ".join(meta) + " };")
        o.append("const uint8_t *const ml_room_looks[ML_JOURNEYS][ML_ROOMS] = " + both(looks_all) + ";")
        o.append("const uint16_t *const ml_room_hints[ML_JOURNEYS][ML_ROOMS] = " + both(hints_all) + ";")
        items = [f"{{ 0x{f:02X}, {st}, {pa}, {we} }}" for (_, f, st, pa, we, _, _) in ITEMS]
        o.append("const NGRpgItemDef ml_items[ML_ITEMS] = { " + ", ".join(items) + " };")
        it_text = {j: [f"{{ {self.text_id(pick(t, j).upper())}u, {self.text_id(wrap(pick(x, j), 30, 3))}u }}"
                       for (*_, t, x) in ITEMS] for j in JOURNEYS}
        o.append("const MLItemText ml_item_text[ML_JOURNEYS][ML_ITEMS] = " + both(it_text) + ";")
        o.append("const MLCollectible ml_collect[ML_COLLECT] = { " +
                 ", ".join(f"{{ ML_CAT_{c.upper()}, 0, {self.text_id(t.upper())}u }}" for (_, c, t) in COLLECT) + " };")
        nodes = []
        for name in NODE_ORDER:
            n = NODES[name]
            acts = self.actions(f"ml_d{len(nodes)}_a", n['actions'])
            ch = n['choices'] + [None] * (3 - len(n['choices']))
            ct = [str(self.text_id(c[0].upper())) + 'u' if c else '0u' for c in ch]
            cn = [str(node_id(c[1])) + 'u' if c else '0xFFFFu' for c in ch]
            cc = [self.cond(c[2] if c and len(c) > 2 else None) for c in ch]
            nodes.append(f"{{ {speaker_id(n['who'])}, {len(n['choices'])}, {self.text_id(n['text'])}u, {node_id(n['next'])}u, "
                         f"{node_id(n['alt']) if n['alt'] else 0xFFFF}u, {self.cond(n['cond'])}, {acts}, "
                         f"{{ {', '.join(ct)} }}, {{ {', '.join(cn)} }}, {{ {', '.join(cc)} }} }}")
        o.append("const NGRpgDialogueNode ml_dialogue[ML_NODES] = {\n    " + ",\n    ".join(nodes) + " };")
        o.append("const NGRpgWorld ml_worlds[ML_JOURNEYS] = {\n    " + ",\n    ".join(
            f"{{ ml_rooms[{k}], ml_items, 0, ml_dialogue, ML_ROOMS, ML_ITEMS, 0, 0, ML_NODES }}"
            for k in range(len(JOURNEYS))) + " };")
        o.append("const MLSpeaker ml_speakers[ML_SPEAKERS] = { " +
                 ", ".join(f"{{ {self.text_id(label.upper())}u, ML_FACE_{name} }}" for name, label in SPEAKERS) + " };")
        chs = {j: [] for j in JOURNEYS}
        goals = {j: [] for j in JOURNEYS}
        item_names = [it[0] for it in ITEMS]
        for j in JOURNEYS:
            for c0 in CHAPTERS:
                c = pick(c0, j)
                sr, sx, sy, sf = c['start']
                br, bx, by, bf = c['back']
                first = len(goals[j])
                for stage, text_ in c['goals']:
                    goals[j].append(f"{{ {stage}, 0, {self.text_id(wrap(text_, 26, 5))}u }}")
                chs[j].append(f"{{ {c['number']}, {counter(c['quest'])}, {self.text_id(c['title'].upper())}u, "
                              f"{self.text_id(c['place'].upper())}u, {self.text_id(c['reward'].upper())}u, "
                              f"{room_id(sr)}, {sx}, {sy}, {FACES[sf]}, {room_id(br)}, {bx}, {by}, {FACES[bf]}, "
                              f"{item_names.index(c['wonder'])}, {first}, {len(c['goals'])}, {1 if c['together'] else 0}, "
                              f"{self.text_id(c['restore_title'].upper())}u, {self.text_id(c['restore_text'].upper())}u, "
                              f"{self.text_id(c['end_text'].upper())}u }}")
        if len(goals['maiya']) != len(goals['luna']):
            raise SystemExit("world: both journeys need as many notebook goals per chapter")
        o.append("const MLChapter ml_chapters[ML_JOURNEYS][ML_CHAPTERS] = " + both(chs) + ";")
        o.append("const MLGoal ml_goals[ML_JOURNEYS][ML_GOALS] = " + both(goals) + ";")
        text_c = "const char *const ml_text[ML_TEXTS] = {\n" + ",\n".join(
            '    "' + s_.replace('\n', '\\n') + '"' for s_ in self.text) + " };"
        head = ["/* Generated by games/maiyaluna/tools/world.py from the world files (world/): do not edit. */",
                "#ifndef ML_WORLD_DATA_H", "#define ML_WORLD_DATA_H"]
        body = "\n".join(o)
        path.write_text("\n".join(head) + "\n" + body + "\n" + text_c + "\n#endif\n", encoding="ascii")

    def header(self, path):
        looks, bgs, musics = look_list(), bg_list(), music_list()
        lines = ["/* Generated by games/maiyaluna/tools/world.py from the world files (world/): do not edit. */",
                 "#ifndef ML_WORLD_IDS_H", "#define ML_WORLD_IDS_H",
                 "#define ML_JOURNEYS 2", "#define ML_J_MAIYA 0", "#define ML_J_LUNA 1",
                 f"#define ML_ROOMS {len(ROOMS)}", f"#define ML_ITEMS {len(ITEMS)}", f"#define ML_COLLECT {len(COLLECT)}",
                 f"#define ML_NODES {len(NODE_ORDER)}", f"#define ML_TEXTS {len(self.text)}",
                 f"#define ML_SPEAKERS {len(SPEAKERS)}", f"#define ML_CHAPTERS {len(CHAPTERS)}",
                 f"#define ML_GOALS {sum(len(c['goals']) for c in CHAPTERS)}",
                 f"#define ML_FLAG_COUNT {len(FLAGS)}"]
        for cat in ('flower', 'history', 'diagram', 'photo', 'specimen', 'wonder'):
            lines.append(f"#define ML_FINDS_{cat.upper()} {sum(1 for c in COLLECT if c[1] == cat)}")
        lines += [f"#define ML_ROOM_{r['name'].upper()} {i}" for i, r in enumerate(ROOMS)]
        lines += [f"#define ML_I_{it[0].upper()} {i}" for i, it in enumerate(ITEMS)]
        lines += [f"#define ML_C_{c[0].upper()} {i}" for i, c in enumerate(COLLECT)]
        lines += [f"#define ML_F_{f.upper()} {i}" for i, f in enumerate(FLAGS) if i]
        lines += [f"#define ML_CTR_{c.upper()} {i}" for i, c in enumerate(COUNTERS)]
        lines += [f"#define ML_EV_{e.upper()} {i + 1}" for i, e in enumerate(EVENTS)]
        lines += [f"#define ML_SPK_{s[0]} {i}" for i, s in enumerate(SPEAKERS)]
        named = [n for n in NODE_ORDER if '__' not in n]
        lines += [f"#define ML_DLG_{n.upper().replace('@', '_J_')} {NODE_ORDER.index(n)}" for n in named]
        lines += [f"#define ML_LOOK_{n.upper()} {i}" for i, n in enumerate(looks)]
        lines += [f"#define ML_LOOKS {len(looks)}"]
        lines += [f"#define ML_BG_{n.upper()} {i}" for i, n in enumerate(bgs)]
        lines += [f"#define ML_BGS {len(bgs)}"]
        lines += [f"#define ML_MUS_{n.upper()} {i}" for i, n in enumerate(musics)]
        lines += [f"#define ML_MUSIC {len(musics)}"]
        lines += [f"#define ML_FACE_{s[0]} {i}" for i, s in enumerate(SPEAKERS)]
        lines += ["#endif", ""]
        path.write_text("\n".join(lines), encoding="ascii")


# Looks: what the game draws for each object, in first-use order ('none' first).
# Speakers and the sisters come first so a person's look is their speaker number.
def look_list():
    out = ['none']
    for name, _ in SPEAKERS:
        if name.lower() not in out:
            out.append(name.lower())
    for r in ROOMS:
        for ob in r['objects']:
            lk = ob['look'].lower()
            if lk not in out:
                out.append(lk)
    return out


def bg_list():
    out = []
    for r in ROOMS:
        for b in (r['bg'], r['restored']):
            if b and b not in out:
                out.append(b)
    return out


MUSIC_FIXED = ['title', 'restored', 'chapter', 'danger', 'secret', 'ending']


def music_list():
    out = list(MUSIC_FIXED)
    for r in ROOMS:
        for m in (r['music'], r['restored_music']):
            if m not in out:
                out.append(m)
    return out
