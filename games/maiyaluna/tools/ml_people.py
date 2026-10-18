"""
Maiya & Luna people: small top-down figures drawn pixel by pixel.

Every figure is built from the same skeleton (head, body, arms, legs) in a
32 x 32 frame with the feet at (16, 29), so the cast keeps one scale and one
anchor and no frame changes proportions. Each person differs in palette,
hair, headwear and clothes. Directions: down (facing the player), left, up
(the back); right is the left frame mirrored by the hardware. Frames per
direction: stand, step (left foot forward), step (right foot forward), act
(arms out: talk, strike, use). A sheet is 4 directions x 4 frames, left to right:
down 0-3, left 4-7, up 8-11, and a 'cheer' frame at 12.

Shading is three steps per material, lit from the upper left, with a
one-pixel dark outline around the whole figure.
"""
import numpy as np

OUTLINE = (34, 24, 30)


def shade(c, k):
    return tuple(max(0, min(255, int(v * k))) for v in c)


class Canvas:
    def __init__(self, w=32, h=32):
        self.w, self.h = w, h
        self.px = np.zeros((h, w, 4), np.uint8)

    def put(self, x, y, c):
        if 0 <= x < self.w and 0 <= y < self.h:
            self.px[y, x, :3] = c
            self.px[y, x, 3] = 255

    def rect(self, x0, y0, x1, y1, c):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                self.put(x, y, c)

    def ellipse(self, cx, cy, rx, ry, c, light=None, dark=None):
        """Filled ellipse; light on its upper-left edge, dark on its lower-right."""
        for y in range(int(cy - ry) - 1, int(cy + ry) + 2):
            for x in range(int(cx - rx) - 1, int(cx + rx) + 2):
                dx, dy = (x + 0.5 - cx) / rx, (y + 0.5 - cy) / ry
                d = dx * dx + dy * dy
                if d <= 1.0:
                    col = c
                    if dark and (dx + dy) > 0.75 and d > 0.45:
                        col = dark
                    elif light and (dx + dy) < -0.8 and d > 0.3:
                        col = light
                    self.put(x, y, col)

    def outline(self, color=OUTLINE):
        a = self.px[:, :, 3] > 0
        grow = a.copy()
        grow[1:, :] |= a[:-1, :]
        grow[:-1, :] |= a[1:, :]
        grow[:, 1:] |= a[:, :-1]
        grow[:, :-1] |= a[:, 1:]
        edge = grow & ~a
        self.px[edge, :3] = color
        self.px[edge, 3] = 255

    def mirror(self):
        c = Canvas(self.w, self.h)
        c.px = self.px[:, ::-1].copy()
        return c


class Look(dict):
    """skin, hair, top, bottom, boots and the extras of one person."""


def tone3(c):
    return (shade(c, 1.22), c, shade(c, 0.72))


