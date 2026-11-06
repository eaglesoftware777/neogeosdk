#!/usr/bin/env python3
"""
Build the illustrated release overview (landscape slides).

    python3 tools/make_overview_pdf.py [-o docs/neogeosdk_v1.7.7_overview.pdf]

The slides are written here as data (SLIDES, below) and typeset with
ReportLab on a dark page, one idea a slide. The facts come from
docs/releases/ANNOUNCEMENT_v1.7.7.md and CHANGELOG.md; when they change,
change the slide text here and run the script again.
"""

import argparse
import os

from reportlab.lib import colors
from reportlab.lib.enums import TA_LEFT
from reportlab.lib.styles import ParagraphStyle
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.pdfgen import canvas
from reportlab.platypus import Paragraph

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
VERSION = 'v1.7.7'
W, H = 1092.0, 614.25               # the v1.7.0 overview's page
MARGIN = 44.0

BG = colors.HexColor('#0b0b0e')
CARD = colors.HexColor('#121318')
INK = colors.HexColor('#f2efe9')
MUTED = colors.HexColor('#8b909a')
DIM = colors.HexColor('#5d626c')
RULE = colors.HexColor('#24262d')
ORANGE = colors.HexColor('#f5a524')
PINK = colors.HexColor('#ff3d7f')
TEAL = colors.HexColor('#2ec4b6')
YELLOW = colors.HexColor('#ffd23f')
BLUE = colors.HexColor('#4aa3ff')
ACCENTS = {'orange': ORANGE, 'pink': PINK, 'teal': TEAL, 'yellow': YELLOW, 'blue': BLUE}

DEJAVU = '/usr/share/fonts/truetype/dejavu'
pdfmetrics.registerFont(TTFont('Sans', f'{DEJAVU}/DejaVuSans.ttf'))
pdfmetrics.registerFont(TTFont('Sans-B', f'{DEJAVU}/DejaVuSans-Bold.ttf'))
pdfmetrics.registerFont(TTFont('Sans-CB', f'{DEJAVU}/DejaVuSansCondensed-Bold.ttf'))
pdfmetrics.registerFont(TTFont('Mono', f'{DEJAVU}/DejaVuSansMono.ttf'))
pdfmetrics.registerFont(TTFont('Mono-B', f'{DEJAVU}/DejaVuSansMono-Bold.ttf'))
pdfmetrics.registerFontFamily('Sans', normal='Sans', bold='Sans-B', italic='Sans', boldItalic='Sans-B')


def hexof(c):
    return '#%02x%02x%02x' % tuple(int(v * 255) for v in (c.red, c.green, c.blue))


def para(c, text, x, y_top, width, size=11.5, color=MUTED, font='Sans', leading=None):
    """Wrapped text from its top edge; returns the height used."""
    style = ParagraphStyle('p', fontName=font, fontSize=size, leading=leading or size * 1.42,
                           textColor=color, alignment=TA_LEFT)
    p = Paragraph(text, style)
    _, h = p.wrap(width, 1000)
    p.drawOn(c, x, y_top - h)
    return h


def eyebrow(c, text, color=ORANGE, y=H - 112):
    c.setFont('Sans-B', 9)
    c.setFillColor(color)
    c.drawString(MARGIN, y, text.upper())


def headline(c, parts, y=H - 152, size=36):
    """parts: [(text, color or None)]; drawn on one line."""
    x = MARGIN
    c.setFont('Sans-B', size)
    for text, col in parts:
        c.setFillColor(col or INK)
        c.drawString(x, y, text)
        x += pdfmetrics.stringWidth(text, 'Sans-B', size)
    return y


def frame(c, n, total, section, right):
    c.setFillColor(BG)
    c.rect(0, 0, W, H, stroke=0, fill=1)
    c.setStrokeColor(RULE)
    c.setLineWidth(0.6)
    c.line(MARGIN, H - 42, W - MARGIN, H - 42)
    c.setFont('Sans-B', 8)
    c.setFillColor(MUTED)
    c.drawString(MARGIN, H - 32, f'{n:02d} / {section.upper()}')
    c.drawRightString(W - MARGIN, H - 32, right.upper())
    c.setFont('Sans', 8)
    c.setFillColor(DIM)
    c.drawString(MARGIN, 26, f'NEOGEOSDK {VERSION.upper()} / EAGLE SOFTWARE')
    c.drawRightString(W - MARGIN, 26, f'{n:02d} / {total:02d}')


