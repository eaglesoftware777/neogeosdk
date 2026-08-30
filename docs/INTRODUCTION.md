# Introduction to the Neo Geo SDK

**Eagle Software · v1.7.0 · MIT**

This is the document to read first. It explains what the SDK is, what the
machine underneath it looks like, what you get in the box, and the order in
which the rest of the documentation is meant to be read.

---

## 1. What this SDK is

The Neo Geo SDK is a hardware-centered development kit for SNK Neo Geo
cartridge hardware — the MVS arcade board and the AES home console. It gives
you a 68000 cross-build, a Z80 sound stack, two asset pipelines, a 2D game
engine in both C and C++, six working example games, and an emulator test
loop that runs from `make`.

It deliberately does **not** hide the hardware.

On a Neo Geo, graphics, palettes, FIX tiles, ROM layout, and sound commands
are part of day-to-day programming rather than something an engine abstracts
away. That is the machine's actual programming model, and it is the reason
Neo Geo software looks and performs the way it does. The SDK preserves that
model and modernises everything around it: the build, the asset conversion,
the debugging, and the test cycle.

| The SDK **preserves** | The SDK **modernises** | The SDK **resists** |
|---|---|---|
| Direct register access, real VRAM/palette writes, hardware sprite budgets | Build tooling on Linux, WSL and Windows; Python asset pipelines; MAME-driven test loops; GDB | Heavy abstraction layers that hide what the hardware is doing |

## 2. The target machine

Three chips, one bus, no operating system.

| Part | Chip | Role |
|---|---|---|
| Main CPU | Motorola 68000 @ 12 MHz | Game logic, VRAM, palettes, sprites, DMA, FIX tiles |
| Sound CPU | Zilog Z80A @ 4 MHz | Runs the sound driver; talks to the YM2610 over command ports |
| Sound chip | Yamaha YM2610 | FM synthesis, SSG square waves, ADPCM-A and ADPCM-B sample playback |

### How a frame is drawn

A Neo Geo frame is **stacked, not composited**. Three fixed layers, fixed
priority:

1. **FIX layer** — a 40 × 32 cell grid of 8 × 8 tiles, of which **28 rows are
   visible**, in a fixed screen position. 15 colours per cell, 16 palettes,
   4096 tiles. This is the HUD and text plane, and it is always on top.
2. **Sprites** — up to **381 sprites** on screen, **96 per scanline**. Each
   sprite is a vertical strip of up to 32 tiles of 16 × 16 pixels, with
   per-sprite horizontal and vertical *reduction* and a chain ("sticky") bit
   that welds a strip to its left neighbour.
3. **Backdrop** — one solid colour behind everything, from the 65,536-colour
   pool. That is the entire background layer.

There is no tile-mapped background plane. Every moving and static element —
player, enemies, terrain, parallax — is built out of sprites. Sprite count
and the per-line limit are what drive your scene budget.

Two facts catch every newcomer:

- **Sprites can only shrink, never stretch.** The SCB2 scale field is a
  *reduction* factor. Art must be authored at its largest intended size.
- **Lower sprite slot number is drawn IN FRONT.** Putting a background at
  slot 1 puts it in front of your hero. Backgrounds belong at high slots.

### Colour

Colour is paletted, not direct. 15 colours per character (4 bits of index;
index 0 is transparent), 256 palettes of 16 entries in palette RAM at
`$400000–$401FFF`, giving 4,096 colours on screen chosen from 65,536.

A Neo Geo colour word is not plain RGB444. Bit 15 is the "dark" bit, bits
14–12 carry the least-significant bit of each channel, and bits 11–0 carry
the top 4 bits of R, G and B. The result is **5 bits per channel, 32 levels,
in steps of 8** — which is why the artbox quantiser snaps to that lattice
rather than to a 4-bit grid.

The **backdrop register is the last word of palette RAM**, at `$401FFE`.
`$402000` is a mirror that silently does nothing.

### Timing

Two interrupts, and that is the whole frame loop.

| IRQ | Source | Vector | Use |
|---|---|---|---|
| 1 | Vertical blank | `$64` | The game tick: read input, run logic, push the frame's sprite list into VRAM |
| 2 | LSPC raster timer | `$68` | Raster effects: split scrolls, mid-frame palette swaps, status bars |

VBlank is roughly 3 ms wide. Everything that writes VRAM from gameplay
should be queued and flushed inside it — which is what the engine's render
queue does for you.

### VRAM

You do not poke VRAM directly. You talk to the LSPC, and it talks to VRAM.

| Address | R/W | Register |
|---|---|---|
| `$3C0000` | W | VRAM address (16-bit word index) |
| `$3C0000` | R | Read data (address register unchanged) |
| `$3C0002` | W | VRAM write data — writing pushes to VRAM |
| `$3C0004` | R/W | Auto-increment, added to the address after every write |
| `$3C0006` | W | Mode register — timer / IRQ / auto-animation speed |
| `$3C0008` | W | Timer high |
| `$3C000A` | W | Timer low |
| `$3C000C` | W | Interrupt clear (bit 1 timer, bit 2 VBL) |

