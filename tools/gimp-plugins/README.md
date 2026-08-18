# GIMP plug-ins — Artbox sprite prep

Manual GIMP pre-processing tools used before feeding source art into the
`artbox/` pipeline (`img2neo*.py`). Two sets, same three tools:

- `gimp2.10/` — GIMP 2.10.x Python-Fu (`gimpfu`, Python 2 style API).
- `gimp3.2/`  — GIMP 3.0+ Python-Fu (GObject-Introspection API, Python 3,
  pixel access via GEGL buffers instead of `gimpfu`'s pixel regions). Pure
  Python, same algorithm as the 2.10 versions - GIMP 3.2's bundled
  interpreter (3.14) has no numpy wheel published yet and no working C
  build toolchain to compile one from source, so this doesn't vectorise
  with numpy even though the GEGL buffer API would support it cleanly.
  If a numpy wheel for that Python version shows up later, the pixel
  loops in each `_process_*`/`_apply_mask` function are the place to
  swap in array operations.

## The three tools

1. **NeoGeo HD Pixel Enhance** — arcade contrast/saturation/gamma,
   conservative sharpen, optional snap to the Neo Geo's 5-bit/channel
   colour grid with a hue-preserving ordered dither, optional transparent
   RGB cleanup. Has a *target display* option (CRT / LCD-emulator) that
   changes the dither pattern and strength.
2. **Remove White Edge Noise** — erases visible white/near-white halo
   pixels touching transparency and decontaminates hidden white RGB
   under transparent and semi-transparent (anti-aliased) pixels.
3. **Smart Background Remove** — samples the border/corners, flood-fills
   only background connected to the image edge (so enclosed white design
   elements like eye highlights survive), expands into leftover edge
   pixels, then removes it with optional feather + defringe.

## What was fixed (white dots)

The GIMP 2.10 originals had two real bugs that could put a bright/white
speck on otherwise flat mid-tone art, and one gap that could leave a
white dot on a sprite's outline after export:

- **Pixel Enhance's ordered dither used a different Bayer cell for each
  of R, G and B**, so the three channels could independently round
  toward their bright extreme at the same pixel even over a flat area -
  a fully decorrelated colour speck with nothing to do with the source
  art. The offset formula could also swing a channel by several full
  5-bit steps (up to ~60 of 255 at the default settings, versus the ~4
  a proper half-step dither should ever move a channel). Fixed: one
  Bayer cell shared by all three channels (dithers luminance, preserves
  hue), offset capped to a real half of one 5-bit step.
- **Remove White Edge Noise's hidden-RGB cleanup only reached pixels
  that were already fully transparent, or within a small search radius
  of one.** A semi-transparent anti-aliasing ramp wider than that radius
  (common after the pipeline resizes/re-exports a sprite) kept its
  original near-white RGB under its partial alpha. GIMP shows nothing
  wrong, because the pixel is still semi-transparent - but once the Neo
  Geo pipeline thresholds alpha down to the hardware's binary
  transparent/opaque bit, any surviving pixel in that ramp becomes a
  fully opaque white dot on the sprite's edge. Fixed: any near-white
  pixel below a configurable semi-transparent ceiling gets its RGB
  decontaminated, independent of edge_radius.
- **Smart Background Remove's feathering reduced a foreground edge
  pixel's alpha without ever touching its RGB** unless that exact pixel
  also happened to be covered by the (smaller, fixed-radius) defringe
  pass. Fixed: RGB decontamination now covers the same pixel set that's
  about to be feathered, regardless of the defringe_edges toggle or how
  wide feather_radius is set.

## CRT vs LCD

Only Pixel Enhance needs this - it's the one tool that adds visible
dither noise on purpose. A real CRT's own phosphor bloom/scanline blur
smears a fine dither pattern into an even gradient; a modern LCD or
emulator shows every dithered pixel as a hard, fully visible dot with no
blur to hide it. So:

- **CRT** keeps the classic 4x4 Bayer matrix at a moderate default
  strength - the same territory the automated pipeline's
  `img2neo_crt.py` already leans on for its own CRT-oriented dither.
- **LCD / emulator** switches to an 8x8 Bayer matrix (much longer
  repeat period, far less visible as a regular checker) at a lower
  default strength, closer to the crisp/no-blur assumption
  `img2neo_hd.py` already makes for its blue-noise dither.

## Installing

**GIMP 2.10** - drop the three files from `gimp2.10/` directly into:

    <GIMP 2.10 profile>/plug-ins/

**GIMP 3.2** - each tool is a subdirectory whose name matches its script
(required by GIMP 3's plug-in browser - a bare .py file directly in
`plug-ins/` will not be found). Copy each subdirectory from `gimp3.2/`
into:

    <GIMP 3.2 profile>/plug-ins/

So you end up with e.g.
`<profile>/plug-ins/neogeo_hd_pixel_enhance_gimp3/neogeo_hd_pixel_enhance_gimp3.py`.
On Windows the script needs no chmod; GIMP's bundled interpreter runs it
directly. Restart GIMP (or use Filters > Script-Fu > Refresh Scripts /
the plug-in equivalent) to pick up new/changed plug-ins.

All three tools appear under **Filters > Enhance**.

## numpy in GIMP 3.2's bundled Python

`pip` is now installed into GIMP 3.2's bundled interpreter (it wasn't
before) at:

    C:\Users\kepler\AppData\Local\Programs\GIMP 3\bin\python.exe

`pip install numpy` on that interpreter still fails - PyPI has no
prebuilt numpy wheel for this Python build yet, and the source build
fails during its Meson/Cython configure step. That environment is also
PEP 668 "externally managed", so any future package install there needs
`--break-system-packages`. Worth retrying numpy again later once a wheel
exists for this Python version; until then the GIMP 3.2 plug-ins stay
pure Python.

## A note on the GIMP 3.2 versions

They're written against GIMP 3's documented `Gimp.PlugIn` / GEGL buffer
API and the core pixel math (dither, gamma/contrast/saturation, white
detection, RGB decontamination, background flood-fill) has been verified
against synthetic test images. The plug-in registration/UI shell itself
could not be exercised against a running GIMP from here - if a menu
entry doesn't show up or throws on first run, check the Error Console
(Filters > Script-Fu > Console works for the message log on Python-Fu
too) for the exact GI symbol it's tripping over.