def card(c, x, y_top, w, h, label, title, body, accent, title_size=17):
    c.setFillColor(CARD)
    c.setStrokeColor(accent)
    c.setLineWidth(0.9)
    c.roundRect(x, y_top - h, w, h, 4, stroke=1, fill=1)
    c.setFont('Sans-B', 7.5)
    c.setFillColor(MUTED)
    c.drawString(x + 14, y_top - 20, label.upper())
    used = para(c, f'<b>{title}</b>', x + 14, y_top - 26, w - 28, size=title_size, color=INK,
                font='Sans-B', leading=title_size * 1.2)
    para(c, body, x + 14, y_top - 34 - used, w - 28, size=9.5, color=MUTED)


def cards(c, items, y_top=H - 200, h=150, cols=None, gap=16, x0=MARGIN, width=None):
    cols = cols or len(items)
    width = width or W - MARGIN - x0
    w = (width - gap * (cols - 1)) / cols
    for k, (label, title, body, accent) in enumerate(items):
        row, col = divmod(k, cols)
        card(c, x0 + col * (w + gap), y_top - row * (h + gap), w, h, label, title, body, ACCENTS[accent])


def note(c, label, text, y_top, color=TEAL, h=None):
    w = W - 2 * MARGIN
    c.setStrokeColor(RULE)
    c.setDash(2, 3)
    probe = Paragraph(text, ParagraphStyle('n', fontName='Sans', fontSize=10, leading=14.5))
    _, th = probe.wrap(w - 28, 1000)
    hh = h or th + 40
    c.rect(MARGIN, y_top - hh, w, hh, stroke=1, fill=0)
    c.setDash()
    c.setFont('Sans-B', 7.5)
    c.setFillColor(color)
    c.drawString(MARGIN + 14, y_top - 18, label.upper())
    para(c, text, MARGIN + 14, y_top - 26, w - 28, size=10, color=MUTED)


def table(c, rows, widths, y_top, head_color=ORANGE, size=10, row_h=24, x0=MARGIN):
    y = y_top
    for r, row in enumerate(rows):
        x = x0
        if r == 0:
            c.setFont('Sans-B', 7.5)
            c.setFillColor(head_color)
        for k, cell in enumerate(row):
            if r == 0:
                c.drawString(x, y, cell.upper())
            else:
                font = 'Sans-B' if k == 0 else 'Sans'
                col = INK if k == 0 else MUTED
                para(c, cell, x, y + size, widths[k] - 12, size=size, color=col, font=font)
            x += widths[k]
        y -= row_h if r else 18
        if r:
            c.setStrokeColor(RULE)
            c.setLineWidth(0.5)
            c.line(x0, y + row_h - 6, x0 + sum(widths), y + row_h - 6)
    return y


def bullets(c, items, y_top, x=MARGIN, width=W - 2 * MARGIN, size=11.5, accent=ORANGE, gap=8):
    y = y_top
    for it in items:
        c.setFillColor(accent)
        c.rect(x, y - size * 0.78, 5, 5, stroke=0, fill=1)
        it = it.replace('<b>', f'<font color="{hexof(INK)}"><b>').replace('</b>', '</b></font>')
        h = para(c, it, x + 16, y, width - 16, size=size, color=MUTED)
        y -= h + gap
    return y


def code(c, lines, x, y_top, w, size=10):
    h = len(lines) * size * 1.5 + 22
    c.setFillColor(colors.HexColor('#0f1014'))
    c.setStrokeColor(RULE)
    c.roundRect(x, y_top - h, w, h, 4, stroke=1, fill=1)
    c.setFont('Mono', size)
    y = y_top - 16 - size
    for ln in lines:
        c.setFillColor(TEAL if ln.startswith('$') else INK if not ln.startswith('#') else DIM)
        c.drawString(x + 14, y, ln)
        y -= size * 1.5
    return h


# --- the slides ---------------------------------------------------------------