All access is word-aligned; long-word and byte access are not allowed. Wait
at least 1.4 µs (16 CPU clocks) after touching the address register before
reading. The SDK wraps all of this in named C helpers.

### The two CPUs talking

One byte at a time. The 68000 writes a sound code to `$320000`; the Z80
reads it on its own port and dispatches to the right music, SFX, FM or SSG
handler. The Z80 can post a status byte back through the same address.

You do not write that byte by hand — you call `playMusic()` / `playSFX()`
with a named id from `sdk/sound_ids.h`, the SDK encodes it, and the Z80
driver decodes it.

## 3. What is in the box

```
neogeosdk/
├─ sdk/                  68000 headers, runtime, and both 2D engines
│  ├─ 2d_engine/         the C engine   (ng_*.c / ng_*.h)
│  ├─ 2d_engine_plus/    the C++ engine (ng_*.cpp / ng_*.hpp), same ABI
│  ├─ ng_fix/ ng_audio/ ng_video/ ng_scene/ ng_show/ bsp/
│  ├─ neogeo.h macro.h   hardware registers and inline helpers
│  └─ sound_ids.h        named music / SFX / FM / SSG ids
├─ games/                one directory per game, each self-contained
│  ├─ demo/              the 25-chapter engine reel        (id 777)
│  ├─ demo_plus/         C++-engine smoke test             (id 778)
│  ├─ skylance/          Sky Lance, a complete shooter     (id 779)
│  ├─ helloworld/        minimal FIX-text + one sample     (id 772)
│  ├─ tutorial/          minimal engine-loop skeleton      (id 555)
│  └─ neogeogame/        empty starting point for your game(id 775)
├─ artbox/               PNG → sprite/FIX/C-ROM pipeline + Artbox Studio
├─ sound/                YM2610 stack: Z80 driver, MML, FM, SSG, samples
├─ docs/                 this documentation set
├─ tools/               validators and the manual generator
├─ dist/ roms/ hash_eagle/  build output and MAME hash sets
├─ Makefile             Linux / WSL build
└─ MakefileWin32.mak    native Windows build
```

## 4. The two engines

The SDK ships the same 2D engine twice: once in plain C
(`sdk/2d_engine/`), once in C++14 (`sdk/2d_engine_plus/`). They are not
different engines — they are the same design with the same public
`extern "C"` ABI, so a C game can link against the C++ build and vice versa.

| Build | Choose it when |
|---|---|
| `sdk/2d_engine` | You are writing C. Smallest code-size footprint, simplest call chain. This is the default. |
| `sdk/2d_engine_plus` | You are writing C++. You want member methods (`cam.follow(...)`), singletons (`CharManager::instance()`), and the scene-stack helpers. |

Pick one per ROM with the `USE_2D_PLUS` make flag:

```sh
make p1                 # link the C engine (default)
make p1 USE_2D_PLUS=1   # link the C++ engine
```

The C++ build compiles under `-std=c++14 -fno-exceptions -fno-rtti
-fno-threadsafe-statics -ffreestanding -march=68000`. No standard library,
no `malloc`, no floating point, no order-dependent global constructors.

Full call-by-call references:

- [`API_2D_ENGINE_C.md`](./API_2D_ENGINE_C.md)
- [`API_2D_ENGINE_CPP.md`](./API_2D_ENGINE_CPP.md)

## 5. Where to go next

| If you want to… | Read |
|---|---|
| Build your first ROM in ten minutes | [`HELLO_WORLD.md`](./HELLO_WORLD.md) |
| Understand the whole build, layer by layer | [`PROGRAMMERS_MANUAL.md`](./PROGRAMMERS_MANUAL.md) |
| Look up an engine call | [`API_2D_ENGINE_C.md`](./API_2D_ENGINE_C.md) / [`API_2D_ENGINE_CPP.md`](./API_2D_ENGINE_CPP.md) |
| Look up a bare-metal SDK call | [`../SDK_API_GUIDE.md`](../SDK_API_GUIDE.md) |
| Convert artwork | [`ARTBOX_PIPELINE.md`](./ARTBOX_PIPELINE.md) |
| Make sound | [`SOUND_DRIVER.md`](./SOUND_DRIVER.md), [`SOUND_STUDIO_GUIDE.md`](./SOUND_STUDIO_GUIDE.md) |
| Start a new game | [`ADDING_A_GAME.md`](./ADDING_A_GAME.md) |
| See what each shipped game demonstrates | [`GAMES.md`](./GAMES.md), [`DEMO_CHAPTERS.md`](./DEMO_CHAPTERS.md) |
| Stay inside the frame budget | [`performance_rules.md`](./performance_rules.md) |
| Look up a helper script | [`TOOLS.md`](./TOOLS.md) |
| Debug on real hardware timing | [`GDB_GUIDE.md`](./GDB_GUIDE.md) |

## 6. Licence and intended use

MIT. The SDK is intended for lawful homebrew development, technical
research, education, preservation-oriented study, and authorised consulting
work. It must not be used for unauthorised copying, distribution,
reproduction, sale, conversion, or operation of copyrighted commercial
software.
