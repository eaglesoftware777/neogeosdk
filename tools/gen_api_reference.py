#!/usr/bin/env python3
"""
Generate the 2D engine API reference documents from the engine headers.

    python3 tools/gen_api_reference.py

Writes docs/API_2D_ENGINE_C.md and docs/API_2D_ENGINE_CPP.md.

The prototype tables are extracted from the headers rather than typed by
hand, so the reference cannot drift away from the code.  The prose that
introduces each module lives in MODULES below; add a new entry when a new
ng_* module appears, and the generator will pick the header up on its own.
"""

import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
C_DIR = os.path.join(ROOT, 'sdk', '2d_engine')
CPP_DIR = os.path.join(ROOT, 'sdk', '2d_engine_plus')
DOC_DIR = os.path.join(ROOT, 'docs')

VERSION = 'v1.7.0'

# module stem -> (section, one-line purpose, longer note or '')
MODULES = {
    'ng_defs': ('Core', 'Sizes, engine constants, and the NEOGEO_USER linkage tag.', ''),
    'ng_runtime': ('Core', 'Thin alias header — includes the whole engine.', ''),
    'ng_engine': ('Core', 'Umbrella include. One #include pulls in every public module.', ''),
    'ng_game_interupt': (
        'Core',
        'The frame tick and the five hooks you attach your game to.',
        'ng_game_engine_frame() runs the whole engine in a fixed order — timers, '
        'characters, actions, physics, NPCs, camera, particles, palette effects, '
        'events — and flushes the render queue during VBlank. Register your own '
        'code at the five hook points rather than reimplementing the tick.'),
    'ng_scene': ('Core', 'Scene entry/teardown helpers.',
                 'ng_scene_begin() is the "start from a known state" call: it clears '
                 'hardware and engine state so nothing leaks in from the previous scene.'),
    'ng_game_time': ('Core', 'Frame and stage counters at 60 Hz.', ''),
    'ng_timers': ('Core', 'One-shot countdown timers, addressed by id.', ''),
    'ng_progress': ('Core', 'Bounded 0..max counters — charge meters, load bars, HP ramps.', ''),
    'ng_status': ('Core', 'A bitset of "has this happened yet" flags.', ''),
    'ng_properties': ('Core', 'A grouped int32 property matrix for game-defined values.', ''),
    'ng_game_events': ('Core', 'A small event queue with an optional handler callback.', ''),
    'ng_fixed': ('Math', '16.16 fixed-point helpers, sin/cos and shrink lookup tables.',
                 'There is no floating point on this target. Everything that would '
                 'have been a float is 16.16 fixed-point, and every trig or scale '
                 'lookup is a table.'),

    'ng_vram': ('Video', 'Direct VRAM sprite-slot clearing helpers.', ''),
    'ng_render_queue': (
        'Video',
        'A 128-slot deferred write queue, flushed inside VBlank.',
        'The engine never writes VRAM from the game logic path. Modules post '
        'commands here and ng_render_queue_flush() drains them during the ~3 ms '
        'VBlank window. ng_rq_free() tells you how much room is left.'),
    'ng_sprite_pool': (
        'Video',
        'The hardware slot map. Constants only, no functions.',
        'HIGHER slot number is drawn IN FRONT. This is the observed hardware '
        'direction and it is the opposite of what earlier revisions of this '
        'header claimed. Backgrounds belong at low slots, foreground effects at '
        'high ones. The legacy names NG_SPR_BG0_FIRST / NG_SPR_BG1_FIRST predate '
        'the correction and are kept only for existing code.'),
    'ng_sprite_group': (
        'Video',
        'Dirty-flag sprite chains — write only what changed.',
        'A group is a strip-0 anchor plus N chained strips welded together by the '
        'SCB3 sticky bit. Setting a property marks a dirty flag; the flush writes '
        'only the words that actually changed.'),
    'ng_sprite_window': ('Video', 'Tracks the strips a variable-size sprite currently occupies.',
                         'When a sprite shrinks between frames the strips it no longer '
                         'uses have to be actively hidden, or the old art stays on '
                         'screen. That bookkeeping is what a window does.'),
    'ng_bg': ('Video', 'Two scrolling sprite background layers with parallax factors.', ''),
    'ng_fix': (
        'Video',
        'The FIX text layer, with a dirty-cell cache.',
        'The layer is 40 x 32 cells of which 28 rows are visible; these helpers '
        'apply the row offset for you. Use ng_fix_blank_cell() to empty a cell — '
        'the BIOS font draws its space glyph as an opaque colour-2 plate.'),
    'ng_art_asset': ('Video', 'Type queries on generated artbox asset records.', ''),
    'ng_palette_assets': ('Video', 'Generated palette asset table lookup.', ''),
    'ng_palette_fx': (
        'Video',
        'Fade, flash, pulse and colour cycle on a palette bank.',
        'Every effect is queue-safe: the palette upload goes through the render '
        'queue, so an effect started in game logic still lands inside VBlank.'),
    'ng_depthfx': ('Video', 'Perspective projection, Z-to-shrink, fog palettes, starfields.',
                   'The Neo Geo can only shrink sprites, so depth is expressed as '
                   'reduction plus a palette swap for fog banding.'),

    'ng_chars': (
        'Characters',
        'The character pool: 64 objects, movement, animation, hitboxes, damage.',
        'ng_chars_draw() hands out hardware strips from NG_SPR_CHAR_FIRST '
        'upward. Two things catch people: ng_chars_count() returns the number of '
        'ACTIVE objects, not a high-water index, so it is not a safe array bound '
        '— iterate NG_MAX_CHARS and skip inactive slots. And a character is '
        'removed with ng_chars_remove(); setting a "destroy pending" life state '
        'is not acted on by the engine.'),
    'ng_actions': ('Characters', 'Table-driven action scripts with sound and FX hooks.', ''),
    'ng_npcs': ('Characters', 'NPC wrappers: patrol bounds, home position, think callbacks.', ''),
    'ng_physics': ('Characters', 'Gravity, drag, solid rectangles, and grounded tests.', ''),
    'ng_border_constraints': ('Characters', 'Keep characters inside declared screen regions.', ''),
    'ng_joystick': (
        'Characters',
        'Edge-detected input, repeat, motion specials, and character control.',
        'Poll once per frame with ng_joystick_update(), then read edges with '
        'ng_joy_pressed(). Acting on the raw held state instead fires every '
        'frame the button is down.'),

    'ng_level': ('World', 'Level state, world bounds, scroll and camera helpers.', ''),
    'ng_camera': ('World', 'Follow, dead zone, look-ahead, shake, cinematic pan, bounds clamp.', ''),
    'ng_particles': (
        'Effects',
        'A 32-slot fixed pool with typed spawns and priority eviction.',
        'The pool never grows and never allocates. When it is full a new spawn '
        'evicts the lowest-priority live particle, so a flood of dust cannot '
        'push out an explosion.'),
    'ng_feedback': (
        'Effects',
        'Hitstop, shake, palette flash and a sound hook, in one call.',
        'ng_impact_event() is the whole game-feel package for a hit. Prefer it '
        'over assembling the four effects by hand so intensity stays consistent.'),
    'ng_debug': ('Effects', 'A FIX-layer performance overlay.',
                 'Compile with -DNG_DEBUG_PERF=1 to enable.'),
    'ng_demo_advanced': ('Effects', 'Prebuilt showcase routines used by the demo ROM.', ''),
}