def s_cover(c, n, total):
    frame(c, n, total, 'NeoGeoSDK', 'Technical overview · 2026')
    c.setFillColor(ORANGE)
    c.rect(MARGIN, 120, W - 2 * MARGIN, 3, stroke=0, fill=1)
    c.setFont('Sans-B', 9)
    c.setFillColor(ORANGE)
    c.drawString(MARGIN, H - 150, f'NEOGEOSDK / {VERSION.upper()} · EAGLE SOFTWARE · MIT')
    c.setFont('Sans-B', 76)
    c.setFillColor(ORANGE)
    c.drawString(MARGIN, H - 240, 'NEO·GEO')
    c.setFillColor(INK)
    c.drawString(MARGIN, H - 318, 'SDK')
    c.setFont('Sans-B', 26)
    c.setFillColor(PINK)
    c.drawString(MARGIN + 220, H - 312, 'The games release')
    para(c, 'Seven games built to real ROMs, four genre engines, a fighting core, an open system ROM '
            'and an experimental Neo Geo CD backend, on direct 68000 control of video, palettes, '
            'sprites, FIX tiles and a Z80-driven YM2610 sound stack.',
         MARGIN, H - 350, 620, size=12.5, color=MUTED)
    c.setStrokeColor(ORANGE)
    c.roundRect(W - MARGIN - 190, 150, 190, 44, 4, stroke=1, fill=0)
    c.setFont('Sans-B', 15)
    c.setFillColor(ORANGE)
    c.drawCentredString(W - MARGIN - 95, 166, 'INSERT COIN')
    c.setFont('Sans-B', 9)
    c.setFillColor(MUTED)
    c.drawString(MARGIN, 140, 'SNK ARCADE / HOME · 68000 + Z80 + YM2610 · MVS · AES · NEO GEO CD (EXPERIMENTAL)')


def s_pitch(c, n, total):
    frame(c, n, total, 'Overview', 'What is NeoGeoSDK')
    eyebrow(c, 'The pitch')
    headline(c, [('Real Neo Geo development, ', None), ('made practical.', ORANGE)])
    para(c, 'For self-made games, demos and homebrew that stay close to how the machine was '
            'programmed, with modern build tooling, scripting and emulator-based testing around it.',
         MARGIN, H - 190, 400, size=12.5)
    table(c, [('', ''),
              ('Target', 'SNK Neo Geo: MVS (arcade), AES (home); Neo Geo CD experimental'),
              ('Release', f'{VERSION} · MIT licensed · SDK 1.7.7, Framework 1.5'),
              ('Host platforms', 'Linux · WSL · Windows'),
              ('Languages', 'C99 · C++14 · 68000 asm · Z80 asm · Python'),
              ('Audio', 'FM · SSG · ADPCM-A · ADPCM-B'),
              ('Games', 'Seven, each with MVS and AES sets, NeoSD, Darksoft and BackBit packages'),
              ('Engines', 'Plain C and C++14, one ABI; opt-in shooter, fighting and genre cores')],
          [130, 400], H - 186, size=10, row_h=30, x0=MARGIN + 470)


def s_glance(c, n, total):
    frame(c, n, total, 'Overview', 'v1.7.0 to v1.7.7')
    eyebrow(c, 'At a glance')
    headline(c, [('Five pre-releases. ', None), ('One release.', PINK)])
    cards(c, [
        ('v1.7.1', 'Framework 1.0', 'The C engine frozen and versioned; video written in the blank; '
                                    'sound commands queued. Maiya and EagleBIOS.', 'orange'),
        ('v1.7.2', 'Frames in the blank', 'A deadline, priorities, moves first: no frame ever '
                                          'written on a drawn line.', 'teal'),
        ('v1.7.3', 'Shooters', 'ng_shooter, pinned character slots. Sky Lance\'s first set.', 'pink'),
        ('v1.7.5', 'Fighting', 'ng_fight: motions, cancels, teams, strikers. Shadow League. '
                               'EagleBIOS v1.7.4.', 'yellow'),
        ('v1.7.7', 'Genre engines', 'Puzzle, mahjong, RPG, beat\'em-up; Neo Geo CD; the ROM '
                                    'registry; seven games released.', 'blue'),
    ], h=170)
    note(c, 'Compatibility', 'Every release since Framework 1.0 adds and never changes: a game written '
                             'for v1.7.0 builds unchanged on v1.7.7.', H - 400)


