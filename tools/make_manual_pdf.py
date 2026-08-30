#!/usr/bin/env python3
"""
Build the printable Neo Geo SDK manual.

    python3 tools/make_manual_pdf.py [-o docs/neogeosdk_v1.7.0_manual.pdf]

Renders the programmer's manual, the bare-metal SDK library reference, and
the C and C++ 2D engine references into a single PDF with a cover, a table
of contents, and running headers.

The source of truth is the markdown in docs/ - this script only typesets it,
so regenerating the API references (tools/gen_api_reference.py) and then
rerunning this is enough to bring the PDF back in sync.
"""

import argparse
import os
import re
import sys

from reportlab.lib import colors
from reportlab.lib.enums import TA_LEFT
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib.units import mm
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.platypus import (BaseDocTemplate, CondPageBreak, Frame,
                                KeepTogether, NextPageTemplate, PageBreak,
                                PageTemplate, Paragraph, Spacer, Table,
                                TableStyle)

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
VERSION = 'v1.7.0'
TITLE = 'Neo Geo SDK'
SUBTITLE = 'Programmer’s Manual and Library Reference'
PUBLISHER = 'Eagle Software'

# (file, part title, blurb shown on the part divider)
SECTIONS = [
    ('docs/INTRODUCTION.md', 'Introduction',
     'What the SDK is, the machine it targets, and how to read the rest.'),
    ('docs/PROGRAMMERS_MANUAL.md', 'Programmer’s Manual',
     'The build system, the project layout, the game contract, the engine '
     'tick, the asset pipelines, and the frame budget.'),
    ('docs/HELLO_WORLD.md', 'Tutorial: Hello World',
     'From a clean checkout to a running ROM, then into a game of your own.'),
    ('SDK_API_GUIDE.md', 'SDK Library Reference',
     'The bare-metal 68000 helpers declared in sdk/neogeo.h - sprite control '
     'blocks, palettes, VRAM, the FIX layer, timing, input, and sound.'),
    ('docs/API_2D_ENGINE_C.md', '2D Engine Reference — C',
     'Every public call in the plain-C 2D engine, grouped by module.'),
    ('docs/API_2D_ENGINE_CPP.md', '2D Engine Reference — C++',
     'The C++14 build of the same engine: classes, methods, and how it '
     'relates to the C API.'),
    ('docs/SOUND_DRIVER.md', 'Sound Driver Reference',
     'The Z80 driver, the YM2610, the command protocol, and the audio '
     'content pipelines.'),
    ('docs/ARTBOX_PIPELINE.md', 'Graphics Pipeline Reference',
     'PNG to C-ROM: the quantiser, asset ordering, generated outputs, and '
     'Artbox Studio.'),
    ('docs/TOOLS.md', 'Tools and Utilities',
     'Every helper script that ships with the SDK - art conversion, sound '
     'encoding, verification, launchers, installers and the GIMP plug-ins.'),
]

INK = colors.HexColor('#14161a')
MUTED = colors.HexColor('#5b6472')
ACCENT = colors.HexColor('#b3261e')
RULE = colors.HexColor('#c9ced8')
CODE_BG = colors.HexColor('#f4f5f7')
TABLE_HEAD = colors.HexColor('#eceef2')

DEJAVU = '/usr/share/fonts/truetype/dejavu'

# Characters outside WinAnsi, mapped for the built-in-font fallback.
FALLBACK_MAP = {
    '→': '->', '←': '<-', '▶': '>', '◀': '<',
    '▲': '^', '▼': 'v', '≥': '>=', '≤': '<=',
    '─': '-', '│': '|', '┌': '+', '┐': '+',
    '└': '+', '┘': '+', '├': '+', '┤': '+',
    '┬': '+', '┴': '+', '┼': '+', '═': '=',
    '³': '^3', '²': '^2', '…': '...', '✓': 'x',
}


