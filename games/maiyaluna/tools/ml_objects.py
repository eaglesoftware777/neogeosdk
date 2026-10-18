"""
Maiya & Luna objects: the things in a room that change (pickups, switches,
gates, gears, valves, chimes, the creatures), as 16 x 16 frames.

OBJECT_FRAMES says, for each look, how its frames are laid out: the game
picks the frame from the object's state (see ml_draw.c, draw_object()).
"""
import math

import numpy as np

from ml_people import Canvas, shade

OUT = (34, 24, 30)
WOOD = [(176, 124, 72), (146, 100, 56), (108, 70, 38)]
STONE = [(196, 190, 178), (160, 154, 144), (112, 106, 100)]
BRASS = [(240, 206, 110), (204, 160, 70), (140, 100, 40)]
IRON = [(150, 156, 166), (110, 116, 126), (70, 74, 84)]


def c16():
    return Canvas(16, 16)


def done(cv, outline=True):
    if outline:
        cv.outline(OUT)
    return cv.px


def signpost():
    cv = c16()
    cv.rect(7, 8, 8, 15, WOOD[1])
    cv.rect(2, 2, 13, 8, WOOD[0])
    cv.rect(2, 8, 13, 8, WOOD[2])
    for x in range(4, 12, 2):
        cv.put(x, 5, WOOD[2])
    return done(cv)


def apple():
    cv = c16()
    cv.ellipse(8, 9.5, 5, 4.5, (220, 50, 50), (255, 140, 120), (150, 24, 30))
    cv.rect(8, 3, 8, 5, (100, 70, 40))
    cv.put(9, 4, (80, 170, 60)); cv.put(10, 4, (80, 170, 60)); cv.put(10, 3, (120, 200, 80))
    return done(cv)


def flower(petal, center=(250, 220, 80)):
    cv = c16()
    cv.rect(8, 9, 8, 15, (60, 140, 50))
    cv.put(9, 12, (90, 180, 70)); cv.put(10, 11, (90, 180, 70)); cv.put(6, 13, (90, 180, 70))
    for a in range(5):
        x = 8 + int(round(3 * math.cos(a * 2 * math.pi / 5 - 1.57)))
        y = 6 + int(round(3 * math.sin(a * 2 * math.pi / 5 - 1.57)))
        cv.ellipse(x + 0.5, y + 0.5, 1.8, 1.8, petal, shade(petal, 1.2), shade(petal, 0.75))
    cv.ellipse(8.5, 6.5, 1.3, 1.3, center)
    return done(cv)


def sparkle(px):
    """A second frame with a glint, for things worth picking up."""
    out = px.copy()
    for (x, y) in ((12, 2), (13, 3), (12, 4), (11, 3)):
        out[y, x] = (255, 255, 230, 255)
    out[3, 12] = (255, 255, 255, 255)
    return out


def photo():
    cv = c16()
    for x in (4, 11):
        cv.rect(x, 9, x, 15, WOOD[2])
    cv.rect(7, 9, 8, 15, WOOD[1])
    cv.rect(3, 3, 12, 9, (60, 60, 70))
    cv.ellipse(7.5, 6, 2.2, 2.2, (120, 180, 230), (200, 230, 255), (60, 100, 160))
    cv.rect(10, 4, 11, 4, (230, 200, 90))
    return done(cv)


def scroll():
    cv = c16()
    cv.rect(3, 5, 12, 11, (236, 222, 180))
    cv.rect(3, 5, 12, 5, (200, 180, 140))
    for y in (7, 9):
        cv.rect(5, y, 10, y, (120, 100, 80))
    cv.ellipse(3, 8, 1.5, 3.5, (210, 190, 150)); cv.ellipse(12.5, 8, 1.5, 3.5, (210, 190, 150))
    return done(cv)


def note():
    cv = c16()
    cv.rect(3, 3, 12, 13, (238, 232, 210))
    for y in (6, 8, 10):
        cv.rect(5, y, 11, y, (130, 120, 110))
    cv.ellipse(8, 3, 1.5, 1.5, (200, 60, 60))
    return done(cv)


def badge():
    cv = c16()
    cv.ellipse(8, 8, 5.5, 5.5, BRASS[1], BRASS[0], BRASS[2])
    cv.ellipse(8, 8, 3, 3, (70, 120, 90))
    cv.put(8, 7, BRASS[0]); cv.put(7, 8, BRASS[0]); cv.put(9, 8, BRASS[0]); cv.put(8, 9, BRASS[0])
    cv.rect(6, 13, 10, 14, (180, 50, 60))
    return done(cv)


def moss():
    cv = c16()
    for k, (x, y) in enumerate(((5, 10), (9, 9), (11, 11), (7, 12), (4, 12))):
        cv.ellipse(x, y, 2.6, 2.0, (120, 170, 60), (220, 240, 120), (70, 110, 40))
    for (x, y) in ((6, 9), (10, 8), (8, 11)):
        cv.put(x, y, (250, 255, 170))
    return done(cv)


def chest(opened):
    cv = c16()
    cv.rect(2, 7, 13, 14, WOOD[1])
    cv.rect(2, 14, 13, 14, WOOD[2])
    if opened:
        cv.rect(2, 3, 13, 6, WOOD[2])
        cv.rect(3, 6, 12, 8, (40, 30, 26))
    else:
        cv.rect(2, 4, 13, 7, WOOD[0])
        cv.rect(7, 6, 8, 9, BRASS[1])
    for x in (2, 13):
        cv.rect(x, 4 if not opened else 7, x, 14, BRASS[2])
    return done(cv)