def s_hardware(c, n, total):
    frame(c, n, total, 'Hardware', 'The target machine')
    eyebrow(c, 'The target machine')
    headline(c, [('Three chips. One bus. ', None), ('No engine.', PINK)])
    cards(c, [
        ('Main CPU', 'M68000 @ 12 MHz', 'Game logic, video memory, palettes, sprites, FIX tiles, directly.', 'orange'),
        ('Sound CPU', 'Z80A @ 4 MHz', 'Runs the SDK\'s driver; takes commands from the 68000 through ports.', 'teal'),
        ('Sound chip', 'YM2610', 'FM synthesis, SSG square waves, ADPCM-A effects and ADPCM-B music.', 'pink'),
    ], h=110)
    note(c, 'The development model', 'Graphics, palettes, FIX tiles, ROM layout and sound commands are part '
            'of everyday programming rather than hidden behind a large engine. The SDK keeps that: it '
            'gives you headers, build glue, asset pipelines and a 2D engine you opt into, not '
            'abstractions you cannot see past.', H - 330)


def divider(number, title, sub, color):
    def draw(c, n, total):
        frame(c, n, total, title, sub)
        c.setFont('Sans-B', 120)
        c.setFillColor(color)
        c.drawString(MARGIN, H - 290, number)
        c.setFont('Sans-B', 34)
        c.setFillColor(INK)
        c.drawString(MARGIN, H - 340, title)
        c.setFillColor(color)
        c.rect(MARGIN, H - 360, 240, 2, stroke=0, fill=1)
        para(c, sub, MARGIN, H - 376, 620, size=12)
    return draw


def s_games(c, n, total):
    frame(c, n, total, 'The games', 'Seven games, real ROMs')
    eyebrow(c, 'What builds from this repository')
    headline(c, [('The ', None), ('shipped games.', TEAL)])
    table(c, [('Game', 'ID', 'Genre', 'In it'),
              ('Maiya: Super Nature Girl', '780', 'Action platformer',
               'Twelve stages, guardians behind sealed gates, hidden vaults; Maiya or her sister Luna'),
              ('Sky Lance', '779', 'Vertical shooter', 'Seven painted routes to boss arenas, three jets'),
              ('Shadow League Fighting', '781', 'Fighting',
               '44 fighters, 12 arenas, team battles with strikers, voices, score and bonus counts'),
              ('No Signal Z Alpha', '784', 'Beat\'em-up', 'Eight streets for one player, grabs, throws, a scooter'),
              ('Maiya & Luna', '783', 'Adventure RPG', 'Two solo journeys, seven chapters, puzzles, saves'),
              ('Seven Wonders Puzzles', '782', 'Puzzle', 'Match, bubble and picture-logic boards, an album'),
              ('Mahjong Demo', '785', 'Mahjong', 'Hong Kong Old Style against three CPU players, solitaire')],
          [230, 50, 150, 570], H - 186, size=10, row_h=34)


def s_fighting_game(c, n, total):
    frame(c, n, total, 'The games', 'Shadow League Fighting')
    eyebrow(c, 'Shadow League Fighting · 781', PINK)
    headline(c, [('A fighter, ', None), ('front to back.', PINK)])
    bullets(c, [
        '<b>44 painted fighters, 12 arenas</b>: single matches and three-on-three team battles with a striker '
        'per team, a referee who calls the round, voices for every move.',
        '<b>Every round ends like one</b>: the winner\'s pose over the loser, team bouts included; the beaten '
        'team kneels at the match\'s end.',
        '<b>A score with bonus counts</b>: life, time and untouched bonuses after each round won, counted into '
        'the score; TRIPLE UNTOUCHED and CLEAN SWEEP finales with their own fanfares.',
        '<b>Arenas in 32 palettes</b>, fitted tile by tile: skies, beams and wet floors keep their gradients.',
        '<b>Combos for everyone</b>: a connected normal cancels into any special or super, a special that '
        'hits into a super, beside each fighter\'s own routes.',
    ], H - 196, size=15, gap=14)