SECTION_ORDER = ['Core', 'Math', 'Video', 'Characters', 'World', 'Effects']


def protos_c(path):
    txt = open(path, encoding='utf-8', errors='replace').read()
    txt = re.sub(r'/\*.*?\*/', '', txt, flags=re.S)
    txt = re.sub(r'//[^\n]*', '', txt)
    flat = re.sub(r'\n\s+', ' ', txt)
    out = re.findall(
        r'^[A-Za-z_][A-Za-z0-9_ \*]*NEOGEO_USER\s+[A-Za-z0-9_]+\s*\([^;]*\);',
        flat, re.M)
    return [re.sub(r'\s+', ' ', p).replace('NEOGEO_USER ', '').strip() for p in out]


def constants(path):
    txt = open(path, encoding='utf-8', errors='replace').read()
    out = []
    for m in re.finditer(r'^#define\s+(NG_[A-Z0-9_]+)\s+([^\n/]+?)\s*(?:/\*|$)', txt, re.M):
        name, val = m.group(1), m.group(2).strip()
        if '(' in name or name.endswith('_H') or name.endswith('_HPP'):
            continue
        out.append((name, val))
    return out


def cpp_classes(path):
    txt = open(path, encoding='utf-8', errors='replace').read()
    txt = re.sub(r'/\*.*?\*/', '', txt, flags=re.S)
    txt = re.sub(r'//[^\n]*', '', txt)
    res = []
    for m in re.finditer(r'\b(class|struct)\s+([A-Za-z_][A-Za-z0-9_]*)\s*\{', txt):
        start = m.end() - 1
        depth, i = 0, start
        while i < len(txt):
            if txt[i] == '{':
                depth += 1
            elif txt[i] == '}':
                depth -= 1
                if depth == 0:
                    break
            i += 1
        body = txt[start + 1:i]
        # Drop everything from the first private:/protected: label on — the
        # reference documents the public surface only.
        cut = re.search(r'^\s*(private|protected)\s*:', body, re.M)
        if cut:
            body = body[:cut.start()]
        meths = []
        for mm in re.finditer(
                r'^\s+((?:static\s+|inline\s+|const\s+)*[A-Za-z_][A-Za-z0-9_:\*&<>, ]*?\s+'
                r'[A-Za-z_][A-Za-z0-9_]*\s*\([^;{)]*\))\s*(?:const\s*)?[;{]', body, re.M):
            sig = re.sub(r'\s+', ' ', mm.group(1)).strip()
            if sig.startswith(('return', 'if', 'for', 'while', 'else')):
                continue
            meths.append(sig)
        if meths:
            res.append((m.group(2), meths))
    return res