def figure(look, direction, frame):
    """direction 'down' | 'left' | 'up'; frame 0 stand, 1 step L, 2 step R, 3 act, 4 cheer."""
    cv = Canvas()
    skin_l, skin, skin_d = tone3(look['skin'])
    hair_l, hair, hair_d = tone3(look['hair'])
    top_l, top, top_d = tone3(look['top'])
    bot_l, bot, bot_d = tone3(look['bottom'])
    boot_l, boot, boot_d = tone3(look['boots'])
    short = look.get('short', 0)            # a child: one pixel shorter body and legs
    feet = 29
    leg_top = 23 + short
    # ---- legs and boots ------------------------------------------------------------
    step = {0: (0, 0), 1: (1, -1), 2: (-1, 1), 3: (0, 0), 4: (0, 0)}[frame]
    if direction in ('down', 'up'):
        for side, (lx, off) in enumerate(((12, step[0]), (17, step[1]))):
            if look.get('robe'):
                continue
            y1 = feet - 2 + off
            cv.rect(lx, leg_top, lx + 2, y1 - 1, bot if not look.get('bare_legs') else skin)
            cv.rect(lx + 2, leg_top, lx + 2, y1 - 1, bot_d if not look.get('bare_legs') else skin_d)
            cv.rect(lx - (1 if side == 0 else 0), y1, lx + 2 + (1 if side == 1 else 0), y1 + 2, boot)
            cv.put(lx + (0 if side == 0 else 1), y1, boot_l)
            cv.rect(lx - (1 if side == 0 else 0), y1 + 2, lx + 2 + (1 if side == 1 else 0), y1 + 2, boot_d)
    else:   # profile: legs apart on a step
        spread = {0: 0, 1: 3, 2: -3, 3: 0, 4: 0}[frame]
        for lx in (14 - (spread if spread > 0 else 0), 16 + (abs(spread) if spread < 0 else spread)):
            if look.get('robe'):
                continue
            cv.rect(lx, leg_top, lx + 2, feet - 2, bot if not look.get('bare_legs') else skin)
            cv.rect(lx - 2, feet - 1, lx + 2, feet + 1, boot)
            cv.rect(lx - 2, feet + 1, lx + 2, feet + 1, boot_d)
    if look.get('robe'):                         # a long dress or robe to the boots
        sway = {0: 0, 1: 1, 2: -1, 3: 0, 4: 0}[frame]
        for y in range(leg_top - 1, feet):
            half = 5 + (y - leg_top) // 3
            for x in range(16 - half + sway, 16 + half + sway):
                cv.put(x, y, bot_d if x >= 16 + half + sway - 2 else (bot_l if x <= 16 - half + sway + 1 else bot))
        cv.rect(12 + sway, feet, 14 + sway, feet + 1, boot)
        cv.rect(17 + sway, feet, 19 + sway, feet + 1, boot)
    # ---- body -----------------------------------------------------------------------
    by0, by1 = 16 + short, leg_top
    cv.ellipse(16, (by0 + by1) / 2 + 0.5, 5.5, (by1 - by0) / 2 + 1.5, top, top_l, top_d)
    if look.get('skirt'):                    # a short tunic skirt flaring at the hips
        sk = look['skirt']
        for y in range(by1 - 1, by1 + 3):
            half = 6 + (y - by1 + 1) // 2
            for x in range(16 - half, 16 + half):
                cv.put(x, y, shade(sk, 0.8) if (x + y) % 5 == 0 else sk)
    if look.get('belt'):
        cv.rect(11, by1 - 2, 21, by1 - 2, look['belt'])
        cv.put(16, by1 - 2, (230, 200, 90))
    if look.get('apron') and direction != 'up':
        cv.rect(13, by0 + 2, 19, by1 + 2, look['apron'])
        cv.rect(19, by0 + 2, 19, by1 + 2, shade(look['apron'], 0.75))
    if look.get('collar') and direction != 'up':
        cv.rect(13, by0, 19, by0 + 1, look['collar'])
    if look.get('strap'):
        for k in range(8):
            cv.put(12 + k, by0 + k // 1 if direction != 'left' else by0 + k, look['strap'])
    if look.get('emblem') and direction == 'down':
        cv.put(16, by0 + 3, look['emblem']); cv.put(15, by0 + 3, look['emblem']); cv.put(16, by0 + 2, look['emblem'])
    if look.get('buttons') and direction == 'down':
        for k in range(3):
            cv.put(16, by0 + 2 + 2 * k, look['buttons'])
    # ---- arms -----------------------------------------------------------------------
    sleeve = look.get('sleeve', top)
    swing = {0: 0, 1: -1, 2: 1, 3: 0, 4: 0}[frame]
    if frame == 4:        # cheer: both arms up
        for ax in (9, 22):
            cv.rect(ax, by0 - 6, ax + 1, by0 + 1, sleeve)
            cv.rect(ax, by0 - 8, ax + 1, by0 - 7, skin)
    elif frame == 3:      # act: arms reach forward
        if direction == 'down':
            cv.rect(10, by0 + 1, 11, by0 + 5, sleeve); cv.rect(21, by0 + 1, 22, by0 + 5, sleeve)
            cv.rect(11, by0 + 6, 12, by0 + 7, skin); cv.rect(20, by0 + 6, 21, by0 + 7, skin)
        elif direction == 'left':
            cv.rect(7, by0 + 2, 13, by0 + 3, sleeve); cv.rect(5, by0 + 2, 6, by0 + 3, skin)
        else:
            cv.rect(10, by0 - 2, 11, by0 + 3, sleeve); cv.rect(21, by0 - 2, 22, by0 + 3, sleeve)
    else:
        if direction in ('down', 'up'):
            cv.rect(10, by0 + 1 + swing, 11, by0 + 5 + swing, sleeve)
            cv.rect(10, by0 + 6 + swing, 11, by0 + 7 + swing, skin)
            cv.rect(21, by0 + 1 - swing, 22, by0 + 5 - swing, shade(sleeve, 0.8))
            cv.rect(21, by0 + 6 - swing, 22, by0 + 7 - swing, skin_d)
        else:
            ax = 15 + swing * 2
            cv.rect(ax, by0 + 1, ax + 2, by0 + 5, shade(sleeve, 0.85))
            cv.rect(ax, by0 + 6, ax + 2, by0 + 7, skin)
    if look.get('cane') and direction != 'up':
        cv.rect(22, by0 + 4, 22, feet, (120, 80, 40))
        cv.rect(21, by0 + 4, 23, by0 + 4, (120, 80, 40))
    if look.get('satchel'):
        sx = 20 if direction != 'left' else 17
        if direction != 'up' or True:
            cv.rect(sx, by1 - 4, sx + 3, by1, look['satchel'])
            cv.rect(sx, by1 - 4, sx + 3, by1 - 4, shade(look['satchel'], 1.2))
    # ---- head ------------------------------------------------------------------------
    hy = 10 + short
    if look.get('bun') and direction != 'down':
        cv.ellipse(16 if direction == 'up' else 19, hy - 5, 3, 2.5, hair, hair_l, hair_d)
    tail = look.get('ponytail')
    if tail and direction == 'up':
        cv.rect(15, hy + 3, 17, hy + 10, hair)
        cv.rect(17, hy + 3, 17, hy + 10, hair_d)
        cv.rect(16, hy + 11, 16, hy + 11, hair_d)
    if tail and direction == 'left':
        cv.ellipse(22, hy + 3, 2, 4, hair, hair_l, hair_d)
        cv.rect(22, hy + 6, 23, hy + 8, hair_d)
    if tail and direction == 'down':
        cv.rect(22, hy + 1, 23, hy + 6, hair_d)
    if look.get('longhair'):
        if direction == 'up':
            cv.rect(10, hy, 22, hy + 9, hair)
            cv.rect(20, hy, 22, hy + 9, hair_d)
        elif direction == 'down':
            cv.rect(9, hy + 1, 11, hy + 9, hair_d)
            cv.rect(21, hy + 1, 23, hy + 9, hair_d)
        else:
            cv.rect(17, hy, 22, hy + 9, hair)
            cv.rect(21, hy, 22, hy + 9, hair_d)
    cv.ellipse(16, hy, 6.5, 6, skin, skin_l, skin_d)
    # hair cap
    if direction == 'up':
        cv.ellipse(16, hy - 0.5, 6.8, 6.2, hair, hair_l, hair_d)
    elif direction == 'down':
        for y in range(hy - 6, hy - 1 + look.get('fringe', 1)):
            for x in range(9, 24):
                dx, dy = (x + 0.5 - 16) / 6.8, (y + 0.5 - hy) / 6.2
                if dx * dx + dy * dy <= 1:
                    cv.put(x, y, hair_l if (x < 14 and y < hy - 3) else (hair_d if x > 19 else hair))
        cv.rect(9, hy - 2, 10, hy + 2, hair_d)
        cv.rect(22, hy - 2, 23, hy + 2, hair_d)
        if look.get('spiky'):
            for x in (11, 14, 17, 20):
                cv.put(x, hy - 7, hair)
                cv.put(x + 1, hy - 8, hair_l)
    else:   # left profile: hair on the back and top
        for y in range(hy - 6, hy + 4):
            for x in range(10, 24):
                dx, dy = (x + 0.5 - 16) / 6.8, (y + 0.5 - hy) / 6.2
                if dx * dx + dy * dy <= 1 and (x >= 15 or y < hy - 2):
                    cv.put(x, y, hair_l if (y < hy - 3 and x < 18) else (hair_d if x > 20 else hair))
        if look.get('spiky'):
            for x in (12, 15, 18):
                cv.put(x, hy - 7, hair)
    # face
    eye = look.get('eyes', (40, 60, 120))
    if direction == 'down':
        cv.put(13, hy + 1, (30, 20, 30)); cv.put(13, hy + 2, eye)
        cv.put(19, hy + 1, (30, 20, 30)); cv.put(19, hy + 2, eye)
        cv.put(12, hy + 3, shade(skin, 0.9)); cv.put(20, hy + 3, shade(skin, 0.9))
        cv.put(16, hy + 4, (180, 90, 80))
        if look.get('glasses'):
            for ex in (12, 18):
                cv.rect(ex, hy + 1, ex + 2, hy + 1, (60, 60, 70))
            cv.put(15, hy + 1, (60, 60, 70)); cv.put(16, hy + 1, (60, 60, 70)); cv.put(17, hy + 1, (60, 60, 70))
        if look.get('beard'):
            cv.ellipse(16, hy + 4.5, 4.5, 2.5, look['beard'], shade(look['beard'], 1.15), shade(look['beard'], 0.8))
            cv.put(16, hy + 3, (160, 90, 80))
    elif direction == 'left':
        cv.put(11, hy + 1, (30, 20, 30)); cv.put(11, hy + 2, eye)
        cv.put(9, hy + 2, skin)                     # nose
        cv.put(11, hy + 4, (180, 90, 80))
        if look.get('glasses'):
            cv.rect(10, hy + 1, 13, hy + 1, (60, 60, 70))
        if look.get('beard'):
            cv.ellipse(12, hy + 4.5, 3, 2.5, look['beard'])
    # headwear
    hat = look.get('cap')
    if hat:
        if direction == 'left':
            cv.ellipse(17, hy - 4, 6.5, 3, hat, shade(hat, 1.2), shade(hat, 0.75))
            cv.rect(9, hy - 3, 12, hy - 2, shade(hat, 0.85))
        else:
            cv.ellipse(16, hy - 4, 7, 3, hat, shade(hat, 1.2), shade(hat, 0.75))
            if direction == 'down':
                cv.rect(10, hy - 2, 22, hy - 2, shade(hat, 0.7))
        if look.get('badge') and direction == 'down':
            cv.put(16, hy - 5, look['badge'])
    if look.get('strawhat'):
        h = look['strawhat']
        cv.ellipse(16, hy - 3, 9, 3.5, h, shade(h, 1.15), shade(h, 0.75))
        cv.ellipse(16, hy - 5, 5, 2.5, shade(h, 0.95), shade(h, 1.15), shade(h, 0.8))
        cv.rect(11, hy - 4, 21, hy - 4, (200, 60, 60))
    if look.get('band') and direction != 'up':
        if direction == 'down':
            cv.rect(10, hy - 3, 22, hy - 3, look['band'])
        else:
            cv.rect(12, hy - 3, 21, hy - 3, look['band'])
    if look.get('goggles'):
        g = look['goggles']
        if direction == 'down':
            cv.rect(10, hy - 4, 22, hy - 4, (110, 70, 30))
            for gx in (12, 18):
                cv.rect(gx, hy - 5, gx + 2, hy - 3, g); cv.put(gx + 1, hy - 4, (90, 200, 160))
        elif direction == 'left':
            cv.rect(11, hy - 5, 13, hy - 3, g); cv.put(12, hy - 4, (90, 200, 160))
            cv.rect(14, hy - 4, 22, hy - 4, (110, 70, 30))
        else:
            cv.rect(10, hy - 4, 22, hy - 4, (110, 70, 30))
    if look.get('flower'):
        fx = {'down': 21, 'left': 18, 'up': 11}[direction]
        cv.put(fx, hy - 4, look['flower']); cv.put(fx + 1, hy - 4, look['flower'])
        cv.put(fx, hy - 3, look['flower']); cv.put(fx + 1, hy - 3, shade(look['flower'], 0.7))
        cv.put(fx - 1, hy - 4, shade(look['flower'], 1.2)); cv.put(fx, hy - 5, shade(look['flower'], 1.2))
        cv.put(fx + 1, hy - 4 + 0, (250, 220, 80))
    if look.get('leaf'):
        lx = {'down': 9, 'left': 19, 'up': 21}[direction]
        cv.put(lx, hy - 4, look['leaf']); cv.put(lx + 1, hy - 5, look['leaf']); cv.put(lx + 1, hy - 4, shade(look['leaf'], 0.7))
    cv.outline()
    return cv


def sheet(look):
    """down 0-3, left 4-7, up 8-11, cheer 12 (13 frames of 32 x 32, left to right)."""
    frames = []
    for d in ('down', 'left', 'up'):
        for f in range(4):
            frames.append(figure(look, d, f).px)
    frames.append(figure(look, 'down', 4).px)
    return np.concatenate(frames, axis=1)


CAST = {
    'maiya': Look(skin=(246, 196, 156), hair=(246, 206, 82), top=(72, 168, 72), bottom=(246, 196, 156),
                  boots=(142, 86, 42), bare_legs=1, ponytail=1, flower=(222, 40, 52), band=(40, 140, 64),
                  belt=(118, 70, 32), skirt=(206, 70, 96), eyes=(40, 130, 90)),
    'luna': Look(skin=(236, 186, 150), hair=(96, 58, 36), top=(62, 100, 204), bottom=(40, 52, 110),
                 boots=(132, 82, 44), longhair=1, band=(170, 200, 255), collar=(206, 62, 142),
                 satchel=(150, 98, 52), belt=(110, 66, 34), eyes=(96, 60, 40)),
    'sunboy': Look(skin=(244, 196, 150), hair=(246, 170, 42), top=(250, 206, 60), bottom=(120, 84, 50),
                   boots=(120, 74, 38), spiky=1, goggles=(196, 156, 70), short=1, emblem=(230, 120, 30),
                   strap=(110, 70, 34), eyes=(40, 130, 90)),
    'nari': Look(skin=(232, 190, 160), hair=(226, 226, 236), top=(132, 62, 132), bottom=(232, 220, 196),
                 boots=(110, 74, 50), robe=1, bun=1, glasses=1, cane=1, eyes=(90, 60, 40)),
    'tovan': Look(skin=(214, 158, 118), hair=(170, 170, 170), top=(226, 216, 196), bottom=(70, 62, 60),
                  boots=(96, 62, 36), cap=(122, 82, 52), beard=(196, 196, 196), apron=(140, 96, 56)),
    'mina': Look(skin=(244, 200, 166), hair=(212, 100, 50), top=(122, 132, 62), bottom=(196, 176, 126),
                 boots=(120, 84, 48), leaf=(80, 170, 70), collar=(240, 240, 230), eyes=(60, 140, 70)),
    'orin': Look(skin=(222, 170, 130), hair=(70, 54, 44), top=(42, 62, 124), bottom=(38, 44, 70),
                 boots=(60, 44, 34), cap=(36, 52, 106), badge=(230, 196, 80), beard=(72, 56, 46),
                 buttons=(230, 196, 80)),
    'valen': Look(skin=(230, 190, 160), hair=(62, 62, 72), top=(74, 78, 90), bottom=(56, 58, 68),
                  boots=(40, 40, 46), glasses=1, collar=(110, 114, 126), emblem=(200, 170, 90)),
    'hana': Look(skin=(240, 194, 156), hair=(110, 70, 40), top=(200, 70, 60), bottom=(70, 100, 170),
                 boots=(110, 72, 40), strawhat=(226, 196, 112)),
}


def fox_sheet():
    """Piko: a small red fox, 4 frames per direction (down, left, up) like the people."""
    frames = []
    orange, light, dark, white, black = (230, 118, 40), (250, 160, 80), (170, 76, 26), (246, 240, 228), (40, 30, 30)
    for d in ('down', 'left', 'up'):
        for f in range(4):
            cv = Canvas()
            step = (f % 2)
            if d == 'left':
                cv.ellipse(17, 23, 7, 4, orange, light, dark)                       # body
                cv.ellipse(25, 20, 4, 2.5, orange, light, dark)                     # tail
                cv.ellipse(28, 19, 1.5, 1.5, white)
                cv.ellipse(10, 18, 4, 3.5, orange, light, dark)                     # head
                cv.rect(6, 19, 7, 20, white); cv.put(5, 19, black)
                cv.put(9, 13, orange); cv.put(9, 14, orange); cv.put(12, 13, dark); cv.put(12, 14, orange)
                cv.put(9, 17, black)
                for lx in (12 + step, 21 - step):
                    cv.rect(lx, 26, lx, 29, black)
            else:
                cv.ellipse(16, 23, 5, 5, orange, light, dark)
                if d == 'down':
                    cv.ellipse(16, 25, 2.5, 3, white)
                cv.ellipse(16, 16, 5, 4, orange, light, dark)
                for ex in (12, 20):
                    cv.put(ex, 11, orange); cv.put(ex, 12, orange); cv.put(ex, 10, dark)
                if d == 'down':
                    cv.rect(14, 17, 18, 19, white); cv.put(16, 19, black)
                    cv.put(14, 15, black); cv.put(18, 15, black)
                else:
                    cv.ellipse(16, 27, 3, 2, orange, light, dark); cv.put(16, 29, white)
                for lx in (13, 19):
                    cv.rect(lx, 27 + (step if lx == 13 else 1 - step), lx, 29, black)
            cv.outline()
            frames.append(cv.px)
    frames.append(frames[0])
    return np.concatenate(frames, axis=1)