def s_more_games(c, n, total):
    frame(c, n, total, 'The games', 'The rest of the line-up')
    eyebrow(c, 'Engines in use', TEAL)
    headline(c, [('Each game ', None), ('shows an engine.', TEAL)])
    cards(c, [
        ('Maiya · 780', 'Platformer', 'Deferred video, pinned hero slots, raster bands, landmarks loaded at a '
                                       'stage\'s start; busy fights at full speed.', 'orange'),
        ('Sky Lance · 779', 'ng_shooter', 'Scroll, camera, parallax, a strip budget per line; fixed-point '
                                            'aimed shots, anchored clouds.', 'teal'),
        ('No Signal · 784', 'ng_beatemup', 'Waves, a crowd director, hitstop per move, stride-driven walk '
                                             'and run, ridable vehicles.', 'pink'),
        ('Maiya & Luna · 783', 'ng_rpg', 'Rooms, people, items and environmental puzzles from ROM tables; '
                                          'saves.', 'yellow'),
        ('Seven Wonders · 782', 'ng_puzzle', 'Match, bubble and picture-logic boards with rewards.', 'blue'),
        ('Mahjong Demo · 785', 'ng_mahjong', 'A four-seat table against the CPU, and solitaire.', 'orange'),
    ], cols=3, h=128)


def s_framework(c, n, total):
    frame(c, n, total, 'The engine', 'Framework 1.0 to 1.5')
    eyebrow(c, 'Additions only')
    headline(c, [('The engine ', None), ('only grows.', ORANGE)])
    table(c, [('Framework', 'Release', 'Added'),
              ('1.0', 'v1.7.1', 'Public calls frozen (docs/api/framework_v1.txt, make api-check); VRAM in the blank; sound queue'),
              ('1.1', 'v1.7.2', 'Raster bands on the line timer (ng_raster)'),
              ('1.2', 'v1.7.2', 'VRAM priorities and the blank\'s deadline'),
              ('1.3', 'v1.7.3', 'ng_shooter; pinned character slots; depth-sorted cast'),
              ('1.4', 'v1.7.5', 'ng_fight, the fighting core'),
              ('1.5', 'v1.7.7', 'Genre engines; 20-bit tile addresses; frame sync; move-first groups')],
          [130, 110, 760], H - 186, size=10.5, row_h=36)


def s_blank(c, n, total):
    frame(c, n, total, 'The engine', 'Video in the blank')
    eyebrow(c, 'NG_VRAM_DEFER', TEAL)
    headline(c, [('Nothing is written ', None), ('on a drawn line.', TEAL)])
    cards(c, [
        ('Deadline', 'Line 8', 'No job starts after it: what does not fit waits for the next blank.', 'teal'),
        ('Order', 'Moves first', 'Then content by priority: HIGH, streaming strips, NORMAL, FIX text, LOW.', 'orange'),
        ('Move-first groups', 'NG_SG_MOVE_FIRST', 'A group already showing its picture keeps moving with the '
                                                   'scenery while its new picture waits.', 'pink'),
        ('Frame sync', 'One batch a frame', 'The main loop publishes a finished frame; the interrupt takes '
                                             'it whole.', 'yellow'),
    ], cols=4, h=150)
    note(c, 'Why it matters', 'A frame that tears or shows half a picture never reaches the screen: it waits a '
                              'frame instead, and the game measures how often that happens (make PERF=1).',
         H - 380)


def s_genres(c, n, total):
    frame(c, n, total, 'The engine', 'Opt-in cores')
    eyebrow(c, 'One line in game.mk')
    headline(c, [('Six cores, ', None), ('picked per game.', YELLOW)])
    cards(c, [
        ('-DNG_SHOOTER=1', 'Shooter', 'Vertical scroll, smooth follow, paced parallax, strip budget.', 'teal'),
        ('-DNG_FIGHT=1', 'Fighting', 'Motions, cancels, super freeze, projectiles, teams, strikers, CPU ranks, training.', 'pink'),
        ('GAME_GENRE_ENGINE = puzzle', 'Puzzle', 'Match and bubble boards, picture logic, hints.', 'blue'),
        ('= mahjong', 'Mahjong', 'Hong Kong Old Style for four seats; solitaire layouts.', 'orange'),
        ('= rpg', 'RPG', 'Rooms, exits, party, items, dialogue, environmental puzzles, saves.', 'yellow'),
        ('= beatemup', 'Beat\'em-up', 'Waves, crowd director, grabs and throws, pick-ups, vehicles.', 'teal'),
    ], cols=3, h=128)