def header_note(path):
    """First block comment of the header, used as the module's own words."""
    txt = open(path, encoding='utf-8', errors='replace').read()
    m = re.search(r'/\*(.*?)\*/', txt, re.S)
    if not m:
        return ''
    lines = [re.sub(r'^\s*\*?\s?', '', l).rstrip() for l in m.group(1).split('\n')]
    lines = [l for l in lines if l.strip()]
    return ' '.join(lines[:3])[:300]


def emit_c():
    out = []
    w = out.append
    w('# 2D Engine — C API Reference')
    w('')
    w(f'**Eagle Software · Neo Geo SDK {VERSION} · `sdk/2d_engine/`**')
    w('')
    w('Every public call in the plain-C 2D engine, grouped by module. This file')
    w('is generated from the headers by `tools/gen_api_reference.py` — the')
    w('prototypes here are the prototypes that exist.')
    w('')
    w('The C++ build of the same engine is documented in')
    w('[`API_2D_ENGINE_CPP.md`](./API_2D_ENGINE_CPP.md); it exposes these same')
    w('`extern "C"` entry points plus method wrappers, so the two are')
    w('link-compatible.')
    w('')
    w('```c')
    w('#include "sdk/2d_engine/ng_engine.h"   /* everything */')
    w('```')
    w('')
    w('Every function carries the `NEOGEO_USER` linkage tag, which places it in')
    w('the cartridge\'s user code section. It is omitted from the signatures')
    w('below for readability.')
    w('')
    w('---')
    w('')

    stems = sorted(
        s for s in (os.path.splitext(f)[0] for f in os.listdir(C_DIR) if f.endswith('.h'))
        if s.startswith('ng_'))

    w('## Contents')
    w('')
    for sec in SECTION_ORDER:
        names = [s for s in stems if MODULES.get(s, ('', '', ''))[0] == sec]
        if not names:
            continue
        w(f'**{sec}** — ' + ' · '.join(f'[{n}](#{n})' for n in names))
        w('')
    unknown = [s for s in stems if s not in MODULES]
    if unknown:
        w('**Other** — ' + ' · '.join(f'`{n}`' for n in unknown))
        w('')
    w('---')
    w('')

    for sec in SECTION_ORDER:
        names = [s for s in stems if MODULES.get(s, ('', '', ''))[0] == sec]
        if not names:
            continue
        w(f'# {sec}')
        w('')
        for stem in names:
            _, purpose, note = MODULES[stem]
            path = os.path.join(C_DIR, stem + '.h')
            w(f'## {stem}')
            w('')
            w(f'`sdk/2d_engine/{stem}.h` — {purpose}')
            w('')
            if note:
                w(note)
                w('')
            ps = protos_c(path)
            if ps:
                w('```c')
                for p in ps:
                    w(p)
                w('```')
                w('')
            cs = constants(path)
            if cs:
                w('| Constant | Value |')
                w('|---|---|')
                for n, v in cs:
                    w(f'| `{n}` | `{v}` |')
                w('')
            if not ps and not cs:
                w('*Types and declarations only — see the header.*')
                w('')
        w('---')
        w('')

    if unknown:
        w('# Other modules')
        w('')
        for stem in unknown:
            path = os.path.join(C_DIR, stem + '.h')
            w(f'## {stem}')
            w('')
            note = header_note(path)
            if note:
                w(note)
                w('')
            ps = protos_c(path)
            if ps:
                w('```c')
                for p in ps:
                    w(p)
                w('```')
                w('')
    return '\n'.join(out) + '\n'