def block():
    cv = c16()
    cv.rect(1, 2, 14, 14, STONE[1])
    cv.rect(1, 2, 14, 4, STONE[0])
    cv.rect(13, 4, 14, 14, STONE[2])
    cv.rect(1, 14, 14, 14, STONE[2])
    for (x, y) in ((4, 7), (9, 10), (6, 11)):
        cv.put(x, y, STONE[2])
    return done(cv)


def plate(down):
    cv = c16()
    cv.rect(1, 1, 14, 14, (90, 84, 78))
    if down:
        cv.rect(3, 4, 12, 13, STONE[2])
        cv.rect(3, 4, 12, 4, (60, 56, 52))
    else:
        cv.rect(3, 2, 12, 11, STONE[0])
        cv.rect(3, 11, 12, 12, STONE[2])
    return done(cv, outline=False)


def valve(opened):
    cv = c16()
    cv.ellipse(8, 8, 6, 6, IRON[1], IRON[0], IRON[2])
    cv.ellipse(8, 8, 3.5, 3.5, (60, 64, 70))
    cv.ellipse(8, 8, 1.5, 1.5, IRON[0])
    if opened:
        cv.rect(7, 1, 8, 15, (200, 60, 50))
    else:
        cv.rect(1, 7, 15, 8, (200, 60, 50))
    return done(cv)


def boards(opened, color=WOOD):
    cv = c16()
    cv.rect(0, 0, 2, 15, STONE[1]); cv.rect(13, 0, 15, 15, STONE[1])
    if opened:
        cv.rect(3, 0, 12, 2, color[1])
        cv.rect(3, 2, 12, 2, color[2])
    else:
        for y in range(1, 15, 4):
            cv.rect(3, y, 12, y + 2, color[0])
            cv.rect(3, y + 3, 12, y + 3, color[2])
    return done(cv)


def woodgate(opened):
    cv = c16()
    cv.rect(0, 0, 1, 15, WOOD[2]); cv.rect(14, 0, 15, 15, WOOD[2])
    if not opened:
        for x in range(3, 14, 3):
            cv.rect(x, 0, x + 1, 15, WOOD[0])
            cv.put(x + 1, 0, WOOD[2])
        cv.rect(2, 4, 13, 5, WOOD[1]); cv.rect(2, 11, 13, 12, WOOD[1])
    return done(cv)


def gear_px(cv, cx, cy, r, angle, col=BRASS):
    teeth = 8
    for y in range(int(cy - r - 2), int(cy + r + 3)):
        for x in range(int(cx - r - 2), int(cx + r + 3)):
            dx, dy = x + 0.5 - cx, y + 0.5 - cy
            d = math.hypot(dx, dy)
            a = math.atan2(dy, dx) + angle
            tooth = (math.cos(a * teeth) > 0.2)
            if d <= r - 1 or (d <= r + 1.2 and tooth):
                c = col[1]
                if dx + dy < -2:
                    c = col[0]
                elif dx + dy > 3:
                    c = col[2]
                if d < 2.2:
                    c = (60, 50, 40)
                elif abs(d - (r - 3)) < 0.6 and r > 5:
                    c = col[2]
                cv.put(x, y, c)


def gear_frames(empty_first):
    frames = []
    if empty_first:
        cv = c16()
        cv.rect(1, 1, 14, 14, (70, 62, 56))
        cv.rect(2, 2, 13, 13, (40, 34, 30))
        cv.ellipse(8, 8, 1.6, 1.6, IRON[1])
        frames.append(done(cv))
    for k in range(4):
        cv = c16()
        cv.rect(1, 1, 14, 14, (70, 62, 56))
        gear_px(cv, 8, 8, 6, k * math.pi / 16)
        frames.append(done(cv))
    return frames


def tablet():
    cv = c16()
    cv.rect(3, 1, 12, 15, STONE[1])
    cv.rect(3, 1, 12, 2, STONE[0])
    cv.rect(12, 2, 12, 15, STONE[2])
    for k, y in enumerate((4, 7, 10)):
        cv.rect(5 + k, y, 7 + k, y + 1, (90, 84, 78))
    return done(cv)


def chime(tone, ringing):
    cv = c16()
    cv.rect(1, 0, 14, 1, WOOD[1])
    length = (13, 7, 10)[tone] if False else (13, 10, 7)[tone]
    col = [(200, 170, 90), (180, 190, 200), (220, 140, 90)][tone]
    off = 1 if ringing else 0
    cv.rect(7 + off, 2, 8 + off, 2 + length, col)
    cv.put(7 + off, 2 + length, shade(col, 0.7)); cv.put(8 + off, 2, shade(col, 1.2))
    if ringing:
        for (x, y) in ((4, 5), (12, 6), (3, 9), (13, 10)):
            cv.put(x, y, (255, 255, 230))
    return done(cv)


def stonedoor(half):
    cv = c16()
    cv.rect(0, 0, 15, 15, STONE[1])
    for y in (5, 10):
        cv.rect(0, y, 15, y, STONE[2])
    cv.rect(0 if half else 15, 0, 0 if half else 15, 15, (60, 56, 52))
    if half == 0:
        cv.ellipse(13, 8, 2, 2, BRASS[1])
    return done(cv)