def s_fight_core(c, n, total):
    frame(c, n, total, 'The engine', 'ng_fight')
    eyebrow(c, 'The fighting core', PINK)
    headline(c, [('Fighters are ', None), ('data.', PINK)])
    bullets(c, [
        '<b>Input</b>: a 64-frame ring, quarter and half circles, dragon punch, charge, 360 and 720, negative '
        'edge, two buttons a frame apart still read together.',
        '<b>Moves</b>: authored cancel routes per fighter plus shared ones; juggle limits; MAX mode; guard '
        'cancels; throws with grips, carries and breaks.',
        '<b>Matches</b>: rounds or three-on-three teams with strikers; the game can hold a round\'s end for its '
        'own ceremony; a deterministic match hash.',
        '<b>CPU and training</b>: seven calibrated ranks; recording and playback dummies.',
    ], H - 196, size=15, gap=14, accent=PINK)


def s_budget(c, n, total):
    frame(c, n, total, 'Performance', 'The frame budget')
    eyebrow(c, 'Measured in MAME, frame by frame')
    headline(c, [('16.7 ms. ', None), ('That\'s the budget.', ORANGE)])
    cards(c, [
        ('Colour mixing', '45 divides to 0', 'A 16-colour tint used the library\'s 32-bit divide 45 times; '
                                             'shifts and adds now give the same colours.', 'orange'),
        ('Maiya, worst stage', '50 to 57 fps', 'Golden Savanna\'s fights, before and after the release\'s '
                                                'performance pass.', 'teal'),
        ('Dropped frames', '17.5% to 3.1%', 'Same stage, same run, the share of frames that overran.', 'pink'),
    ], h=130)
    note(c, 'Rule 14: divide in 16 bits', 'The 68000 divides 32 bits by 16. A division the compiler cannot prove '
            'fits becomes a library call several times slower; docs/performance_rules.md lists the rules '
            'every game follows.', H - 360)


def s_bios(c, n, total):
    frame(c, n, total, 'Platform', 'EagleBIOS')
    eyebrow(c, 'The open system ROM', YELLOW)
    headline(c, [('Our own BIOS, ', None), ('in every package.', YELLOW)])
    bullets(c, [
        '<b>MVS and AES</b>: boots every SDK game; packages ship EagleBIOS and no other BIOS.',
        '<b>Memory cards</b>: 16-bit JEIDA/PCMCIA detection, a save directory, errors; a checksummed backup '
        'RAM format with migration.',
        '<b>Clock and menus</b>: the real-time clock, settings, cabinet and game DIPs, diagnostics, sound and '
        'input tests, cartridge information.',
        '<b>HOW TO PLAY</b>: a game\'s own text page (HTP1), with joystick and button badges.',
    ], H - 196, size=15, gap=14, accent=YELLOW)


def s_cd(c, n, total):
    frame(c, n, total, 'Platform', 'Neo Geo CD')
    eyebrow(c, 'Experimental', BLUE)
    headline(c, [('From cartridge ', None), ('to disc.', BLUE)])
    cards(c, [
        ('Programs', 'PRG and IPL', 'Native CD programs from the same C sources; format and bank checks.', 'blue'),
        ('Discs', 'ISO and BIN/CUE', 'Authored by the SDK; one BIN/CUE per disc for SD loaders and burning.', 'teal'),
        ('Ports', 'Four games', 'Maiya, Sky Lance, Maiya & Luna and No Signal, with streamed scenery and CD audio.', 'orange'),
    ], h=120)
    note(c, 'Status', 'All four discs boot in MAME with stock CDZ firmware and with EagleBIOS-CD. Saves under '
                      'EagleBIOS-CD and physical discs are still to be verified: '
                      'see docs/NEOGEO_CD_PIPELINE_AND_TESTING_GUIDE.md.', H - 350, color=BLUE)


def s_registry(c, n, total):
    frame(c, n, total, 'Shipping', 'Every ROM named once')
    eyebrow(c, 'releases/rom-registry.json')
    headline(c, [('One name, ', None), ('one set of bytes.', TEAL)])
    bullets(c, [
        '<b>A version per game</b>, MAJOR.MINOR.PATCH: new content, improvements, fixes. Changed bytes always get '
        'a new version.',
        '<b>A build identity</b>: twelve hex digits over the hashes of the six ROM parts; a MAME set, a Darksoft '
        'folder and a BackBit zip of one build share it.',
        '<b>Audits</b>: make_release.py refuses to replace a version that went out; audit_release.py checks every '
        'package; rom_registry.py --verify checks the release folder.',
    ], H - 196, size=15, gap=14, accent=TEAL)