def emit_cpp():
    out = []
    w = out.append
    w('# 2D Engine — C++ API Reference')
    w('')
    w(f'**Eagle Software · Neo Geo SDK {VERSION} · `sdk/2d_engine_plus/`**')
    w('')
    w('The C++14 build of the 2D engine. This is not a different engine — it is')
    w('the same design with the same public ABI, so a C game can link against')
    w('this build and a C++ game can link against the C one.')
    w('')
    w('```sh')
    w('make p1 USE_2D_PLUS=1     # link this engine')
    w('make p1                   # link the C engine')
    w('```')
    w('')
    w('```cpp')
    w('#include "sdk/2d_engine_plus/ng_engine_plus.hpp"')
    w('```')
    w('')
    w('## What this build adds')
    w('')
    w('Every function in [`API_2D_ENGINE_C.md`](./API_2D_ENGINE_C.md) exists')
    w('here unchanged, declared `extern "C"`. On top of them this build adds')
    w('member methods and singletons, so the same work reads as')
    w('`cam.follow(...)` and `CharManager::instance()` instead of a free')
    w('function plus an explicit pointer. The wrappers are `inline` and compile')
    w('to the same hardware writes — there is no runtime cost and no vtable.')
    w('')
    w('Compiled under `-std=c++14 -fno-exceptions -fno-rtti')
    w('-fno-threadsafe-statics -ffreestanding -march=68000`. No standard')
    w('library, no `malloc`, no floating point, and no global constructors that')
    w('depend on initialisation order.')
    w('')
    w('## Choosing between the builds')
    w('')
    w('| Build | Choose it when |')
    w('|---|---|')
    w('| `sdk/2d_engine` | You are writing C. Smallest code-size footprint and the simplest call chain. |')
    w('| `sdk/2d_engine_plus` | You are writing C++. You want member methods, singletons, and the scene-stack helpers. |')
    w('')
    w('One engine per ROM. `games/demo_plus` (id 778) is the smoke-test target')
    w('that links only against this build.')
    w('')
    w('---')
    w('')
    w('## Classes and methods')
    w('')
    w('Extracted from the headers by `tools/gen_api_reference.py`. Types with no')
    w('methods are plain data structures shared with the C build.')
    w('')

    files = sorted(f for f in os.listdir(CPP_DIR) if f.endswith('.hpp'))
    for f in files:
        stem = os.path.splitext(f)[0]
        cls = cpp_classes(os.path.join(CPP_DIR, f))
        if not cls:
            continue
        purpose = MODULES.get(stem, ('', '', ''))[1]
        w(f'### {stem}.hpp')
        w('')
        if purpose:
            w(purpose)
            w('')
        for name, meths in cls:
            w(f'**`{name}`**')
            w('')
            w('```cpp')
            for m in meths:
                w(m + ';')
            w('```')
            w('')

    w('---')
    w('')
    w('## Free functions')
    w('')
    w('The `extern "C"` surface is identical to the C build. See')
    w('[`API_2D_ENGINE_C.md`](./API_2D_ENGINE_C.md) for the full list; the')
    w('module layout, constants, and semantics all carry over unchanged.')
    w('')
    return '\n'.join(out) + '\n'


def main():
    os.chdir(ROOT)
    with open(os.path.join(DOC_DIR, 'API_2D_ENGINE_C.md'), 'w', encoding='utf-8') as fh:
        fh.write(emit_c())
    with open(os.path.join(DOC_DIR, 'API_2D_ENGINE_CPP.md'), 'w', encoding='utf-8') as fh:
        fh.write(emit_cpp())
    print('wrote docs/API_2D_ENGINE_C.md and docs/API_2D_ENGINE_CPP.md')
    return 0


if __name__ == '__main__':
    sys.exit(main())