def beetle(frame):
    cv = c16()
    stunned = frame == 2
    leg = (1 if frame == 1 else 0)
    for (x, y) in ((3, 6), (3, 10), (12, 6), (12, 10)):
        cv.rect(x, y + leg * (1 if x < 8 else -1), x + 1, y + leg * (1 if x < 8 else -1), (50, 30, 26))
    cv.ellipse(8, 9, 4.5, 5, (172, 72, 40), (220, 120, 70), (110, 40, 24))
    cv.rect(8, 5, 8, 13, (90, 36, 24))
    cv.ellipse(8, 4, 2.5, 2, (60, 40, 34))
    if stunned:
        for (x, y) in ((3, 1), (6, 0), (10, 1), (13, 2)):
            cv.put(x, y, (255, 240, 120))
    else:
        cv.put(7, 3, (250, 250, 200)); cv.put(9, 3, (250, 250, 200))
    return done(cv)


def crack(opened):
    cv = c16()
    if opened:
        cv.rect(2, 2, 13, 15, (24, 18, 20))
        cv.rect(2, 2, 13, 3, (70, 60, 64))
    else:
        cv.rect(0, 0, 15, 15, (98, 78, 92))
        pts = [(8, 1), (7, 4), (9, 7), (6, 10), (8, 13), (7, 15)]
        for (x0, y0), (x1, y1) in zip(pts, pts[1:]):
            for t in range(6):
                cv.put(int(x0 + (x1 - x0) * t / 6), int(y0 + (y1 - y0) * t / 6), (40, 30, 36))
        cv.put(10, 6, (40, 30, 36)); cv.put(11, 5, (40, 30, 36))
    return done(cv, outline=False)


def water(frame):
    cv = c16()
    for y in range(16):
        for x in range(16):
            w = math.sin((x + frame * 3) * 0.8 + y * 0.35)
            cv.put(x, y, (120, 190, 240) if w > 0.9 else ((72, 146, 222) if w > -0.2 else (56, 120, 200)))
    return cv.px


def splash(frame):
    cv = c16()
    for k in range(6):
        x = 3 + (k * 5 + frame * 3) % 10
        y = 4 + (k * 7 + frame * 5) % 9
        cv.put(x, y, (230, 244, 252)); cv.put(x + 1, y, (170, 210, 240))
    return cv.px


def bandage():
    cv = c16()
    cv.ellipse(8, 9, 5, 4, (240, 236, 224), (255, 255, 250), (200, 190, 176))
    cv.ellipse(8, 9, 2, 1.5, (210, 200, 186))
    cv.rect(5, 4, 11, 5, (220, 70, 70))
    return done(cv)


def lens():
    cv = c16()
    cv.ellipse(6.5, 6.5, 4.5, 4.5, BRASS[1])
    cv.ellipse(6.5, 6.5, 3, 3, (170, 220, 250), (240, 250, 255), (110, 170, 220))
    for k in range(5):
        cv.put(10 + k, 10 + k, WOOD[2]); cv.put(11 + k, 10 + k, WOOD[1])
    return done(cv)


def notebook():
    cv = c16()
    cv.rect(3, 2, 12, 14, (60, 120, 90))
    cv.rect(4, 3, 12, 13, (80, 150, 110))
    cv.rect(3, 2, 4, 14, (40, 80, 60))
    cv.rect(7, 6, 10, 7, (230, 220, 180))
    return done(cv)


def cog():
    cv = c16()
    gear_px(cv, 8, 8, 6, 0.2, BRASS)
    return done(cv)


def wheelmech():
    cv = c16()
    gear_px(cv, 6, 9, 4.5, 0.1, BRASS)
    gear_px(cv, 11, 5, 3.5, 0.5, IRON)
    return done(cv)


def heart_icon():
    cv = c16()
    cv.ellipse(5.5, 6, 3.2, 3, (220, 50, 70)); cv.ellipse(10.5, 6, 3.2, 3, (220, 50, 70))
    for y in range(7, 14):
        half = 8 - (y - 7)
        cv.rect(8 - half, y, 7 + half, y, (220, 50, 70))
    cv.put(5, 5, (255, 170, 180))
    return done(cv)



# ---- the later chapters' things ------------------------------------------------------------
CU = [(210, 136, 80), (170, 100, 56), (110, 62, 36)]
SKY = (250, 236, 170)


def blank():
    return c16().px


def plank():
    cv = c16()
    cv.rect(1, 6, 14, 10, WOOD[0])
    cv.rect(1, 9, 14, 10, WOOD[2])
    cv.put(4, 7, WOOD[2]); cv.put(10, 8, WOOD[2]); cv.put(12, 7, WOOD[1])
    return done(cv)


def rope():
    cv = c16()
    cv.ellipse(8, 9, 6, 4.5, (196, 168, 110), (230, 206, 150), (140, 112, 70))
    cv.ellipse(8, 9, 3.5, 2.2, (120, 96, 60))
    cv.ellipse(8, 9, 2, 1.2, (196, 168, 110))
    cv.rect(13, 4, 14, 8, (196, 168, 110))
    return done(cv)


def key():
    cv = c16()
    cv.ellipse(5, 6, 3.5, 3.5, BRASS[1], BRASS[0], BRASS[2])
    cv.ellipse(5, 6, 1.4, 1.4, (60, 50, 40))
    cv.rect(8, 5, 14, 7, BRASS[1])
    cv.rect(11, 8, 11, 10, BRASS[2]); cv.rect(13, 8, 13, 11, BRASS[2])
    return done(cv)