def register_fonts():
    """Prefer DejaVu (full Unicode); fall back to the built-in Type1 set."""
    try:
        pdfmetrics.registerFont(TTFont('Body', f'{DEJAVU}/DejaVuSans.ttf'))
        pdfmetrics.registerFont(TTFont('Body-B', f'{DEJAVU}/DejaVuSans-Bold.ttf'))
        pdfmetrics.registerFont(TTFont('Body-I', f'{DEJAVU}/DejaVuSans-Oblique.ttf'))
        pdfmetrics.registerFont(TTFont('Mono', f'{DEJAVU}/DejaVuSansMono.ttf'))
        pdfmetrics.registerFont(TTFont('Mono-B', f'{DEJAVU}/DejaVuSansMono-Bold.ttf'))
        pdfmetrics.registerFontFamily('Body', normal='Body', bold='Body-B',
                                      italic='Body-I', boldItalic='Body-B')
        return True
    except Exception:
        return False


UNICODE_OK = register_fonts()
if UNICODE_OK:
    F_BODY, F_BOLD, F_ITAL, F_MONO, F_MONO_B = (
        'Body', 'Body-B', 'Body-I', 'Mono', 'Mono-B')
else:
    F_BODY, F_BOLD, F_ITAL, F_MONO, F_MONO_B = (
        'Helvetica', 'Helvetica-Bold', 'Helvetica-Oblique',
        'Courier', 'Courier-Bold')


def clean(text):
    if UNICODE_OK:
        return text
    for k, v in FALLBACK_MAP.items():
        text = text.replace(k, v)
    return text