def s_testing(c, n, total):
    frame(c, n, total, 'Shipping', 'Testing')
    eyebrow(c, 'MAME, both machines')
    headline(c, [('Tried in MAME, ', None), ('every build.', ORANGE)])
    cards(c, [
        ('mame_bats.py', 'Launchers', 'A menu, every game for MVS and AES, level launchers that start any '
                                       'stage, video recording steps.', 'orange'),
        ('install_wip_mame.py', 'Installs', 'A build into the test folder with its own software list, '
                                             'settings and saves.', 'teal'),
        ('release_all_games.sh', 'Pipeline', 'Every game built, host tests, registry checks, boots, a QA '
                                              'summary.', 'pink'),
        ('Game QA', 'Scripts', 'Matches played, captures at round ends, frame and VRAM counters, '
                               'instruction profiles.', 'yellow'),
    ], cols=4, h=150)


def s_packages(c, n, total):
    frame(c, n, total, 'Shipping', 'Packages')
    eyebrow(c, 'For each game, each version')
    headline(c, [('Emulator, flash cart, ', None), ('cabinet.', PINK)])
    table(c, [('Package', 'For'),
              ('-WIP-EagleBIOS.zip / -WIP-AES-EagleBIOS.zip', 'MAME: the set, its software list, EagleBIOS (AES: a blank memory card)'),
              ('-NeoSD_MVS.neo / -NeoSD_AES.neo', 'NeoSD: cartridge data, the system\'s own BIOS'),
              ('-Darksoft-MVS.zip / -AES.zip', 'Darksoft Neo Geo Multi'),
              ('-BackBit-MVS.zip / -AES.zip', 'BackBit Platinum'),
              ('-README / -CHANGELOG / -MANIFEST', 'How to play; what changed; sizes, CRC-32 and SHA-256')],
          [430, 570], H - 186, size=10.5, row_h=36)


def s_start(c, n, total):
    frame(c, n, total, 'Get started', 'Build, run, write')
    eyebrow(c, 'From a clean checkout')
    headline(c, [('make GAME=<name> all ', None), ('make test', ORANGE)], size=30)
    code(c, ['# Linux / WSL', '$ make GAME=shadowleague all', '$ make neo GAME=maiya',
             '$ tools/release_all_games.sh', '', '# Windows', '$ make -f MakefileWin32.mak GAME=skylance all'],
         MARGIN, H - 190, 470)
    cards(c, [('Repository', 'github.com/eaglesoftware777/neogeosdk', 'Source, issues and releases.', 'orange'),
              ('Release', VERSION, 'SDK 1.7.7 · Framework 1.5', 'teal'),
              ('Licence', 'MIT', 'Use it, ship games with it.', 'pink')],
          y_top=H - 190, h=96, cols=1, gap=12, x0=MARGIN + 500)



def build(out):
    plan = [s_cover, s_pitch, s_glance, s_hardware,
            divider('01', 'The games', 'Seven games build to real ROMs from this repository, each for MVS and AES.', TEAL),
            s_games, s_fighting_game, s_more_games,
            divider('02', 'The engine', 'One engine in plain C or C++14; opt-in cores for shooters, fighters and four genres.', ORANGE),
            s_framework, s_blank, s_genres, s_fight_core, s_budget,
            divider('03', 'The platform', 'EagleBIOS for MVS and AES, and an experimental road to the Neo Geo CD.', YELLOW),
            s_bios, s_cd,
            divider('04', 'Shipping', 'Versions that mean one set of bytes, packages for every machine, tests on both.', PINK),
            s_registry, s_testing, s_packages, s_start]
    c = canvas.Canvas(out, pagesize=(W, H))
    c.setTitle(f'NeoGeoSDK {VERSION} — Technical Overview')
    c.setAuthor('Eagle Software')
    c.setSubject('Neo Geo development')
    c.setCreator('NeoGeoSDK tools/make_overview_pdf.py')
    total = len(plan)
    for k, draw in enumerate(plan, 1):
        draw(c, k, total)
        c.showPage()
    c.save()
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('-o', '--out', default=os.path.join(ROOT, 'docs', f'neogeosdk_{VERSION}_overview.pdf'))
    args = ap.parse_args()
    path = build(args.out)
    print(f'wrote {os.path.relpath(path, ROOT)}  ({os.path.getsize(path) / 1024:.0f} KB)')


if __name__ == '__main__':
    main()