def shell():
    cv = c16()
    cv.ellipse(8, 9, 5.5, 4.5, (250, 210, 190), (255, 240, 230), (200, 140, 130))
    for k in range(-2, 3):
        for t in range(5):
            cv.put(8 + k * (t // 2 + 1) // 2 + k, 13 - t, (210, 150, 140))
    cv.rect(6, 13, 10, 14, (230, 180, 160))
    return done(cv)


def feather():
    cv = c16()
    for t in range(12):
        x, y = 3 + t, 13 - t
        cv.put(x, y, (240, 240, 250))
        for w in range(1, 3):
            cv.put(x - w, y - w + 1, (120, 160, 220)); cv.put(x + w - 1, y + w, (90, 130, 200))
    return done(cv)


def fossil():
    cv = c16()
    cv.ellipse(8, 8, 6, 6, STONE[1], STONE[0], STONE[2])
    for a in range(30):
        r = 0.6 + a * 0.15
        ang = a * 0.55
        cv.put(int(8 + r * math.cos(ang)), int(8 + r * math.sin(ang)), (110, 96, 80))
    return done(cv)


def crystal_item():
    cv = c16()
    for y in range(3, 14):
        half = min(3, (y - 2) // 2)
        for x in range(8 - half, 9 + half):
            cv.put(x, y, (220, 210, 255) if x < 8 else ((170, 140, 230) if x == 8 else (120, 96, 200)))
    return done(cv)


def acorn():
    cv = c16()
    cv.ellipse(8, 10, 3.5, 4, (170, 110, 50), (210, 150, 80), (120, 74, 30))
    cv.ellipse(8, 6.5, 4.5, 2.2, (110, 80, 50), (150, 110, 70), (80, 56, 34))
    cv.rect(8, 3, 8, 4, (90, 60, 30))
    return done(cv)


def coin():
    cv = c16()
    cv.ellipse(8, 8, 5, 5, BRASS[1], BRASS[0], BRASS[2])
    cv.ellipse(8, 8, 3, 3, BRASS[0])
    cv.rect(8, 6, 8, 10, BRASS[2])
    return done(cv)


def shard():
    cv = c16()
    pts = [(3, 6), (12, 4), (13, 11), (6, 13)]
    for y in range(16):
        for x in range(16):
            inside = all(((bx - ax) * (y - ay) - (by - ay) * (x - ax)) >= 0 for (ax, ay), (bx, by) in zip(pts, pts[1:] + pts[:1]))
            if inside:
                cv.put(x, y, (190, 100, 60) if (x + y) % 5 else (230, 210, 170))
    return done(cv)


def pipe_item():
    cv = c16()
    cv.rect(2, 6, 13, 9, CU[1])
    cv.rect(2, 6, 13, 6, CU[0])
    cv.rect(1, 5, 2, 10, CU[2]); cv.rect(13, 5, 14, 10, CU[2])
    return done(cv)


def oilcan():
    cv = c16()
    cv.rect(4, 7, 11, 14, (90, 140, 90))
    cv.rect(4, 7, 5, 14, (130, 180, 120))
    cv.rect(11, 5, 14, 6, IRON[1])
    cv.rect(6, 4, 9, 6, IRON[0])
    return done(cv)


def compass():
    cv = c16()
    cv.ellipse(8, 8, 6, 6, BRASS[1], BRASS[0], BRASS[2])
    cv.ellipse(8, 8, 4.5, 4.5, (240, 236, 220))
    cv.rect(8, 4, 8, 8, (210, 50, 40)); cv.rect(8, 9, 8, 12, (60, 60, 70))
    return done(cv)


def crank():
    cv = c16()
    cv.rect(2, 7, 11, 8, IRON[1])
    cv.rect(10, 3, 11, 8, IRON[1])
    cv.rect(9, 2, 13, 4, WOOD[0])
    cv.ellipse(3, 7.5, 2, 2, IRON[2])
    return done(cv)


def sapling():
    cv = c16()
    cv.rect(7, 8, 8, 14, (110, 76, 44))
    cv.ellipse(8, 6, 4, 3.5, (80, 170, 70), (130, 210, 100), (50, 120, 50))
    cv.rect(5, 13, 10, 14, (120, 90, 60))
    return done(cv)


def chest_crate(opened):
    cv = c16()
    cv.rect(1, 4, 14, 14, WOOD[1])
    for y in (4, 9, 14):
        cv.rect(1, y, 14, y, WOOD[2])
    cv.rect(1, 4, 2, 14, WOOD[0]); cv.rect(13, 4, 14, 14, WOOD[2])
    if opened:
        cv.rect(2, 4, 13, 6, (40, 30, 24))
    return done(cv)


def rocks(cleared):
    cv = c16()
    if cleared:
        for (x, y) in ((3, 12), (11, 13), (7, 14)):
            cv.ellipse(x, y, 1.6, 1.2, STONE[1], STONE[0], STONE[2])
        return done(cv, outline=False)
    cv.ellipse(5, 10, 4.5, 4, (150, 140, 128), (186, 176, 160), (100, 92, 84))
    cv.ellipse(11, 9, 4.5, 5, (140, 130, 118), (176, 168, 150), (96, 88, 80))
    cv.ellipse(8, 5, 3.5, 3, (160, 150, 136), (196, 188, 170), (110, 100, 90))
    cv.rect(2, 3, 3, 4, (90, 140, 60))
    return done(cv)


def hedgegate(opened):
    cv = c16()
    if opened:
        cv.rect(0, 0, 1, 15, (36, 98, 44)); cv.rect(14, 0, 15, 15, (36, 98, 44))
        return done(cv, outline=False)
    cv.rect(0, 0, 15, 15, (48, 120, 52))
    for k in range(20):
        x, y = (k * 7) % 15, (k * 11) % 15
        cv.put(x, y, (78, 154, 66))
    cv.rect(2, 3, 13, 3, WOOD[1]); cv.rect(2, 12, 13, 12, WOOD[1])
    cv.rect(3, 3, 3, 12, WOOD[2]); cv.rect(12, 3, 12, 12, WOOD[2])
    return done(cv, outline=False)


def planks_bridge(built):
    cv = c16()
    if built:
        for x in range(16):
            cv.rect(x, 1, x, 14, WOOD[0] if x % 4 else WOOD[2])
        cv.rect(0, 0, 15, 0, (200, 180, 130)); cv.rect(0, 15, 15, 15, (200, 180, 130))
        return cv.px
    cv.rect(0, 0, 3, 0, (200, 180, 130)); cv.rect(0, 15, 3, 15, (200, 180, 130))
    cv.rect(0, 1, 2, 3, WOOD[1]); cv.rect(0, 12, 2, 14, WOOD[1])
    return cv.px


def sandbar(dry):
    cv = c16()
    if not dry:
        return cv.px
    cv.rect(0, 0, 15, 15, (226, 204, 146))
    for k in range(10):
        cv.put((k * 5) % 16, (k * 7) % 16, (200, 176, 120))
    return cv.px


def basket():
    cv = c16()
    cv.rect(1, 3, 14, 13, WOOD[1])
    for y in range(4, 13, 3):
        cv.rect(1, y, 14, y, WOOD[2])
    cv.rect(1, 3, 14, 3, WOOD[0])
    cv.rect(7, 0, 8, 2, (200, 180, 130))
    return done(cv)


def winch(roped):
    cv = c16()
    cv.rect(2, 3, 3, 14, WOOD[2]); cv.rect(12, 3, 13, 14, WOOD[2])
    cv.ellipse(8, 8, 4, 4, WOOD[1], WOOD[0], WOOD[2])
    if roped:
        for y in range(5, 12, 2):
            cv.rect(5, y, 11, y, (210, 186, 130))
    else:
        cv.rect(12, 10, 15, 11, (200, 180, 130))
    return done(cv)


def lever(on):
    cv = c16()
    cv.rect(3, 11, 12, 14, STONE[1]); cv.rect(3, 11, 12, 11, STONE[0])
    if on:
        for t in range(8):
            cv.put(8 + t // 2, 11 - t, IRON[1])
        cv.ellipse(12, 3.5, 2, 2, (220, 60, 50))
    else:
        for t in range(8):
            cv.put(7 - t // 2, 11 - t, IRON[1])
        cv.ellipse(3.5, 3.5, 2, 2, (220, 60, 50))
    return done(cv)


def mirror_px(slant):
    cv = c16()
    cv.rect(6, 13, 9, 15, WOOD[2])
    for t in range(14):
        x = 1 + t if slant == 0 else 14 - t
        y = 14 - t
        for w in (-1, 0, 1):
            col = (220, 240, 255) if w == 0 else ((150, 190, 230) if w < 0 else BRASS[1])
            cv.put(x + w, y, col)
    return done(cv)


def sunbeam():
    cv = c16()
    cv.ellipse(8, 8, 5, 5, (255, 220, 110), (255, 250, 210), (230, 170, 60))
    for a in range(8):
        cv.put(int(8 + 7 * math.cos(a * math.pi / 4)), int(8 + 7 * math.sin(a * math.pi / 4)), SKY)
    return done(cv)


def lens_stand():
    cv = c16()
    cv.rect(7, 11, 8, 15, BRASS[2])
    cv.ellipse(8, 7, 5, 6, BRASS[1])
    cv.ellipse(8, 7, 3.5, 4.5, (190, 230, 255), (250, 255, 255), (120, 180, 230))
    return done(cv)


def receiver_px(lit):
    cv = c16()
    cv.rect(6, 11, 9, 15, STONE[2])
    cv.ellipse(8, 7, 5.5, 5.5, BRASS[1], BRASS[0], BRASS[2])
    cv.ellipse(8, 7, 3, 3, (255, 250, 200) if lit else (90, 70, 40))
    if lit:
        for (x, y) in ((1, 1), (14, 1), (1, 12), (14, 12)):
            cv.put(x, y, SKY)
    return done(cv)


def shutter(opened):
    cv = c16()
    cv.rect(0, 0, 1, 15, WOOD[2]); cv.rect(14, 0, 15, 15, WOOD[2])
    if not opened:
        for y in range(0, 16, 3):
            cv.rect(2, y, 13, y + 1, WOOD[1]); cv.rect(2, y + 2, 13, y + 2, WOOD[2])
    return done(cv, outline=False)


def hatch(opened):
    cv = c16()
    if opened:
        cv.rect(1, 1, 14, 14, (20, 16, 18)); cv.rect(1, 1, 14, 2, (60, 50, 44))
    else:
        cv.rect(1, 1, 14, 14, WOOD[1])
        for x in range(1, 15, 4):
            cv.rect(x, 1, x, 14, WOOD[2])
        cv.ellipse(8, 8, 2, 2, BRASS[1])
    return done(cv)


def pan_px():
    cv = c16()
    cv.ellipse(8, 9, 7, 4.5, IRON[1], IRON[0], IRON[2])
    cv.ellipse(8, 8.5, 5.5, 3, (90, 96, 106))
    for x in (2, 13):
        cv.rect(x, 0, x, 6, (200, 180, 130))
    return done(cv)


def steam(on):
    cv = c16()
    cv.rect(2, 11, 13, 15, IRON[2])
    for x in range(3, 13, 3):
        cv.rect(x, 12, x + 1, 14, (30, 30, 36))
    if on:
        for k in range(14):
            cv.ellipse(4 + (k * 5) % 9, 9 - (k * 3) % 9, 2.2, 2, (236, 240, 244), (255, 255, 255), (190, 196, 204))
    return done(cv, outline=not on)


def haze(clear):
    cv = c16()
    if clear:
        return cv.px
    for y in range(16):
        for x in range(16):
            if (x * 3 + y * 5) % 4 == 0 or math.sin(x * 0.7 + y * 0.4) > 0.6:
                cv.put(x, y, (150, 160, 110) if (x + y) % 3 else (120, 130, 90))
    return cv.px


def fan(k):
    cv = c16()
    cv.rect(0, 0, 15, 15, IRON[2])
    cv.ellipse(8, 8, 7, 7, (40, 40, 48))
    for b in range(4):
        a = b * math.pi / 2 + k * math.pi / 8
        for r in range(1, 7):
            for w in (-1, 0, 1):
                cv.put(int(8 + r * math.cos(a) - w * math.sin(a)), int(8 + r * math.sin(a) + w * math.cos(a)), WOOD[0] if w else WOOD[1])
    cv.ellipse(8, 8, 1.5, 1.5, BRASS[1])
    return done(cv, outline=False)


def stalactite(tone, ringing):
    cv = c16()
    length = (14, 11, 8)[tone]
    col = [(170, 150, 220), (130, 180, 220), (220, 170, 230)][tone]
    off = 1 if ringing else 0
    for y in range(length):
        half = max(0, 3 - y * 3 // length)
        cv.rect(8 - half + off, y, 8 + half + off, y, col if y % 3 else shade(col, 1.2))
    if ringing:
        for (x, y) in ((3, 4), (13, 5), (2, 9), (14, 10)):
            cv.put(x, y, (255, 255, 230))
    return done(cv)


def dial(sym, ringing):
    cv = c16()
    cv.ellipse(8, 8, 7, 7, STONE[1], STONE[0], STONE[2])
    cv.ellipse(8, 8, 5, 5, (230, 220, 196) if ringing else (206, 198, 180))
    if sym == 'seed':
        cv.ellipse(8, 9, 2, 2.5, (140, 100, 50)); cv.rect(8, 4, 8, 7, (70, 160, 60)); cv.put(9, 5, (70, 160, 60))
    elif sym == 'sun':
        cv.ellipse(8, 8, 2.5, 2.5, (250, 200, 60))
        for a in range(8):
            cv.put(int(8 + 4 * math.cos(a * math.pi / 4)), int(8 + 4 * math.sin(a * math.pi / 4)), (240, 160, 40))
    elif sym == 'leaf':
        cv.ellipse(8, 8, 2.2, 3.5, (220, 120, 40), (250, 170, 70), (160, 70, 30)); cv.rect(8, 11, 8, 12, (110, 70, 40))
    else:
        for d in range(-3, 4):
            cv.put(8 + d, 8, (120, 170, 230)); cv.put(8, 8 + d, (120, 170, 230)); cv.put(8 + d, 8 + d, (160, 200, 240))
    return done(cv)


def flood(drained):
    cv = c16()
    if drained:
        return cv.px
    for y in range(16):
        for x in range(16):
            w = math.sin(x * 0.7 + y * 0.45)
            cv.put(x, y, (110, 170, 220) if w > 0.8 else ((60, 110, 170) if w > -0.3 else (40, 80, 140)))
    return cv.px


def facet(slant):
    """A crystal face on the cave floor, set '/' or '\\' (a mirror of quartz)."""
    cv = c16()
    cv.ellipse(8, 13, 6, 2.5, (70, 60, 90))
    for t in range(13):
        x = 2 + t if slant == 0 else 13 - t
        y = 13 - t
        for w in (-1, 0, 1, 2):
            col = (236, 226, 255) if w == 0 else ((170, 140, 230) if w < 0 else (120, 96, 200))
            cv.put(x + w, y, col)
    return done(cv)


def socket(filled):
    cv = c16()
    cv.rect(1, 1, 14, 14, STONE[2])
    cv.rect(2, 2, 13, 13, (60, 56, 52))
    cv.rect(2, 2, 13, 2, STONE[1])
    if filled:
        gear_px(cv, 8, 8, 4.5, 0.3, BRASS)
        cv.put(3, 3, SKY); cv.put(12, 12, SKY)
    return done(cv)


def cradle(filled):
    cv = c16()
    cv.rect(6, 12, 9, 15, BRASS[2])
    cv.ellipse(8, 7, 6, 6, BRASS[1], BRASS[0], BRASS[2])
    cv.ellipse(8, 7, 4, 4, (40, 34, 30))
    if filled:
        cv.ellipse(8, 7, 4, 4, (190, 230, 255), (250, 255, 255), (120, 180, 230))
        cv.put(6, 5, (255, 255, 255))
    return done(cv)


def telescope():
    cv = c16()
    for t in range(11):
        for w in range(-1, 2):
            cv.put(3 + t, 11 - t + w, BRASS[1] if w else BRASS[0])
    cv.rect(6, 11, 7, 15, WOOD[2]); cv.rect(9, 11, 10, 15, WOOD[2])
    cv.ellipse(13.5, 1.5, 1.5, 1.5, (190, 230, 255))
    return done(cv)


def sundial():
    cv = c16()
    cv.ellipse(8, 9, 7, 5, STONE[1], STONE[0], STONE[2])
    for a in range(12):
        cv.put(int(8 + 5.5 * math.cos(a * math.pi / 6)), int(9 + 4 * math.sin(a * math.pi / 6)), (90, 84, 78))
    for t in range(5):
        cv.put(8 + t // 2, 9 - t, BRASS[1])
    return done(cv)


def markstone(lit):
    cv = c16()
    cv.rect(4, 2, 11, 15, STONE[1]); cv.rect(4, 2, 11, 3, STONE[0]); cv.rect(10, 3, 11, 15, STONE[2])
    cv.ellipse(7.5, 8, 2, 2, (255, 240, 160) if lit else (90, 84, 78))
    if lit:
        cv.put(2, 2, SKY); cv.put(13, 4, SKY)
    return done(cv)


def cart():
    cv = c16()
    cv.rect(1, 4, 14, 11, IRON[1]); cv.rect(1, 4, 14, 5, IRON[0])
    cv.ellipse(4, 13, 2, 2, (40, 40, 46)); cv.ellipse(11, 13, 2, 2, (40, 40, 46))
    cv.rect(3, 6, 12, 8, (120, 110, 100))
    return done(cv)


def barrel():
    cv = c16()
    cv.ellipse(8, 8, 6, 7, WOOD[1], WOOD[0], WOOD[2])
    cv.rect(2, 4, 13, 4, IRON[2]); cv.rect(2, 11, 13, 11, IRON[2])
    return done(cv)


def critter(kind, frame):
    cv = c16()
    stunned = frame == 2
    step = frame == 1
    if kind == 'crab':
        cv.ellipse(8, 10, 5, 3.5, (220, 90, 60), (250, 140, 100), (160, 50, 30))
        for x in (2, 13):
            cv.ellipse(x, 6 + (1 if step else 0), 2, 1.6, (230, 100, 70))
        for x in (4, 6, 10, 12):
            cv.put(x, 14 - (1 if step and x < 8 else 0), (160, 50, 30))
        cv.put(6, 7, (20, 20, 20)); cv.put(10, 7, (20, 20, 20))
    elif kind == 'wasp':
        cv.ellipse(9, 9, 4, 2.5, (240, 200, 50), (255, 230, 120), (180, 140, 30))
        for x in (7, 10):
            cv.rect(x, 7, x, 11, (40, 30, 20))
        cv.ellipse(4, 8.5, 2, 2, (50, 40, 30))
        wy = 4 if step else 5
        cv.ellipse(8, wy, 2.5, 1.5, (220, 230, 250)); cv.ellipse(11, wy, 2.5, 1.5, (200, 214, 240))
    elif kind == 'bat':
        cv.ellipse(8, 8, 2.5, 3, (90, 70, 100), (130, 110, 140), (60, 44, 70))
        for side in (-1, 1):
            for t in range(5):
                cv.put(8 + side * (3 + t), 7 + (t // 2 if step else -t // 2), (80, 60, 90))
        cv.put(7, 7, (250, 230, 120)); cv.put(9, 7, (250, 230, 120))
    elif kind == 'robot':
        cv.rect(4, 4, 11, 12, IRON[1]); cv.rect(4, 4, 11, 5, IRON[0])
        cv.rect(5, 6, 10, 7, (240, 80, 60) if not stunned else (60, 60, 70))
        cv.rect(4, 13, 5, 14 if step else 15, IRON[2]); cv.rect(10, 13, 11, 15 if step else 14, IRON[2])
        cv.rect(7, 1, 8, 3, IRON[2])
    else:   # squirrel
        cv.ellipse(7, 10, 3.5, 3, (190, 110, 50), (230, 150, 80), (130, 70, 30))
        cv.ellipse(12, 6, 3, 4, (200, 120, 60), (240, 170, 100), (140, 80, 40))
        cv.ellipse(4, 7, 2, 2, (190, 110, 50)); cv.put(3, 6, (20, 20, 20))
        cv.put(6, 13 + (1 if step else 0), (130, 70, 30))
    if stunned:
        for (x, y) in ((3, 1), (6, 0), (10, 1), (13, 2)):
            cv.put(x, y, (255, 240, 120))
    return done(cv)


def beam(mask):
    """Light crossing a tile: arms towards bits 1 up, 2 right, 4 down, 8 left."""
    cv = c16()
    core, glow = (255, 252, 220), (250, 210, 100)
    arms = [(1, (0, -1)), (2, (1, 0)), (4, (0, 1)), (8, (-1, 0))]
    for bit, (dx, dy) in arms:
        if not mask & bit:
            continue
        for t in range(9):
            x, y = 7 + dx * t, 7 + dy * t
            for w in range(-1, 3):
                xx, yy = (x + w, y) if dy else (x, y + w)
                cv.put(xx, yy, core if w in (0, 1) else glow)
    cv.ellipse(8, 8, 2.6, 2.6, core)
    return cv.px

def sheet(frames):
    return np.concatenate(frames, axis=1)


# look -> (frames, kind). kind: 'one' (one frame, glint as a second),
# 'state' (frame = state), 'gear' (frame 0 empty, 1-4 turning),
# 'tone' (frame = tone * 2 + ringing), 'half' (left/right halves, open hides),
# 'creature' (walk 0-1, stunned 2)
def object_sheets():
    o = {}
    o['signpost'] = ([signpost()], 'one')
    o['apple'] = ([apple(), sparkle(apple())], 'glint')
    o['flower_blue'] = ([flower((140, 170, 250)), sparkle(flower((140, 170, 250)))], 'glint')
    o['flower_gold'] = ([flower((250, 200, 60), (230, 120, 40)), sparkle(flower((250, 200, 60), (230, 120, 40)))], 'glint')
    o['photo'] = ([photo(), sparkle(photo())], 'glint')
    o['scroll'] = ([scroll(), sparkle(scroll())], 'glint')
    o['note'] = ([note()], 'one')
    o['badge'] = ([badge(), sparkle(badge())], 'glint')
    o['moss'] = ([moss(), sparkle(moss())], 'glint')
    o['chest'] = ([chest(False), chest(True)], 'state')
    o['block'] = ([block()], 'one')
    o['plate'] = ([plate(False), plate(True)], 'state')
    o['valve'] = ([valve(False), valve(True)], 'state')
    o['floodgate'] = ([boards(False), boards(True)], 'state')
    o['sluicegate'] = ([boards(False, IRON), boards(True, IRON)], 'state')
    o['woodgate'] = ([woodgate(False), woodgate(True)], 'state')
    o['gear'] = (gear_frames(True), 'gear')
    o['gear_big'] = (gear_frames(False), 'turning')
    o['tablet'] = ([tablet()], 'one')
    o['chime'] = ([chime(t, r) for t in range(3) for r in (0, 1)], 'tone')
    o['stonedoor'] = ([stonedoor(0), stonedoor(1)], 'half')
    o['beetle'] = ([beetle(0), beetle(1), beetle(2)], 'creature')
    o['crack'] = ([crack(False), crack(True)], 'state')
    for name, fn in (('plank', plank), ('rope', rope), ('key', key), ('shell', shell), ('feather', feather),
                     ('fossil', fossil), ('crystal', crystal_item), ('acorn', acorn), ('coin', coin),
                     ('shard', shard), ('oilcan', oilcan), ('compass', compass), ('crank', crank),
                     ('sapling', sapling), ('lensitem', lens), ('cogitem', cog), ('pipeitem', pipe_item)):
        o[name] = ([fn(), sparkle(fn())], 'glint')
    for name, petal, centre in (('flower_red', (230, 60, 60), (250, 220, 80)),
                                ('flower_white', (246, 246, 240), (250, 200, 60)),
                                ('flower_violet', (170, 110, 230), (250, 230, 120)),
                                ('flower_orange', (250, 150, 50), (140, 70, 30))):
        o[name] = ([flower(petal, centre), sparkle(flower(petal, centre))], 'glint')
    o['crate'] = ([chest_crate(False), chest_crate(True)], 'state')
    o['rocks'] = ([rocks(False), rocks(True)], 'state')
    o['hedgegate'] = ([hedgegate(False), hedgegate(True)], 'state')
    o['planks'] = ([planks_bridge(False), planks_bridge(True)], 'state')
    o['sandbar'] = ([sandbar(False), sandbar(True)], 'state')
    o['basket'] = ([basket()], 'mover')
    o['winch'] = ([winch(False), winch(True)], 'state')
    o['lever'] = ([lever(False), lever(True)], 'state')
    o['mirror'] = ([mirror_px(0), mirror_px(1)], 'state')
    o['sunbeam'] = ([sunbeam()], 'one')
    o['lens'] = ([lens_stand()], 'one')
    o['receiver'] = ([receiver_px(False), receiver_px(True)], 'state')
    o['markstone'] = ([markstone(False), markstone(True)], 'state')
    o['shutter'] = ([shutter(False), shutter(True)], 'state')
    o['hatch'] = ([hatch(False), hatch(True)], 'state')
    o['pan'] = ([pan_px()], 'one')
    o['steam'] = ([steam(False), steam(True)], 'state')
    o['haze'] = ([haze(False), haze(True)], 'state')
    o['fan'] = ([fan(k) for k in range(4)], 'turning')
    o['stalactite'] = ([stalactite(t, r) for t in range(3) for r in (0, 1)], 'tone')
    for sym in ('seed', 'sun', 'leaf', 'snow'):
        o['dial_' + sym] = ([dial(sym, r) for _ in range(3) for r in (0, 1)], 'tone')
    o['socket'] = ([socket(False), socket(True)], 'state')
    o['facet'] = ([facet(0), facet(1)], 'state')
    o['flood'] = ([flood(False), flood(True)], 'state')
    o['node'] = ([markstone(False), markstone(True)], 'state')
    o['cradle'] = ([cradle(False), cradle(True)], 'state')
    o['telescope'] = ([telescope()], 'one')
    o['sundial'] = ([sundial()], 'one')
    o['cart'] = ([cart()], 'one')
    o['barrel'] = ([barrel()], 'one')
    for kind in ('crab', 'wasp', 'bat', 'robot', 'squirrel'):
        o[kind] = ([critter(kind, f) for f in range(3)], 'creature')
    return o


def effect_sheets():
    return {'water': [water(0), water(1)], 'splash': [splash(0), splash(1)], 'beam': [beam(m) for m in range(16)]}


def item_icons(names):
    """One 16 x 16 icon per item, in item order (worldlib.ITEMS)."""
    table = {'NOTEBOOK': notebook, 'LENS': lens, 'APPLE': apple, 'BANDAGE': bandage, 'COG': cog,
             'DIAGRAM': scroll, 'WHEELMECH': wheelmech, 'ROPE': rope, 'PLANK': plank, 'KEY': key,
             'SURVEYMAP': scroll, 'TIDETABLE': scroll, 'ESCAPEMENT': cog, 'OPTLENS': lens, 'GEAR': cog,
             'CRANK': crank, 'GEARASSEMBLY': wheelmech, 'INSTRUMENT': compass, 'NAVCHART': scroll,
             'OILCAN': oilcan, 'SEEDS': acorn, 'BERRIES': apple, 'TEA': bandage, 'SAPLING': sapling,
             'FUSE': key, 'PLAN': scroll, 'LOGBOOK': note, 'PIPE': pipe_item}
    out = []
    for n in names:
        f = table.get(n)
        out.append(f() if f else note())
    return out


def category_icons():
    """flower, history, diagram, photo, specimen, wonder, heart."""
    return [flower((240, 120, 160)), badge(), scroll(), photo(), moss(), wheelmech(), heart_icon()]