def esc(text):
    return (text.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;'))


INLINE_CODE = re.compile(r'`([^`]+)`')
BOLD = re.compile(r'\*\*([^*]+)\*\*')
ITALIC = re.compile(r'(?<!\*)\*([^*\n]+)\*(?!\*)')
LINK = re.compile(r'\[([^\]]+)\]\(([^)]+)\)')


def inline(text):
    """Markdown inline formatting -> reportlab mini-HTML."""
    text = clean(text)
    text = LINK.sub(lambda m: m.group(1), text)
    parts, out, last = [], [], 0
    # Protect code spans first so ** inside them is not treated as bold.
    for m in INLINE_CODE.finditer(text):
        out.append(('t', text[last:m.start()]))
        out.append(('c', m.group(1)))
        last = m.end()
    out.append(('t', text[last:]))
    for kind, chunk in out:
        if kind == 'c':
            parts.append(f'<font face="{F_MONO}" size="8.4">{esc(chunk)}</font>')
        else:
            chunk = esc(chunk)
            chunk = BOLD.sub(r'<b>\1</b>', chunk)
            chunk = ITALIC.sub(r'<i>\1</i>', chunk)
            parts.append(chunk)
    return ''.join(parts)


def styles():
    s = {}
    s['body'] = ParagraphStyle('body', fontName=F_BODY, fontSize=9.2,
                               leading=13.4, textColor=INK, alignment=TA_LEFT,
                               spaceAfter=6)
    s['h1'] = ParagraphStyle('h1', parent=s['body'], fontName=F_BOLD,
                             fontSize=18, leading=22, spaceBefore=16,
                             spaceAfter=8, textColor=INK)
    s['h2'] = ParagraphStyle('h2', parent=s['body'], fontName=F_BOLD,
                             fontSize=13.5, leading=17, spaceBefore=14,
                             spaceAfter=6, textColor=INK)
    s['h3'] = ParagraphStyle('h3', parent=s['body'], fontName=F_BOLD,
                             fontSize=10.8, leading=14, spaceBefore=11,
                             spaceAfter=4, textColor=ACCENT)
    s['h4'] = ParagraphStyle('h4', parent=s['body'], fontName=F_BOLD,
                             fontSize=9.6, leading=13, spaceBefore=9,
                             spaceAfter=3, textColor=MUTED)
    s['code'] = ParagraphStyle('code', fontName=F_MONO, fontSize=7.6,
                               leading=10.2, textColor=INK, spaceAfter=0,
                               spaceBefore=0)
    s['bullet'] = ParagraphStyle('bullet', parent=s['body'], leftIndent=13,
                                 bulletIndent=3, spaceAfter=3)
    s['quote'] = ParagraphStyle('quote', parent=s['body'], leftIndent=12,
                                textColor=MUTED, fontName=F_ITAL,
                                borderPadding=0, spaceBefore=4, spaceAfter=6)
    s['cell'] = ParagraphStyle('cell', fontName=F_BODY, fontSize=8.1,
                               leading=10.8, textColor=INK)
    s['cellh'] = ParagraphStyle('cellh', parent=s['cell'], fontName=F_BOLD)
    s['toc1'] = ParagraphStyle('toc1', parent=s['body'], fontName=F_BOLD,
                               fontSize=10.5, leading=15, spaceBefore=8,
                               spaceAfter=1)
    s['toc2'] = ParagraphStyle('toc2', parent=s['body'], fontSize=9,
                               leading=13, leftIndent=12, spaceAfter=0)
    s['coverT'] = ParagraphStyle('coverT', fontName=F_BOLD, fontSize=34,
                                 leading=38, textColor=INK)
    s['coverS'] = ParagraphStyle('coverS', fontName=F_BODY, fontSize=13,
                                 leading=18, textColor=MUTED)
    s['coverM'] = ParagraphStyle('coverM', fontName=F_MONO, fontSize=8.6,
                                 leading=13, textColor=MUTED)
    s['partT'] = ParagraphStyle('partT', fontName=F_BOLD, fontSize=26,
                                leading=31, textColor=INK)
    s['partN'] = ParagraphStyle('partN', fontName=F_MONO, fontSize=10,
                                leading=14, textColor=ACCENT)
    s['partB'] = ParagraphStyle('partB', fontName=F_BODY, fontSize=10.5,
                                leading=15.5, textColor=MUTED)
    return s


class Rule:
    """A thin horizontal rule flowable."""

    def __init__(self, width=None, thickness=0.6, color=RULE, space=5):
        self.width, self.thickness, self.color, self.space = (
            width, thickness, color, space)

    def wrap(self, aw, ah):
        self._w = self.width or aw
        return (self._w, self.thickness + self.space * 2)

    def drawOn(self, canv, x, y, _sW=0):
        canv.saveState()
        canv.setStrokeColor(self.color)
        canv.setLineWidth(self.thickness)
        canv.line(x, y + self.space, x + self._w, y + self.space)
        canv.restoreState()

    def split(self, aw, ah):
        return []

    def getKeepWithNext(self):
        return False

    def setKeepWithNext(self, v):
        pass

    def isIndexing(self):
        return 0

    def getSpaceBefore(self):
        return 0

    def getSpaceAfter(self):
        return 0


def code_block(lines, avail_w, st):
    """Render a fenced code block as a bordered, single-cell table."""
    body = '<br/>'.join(
        esc(clean(l)).replace(' ', '&nbsp;') or '&nbsp;' for l in lines)
    p = Paragraph(body, st['code'])
    t = Table([[p]], colWidths=[avail_w])
    t.setStyle(TableStyle([
        ('BACKGROUND', (0, 0), (-1, -1), CODE_BG),
        ('BOX', (0, 0), (-1, -1), 0.5, RULE),
        ('LEFTPADDING', (0, 0), (-1, -1), 7),
        ('RIGHTPADDING', (0, 0), (-1, -1), 7),
        ('TOPPADDING', (0, 0), (-1, -1), 6),
        ('BOTTOMPADDING', (0, 0), (-1, -1), 6),
        ('VALIGN', (0, 0), (-1, -1), 'TOP'),
    ]))
    return t


def md_table(rows, avail_w, st):
    ncol = max(len(r) for r in rows)
    rows = [r + [''] * (ncol - len(r)) for r in rows]
    data = [[Paragraph(inline(c), st['cellh'] if i == 0 else st['cell'])
             for c in row] for i, row in enumerate(rows)]
    # Weight columns by the longest raw cell, with a floor so no column
    # collapses to nothing.
    weights = []
    for c in range(ncol):
        longest = max(len(rows[r][c]) for r in range(len(rows)))
        weights.append(max(longest, 6))
    total = float(sum(weights))
    widths = [max(avail_w * w / total, 34) for w in weights]
    over = sum(widths) - avail_w
    if over > 0:                       # give the excess back to the widest
        widest = widths.index(max(widths))
        widths[widest] -= over
    t = Table(data, colWidths=widths, repeatRows=1)
    t.setStyle(TableStyle([
        ('BACKGROUND', (0, 0), (-1, 0), TABLE_HEAD),
        ('GRID', (0, 0), (-1, -1), 0.4, RULE),
        ('VALIGN', (0, 0), (-1, -1), 'TOP'),
        ('LEFTPADDING', (0, 0), (-1, -1), 5),
        ('RIGHTPADDING', (0, 0), (-1, -1), 5),
        ('TOPPADDING', (0, 0), (-1, -1), 4),
        ('BOTTOMPADDING', (0, 0), (-1, -1), 4),
    ]))
    return t


TABLE_SEP = re.compile(r'^\|[\s:|-]+\|$')


def render_markdown(text, avail_w, st, headings, part_no, drop_first_h1=True):
    """Markdown -> flowables.  Collects (level, title) into `headings`."""
    flow = []
    lines = text.split('\n')
    i, seen_h1 = 0, False
    while i < len(lines):
        line = lines[i].rstrip()

        if line.startswith('```'):
            block, i = [], i + 1
            while i < len(lines) and not lines[i].startswith('```'):
                block.append(lines[i])
                i += 1
            i += 1
            flow.append(Spacer(1, 3))
            flow.append(code_block(block, avail_w, st))
            flow.append(Spacer(1, 7))
            continue

        if line.startswith('|') and i + 1 < len(lines) and TABLE_SEP.match(
                lines[i + 1].strip()):
            rows = []
            header = [c.strip() for c in line.strip().strip('|').split('|')]
            rows.append(header)
            i += 2
            while i < len(lines) and lines[i].strip().startswith('|'):
                rows.append([c.strip()
                             for c in lines[i].strip().strip('|').split('|')])
                i += 1
            flow.append(Spacer(1, 3))
            flow.append(md_table(rows, avail_w, st))
            flow.append(Spacer(1, 8))
            continue

        if line.strip() in ('---', '***', '___'):
            flow.append(Rule(width=avail_w))
            i += 1
            continue

        m = re.match(r'^(#{1,4})\s+(.*)$', line)
        if m:
            level, title = len(m.group(1)), m.group(2).strip()
            if level == 1 and not seen_h1:
                seen_h1 = True
                if drop_first_h1:
                    i += 1
                    continue
            key = f'{part_no}.{len(headings)}'
            headings.append((level, re.sub(r'[*`]', '', title), key))
            style = st[f'h{level}']
            p = Paragraph(f'<a name="{key}"/>' + inline(title), style)
            flow.append(CondPageBreak(30 * mm) if level <= 2 else Spacer(1, 0))
            flow.append(p)
            i += 1
            continue

        if line.startswith('> '):
            buf = []
            while i < len(lines) and lines[i].startswith('>'):
                buf.append(lines[i].lstrip('>').strip())
                i += 1
            flow.append(Paragraph(inline(' '.join(b for b in buf if b)),
                                  st['quote']))
            continue

        m = re.match(r'^(\s*)([-*])\s+(.*)$', line)
        if m:
            indent, body = len(m.group(1)), m.group(3)
            j = i + 1
            while (j < len(lines) and lines[j].strip()
                   and not re.match(r'^\s*([-*]|\d+\.)\s+', lines[j])
                   and not lines[j].startswith(('#', '|', '```', '>'))):
                body += ' ' + lines[j].strip()
                j += 1
            style = ParagraphStyle('b', parent=st['bullet'],
                                   leftIndent=13 + indent * 8,
                                   bulletIndent=3 + indent * 8)
            flow.append(Paragraph(inline(body), style, bulletText='•'))
            i = j
            continue

        m = re.match(r'^(\s*)(\d+)\.\s+(.*)$', line)
        if m:
            indent, num, body = len(m.group(1)), m.group(2), m.group(3)
            j = i + 1
            while (j < len(lines) and lines[j].strip()
                   and not re.match(r'^\s*([-*]|\d+\.)\s+', lines[j])
                   and not lines[j].startswith(('#', '|', '```', '>'))):
                body += ' ' + lines[j].strip()
                j += 1
            style = ParagraphStyle('n', parent=st['bullet'],
                                   leftIndent=15 + indent * 8,
                                   bulletIndent=3 + indent * 8)
            flow.append(Paragraph(inline(body), style, bulletText=f'{num}.'))
            i = j
            continue

        if not line.strip():
            i += 1
            continue

        buf = [line]
        j = i + 1
        while (j < len(lines) and lines[j].strip()
               and not lines[j].startswith(('#', '|', '```', '>', '---'))
               and not re.match(r'^\s*([-*]|\d+\.)\s+', lines[j])):
            buf.append(lines[j].strip())
            j += 1
        flow.append(Paragraph(inline(' '.join(buf)), st['body']))
        i = j
    return flow


class Manual(BaseDocTemplate):
    def __init__(self, path, **kw):
        BaseDocTemplate.__init__(self, path, **kw)
        self.part_title = ''

    def afterFlowable(self, flowable):
        title = getattr(flowable, '_part_title', None)
        if title is not None:
            self.part_title = title


def page_furniture(canv, doc):
    canv.saveState()
    w, h = A4
    if canv.getPageNumber() > 1:
        canv.setFont(F_MONO, 7.2)
        canv.setFillColor(MUTED)
        canv.drawString(20 * mm, h - 12 * mm,
                        clean(f'{TITLE} {VERSION}'.upper()))
        title = getattr(doc, 'part_title', '')
        if title:
            canv.drawRightString(w - 20 * mm, h - 12 * mm, clean(title.upper()))
        canv.setStrokeColor(RULE)
        canv.setLineWidth(0.4)
        canv.line(20 * mm, h - 14 * mm, w - 20 * mm, h - 14 * mm)
        canv.line(20 * mm, 15 * mm, w - 20 * mm, 15 * mm)
        canv.setFont(F_MONO, 7.2)
        canv.drawCentredString(w / 2.0, 10.5 * mm, str(canv.getPageNumber()))
        canv.drawString(20 * mm, 10.5 * mm, clean(PUBLISHER.upper()))
    canv.restoreState()


def cover_page(canv, doc):
    canv.saveState()
    w, h = A4
    canv.setFillColor(INK)
    canv.rect(0, h - 58 * mm, w, 58 * mm, stroke=0, fill=1)
    canv.setFillColor(colors.white)
    canv.setFont(F_MONO, 9)
    canv.drawString(20 * mm, h - 20 * mm,
                    clean('68000  ·  Z80  ·  YM2610'))
    canv.setFont(F_BOLD, 30)
    canv.drawString(20 * mm, h - 36 * mm, clean(TITLE.upper()))
    canv.setFont(F_MONO, 12)
    canv.setFillColor(colors.HexColor('#d8dbe2'))
    canv.drawString(20 * mm, h - 47 * mm, clean(SUBTITLE))
    canv.setFillColor(ACCENT)
    canv.rect(0, h - 61 * mm, w, 3 * mm, stroke=0, fill=1)

    canv.setFillColor(INK)
    canv.setFont(F_BOLD, 11)
    canv.drawString(20 * mm, 46 * mm, clean(PUBLISHER))
    canv.setFillColor(MUTED)
    canv.setFont(F_MONO, 8.6)
    canv.drawString(20 * mm, 39 * mm, clean(f'Release {VERSION}  ·  MIT'))
    canv.drawString(20 * mm, 34 * mm,
                    'https://github.com/eaglesoftware777/neogeosdk')
    canv.setStrokeColor(RULE)
    canv.setLineWidth(0.5)
    canv.line(20 * mm, 28 * mm, w - 20 * mm, 28 * mm)
    canv.setFont(F_MONO, 7.4)
    canv.drawString(20 * mm, 22 * mm, clean(
        'SNK Neo Geo  ·  MVS arcade and AES home  ·  '
        'cartridge-era hardware'))
    canv.restoreState()


def build(out_path):
    st = styles()
    margin = 20 * mm
    doc = Manual(out_path, pagesize=A4,
                 leftMargin=margin, rightMargin=margin,
                 topMargin=22 * mm, bottomMargin=20 * mm,
                 title=f'{TITLE} {VERSION} — {SUBTITLE}',
                 author=PUBLISHER, subject='Neo Geo development')
    avail_w = A4[0] - 2 * margin
    frame = Frame(margin, 20 * mm, avail_w, A4[1] - 42 * mm, id='body')
    doc.addPageTemplates([
        PageTemplate(id='cover', frames=[frame], onPage=cover_page),
        PageTemplate(id='body', frames=[frame], onPage=page_furniture),
    ])

    story = [NextPageTemplate('body'), PageBreak()]

    # ---- gather content, collecting headings for the table of contents ----
    parts = []
    for n, (rel, part_title, blurb) in enumerate(SECTIONS, start=1):
        path = os.path.join(ROOT, rel)
        if not os.path.exists(path):
            print(f'  skip (missing): {rel}', file=sys.stderr)
            continue
        text = open(path, encoding='utf-8').read()
        headings = []
        flow = render_markdown(text, avail_w, st, headings, n)
        parts.append((n, part_title, blurb, rel, headings, flow))

    # ---- table of contents ----
    story.append(Paragraph('Contents', st['h1']))
    story.append(Rule(width=avail_w))
    story.append(Spacer(1, 4))
    for n, part_title, blurb, rel, headings, _ in parts:
        story.append(Paragraph(
            f'<a href="#part{n}">{esc(clean(f"Part {n}  ·  {part_title}"))}</a>',
            st['toc1']))
        story.append(Paragraph(
            f'<font face="{F_MONO}" size="7.4">{esc(rel)}</font>', st['toc2']))
        for level, title, key in headings:
            if level > 2:
                continue
            indent = 12 + (level - 1) * 10
            style = ParagraphStyle(f'toc{key}', parent=st['toc2'],
                                   leftIndent=indent)
            story.append(Paragraph(
                f'<a href="#{key}">{esc(clean(title))}</a>', style))
        story.append(Spacer(1, 3))

    # ---- parts ----
    for n, part_title, blurb, rel, headings, flow in parts:
        # The running header is painted when the page opens, before any of
        # its flowables are seen - so the title has to be announced on the
        # page BEFORE the break, not on the divider itself.
        marker = Spacer(1, 0)
        marker._part_title = part_title
        story.append(marker)
        story.append(PageBreak())
        divider = [
            Spacer(1, 46 * mm),
            Paragraph(f'<a name="part{n}"/>PART {n}', st['partN']),
            Spacer(1, 2),
            Paragraph(esc(clean(part_title)), st['partT']),
            Spacer(1, 5),
            Rule(width=avail_w * 0.42, thickness=1.6, color=ACCENT),
            Spacer(1, 5),
            Paragraph(esc(clean(blurb)), st['partB']),
            Spacer(1, 8),
            Paragraph(f'<font face="{F_MONO}" size="8">{esc(rel)}</font>',
                      st['partB']),
        ]
        story.extend(divider)
        story.append(PageBreak())
        story.extend(flow)

    doc.build(story)
    return out_path


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('-o', '--out',
                    default=os.path.join(ROOT, 'docs',
                                         f'neogeosdk_{VERSION}_manual.pdf'))
    args = ap.parse_args()
    os.chdir(ROOT)
    path = build(args.out)
    size = os.path.getsize(path)
    print(f'wrote {os.path.relpath(path, ROOT)}  ({size / 1024:.0f} KB)'
          f'{"" if UNICODE_OK else "  [built-in fonts, ASCII fallback]"}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
