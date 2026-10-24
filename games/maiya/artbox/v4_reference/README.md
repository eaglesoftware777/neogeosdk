# Approved V4 art reference

The graphics and FIX banks remain in the parent artbox directory. These are
their approved palette/tile header and asset manifest, preserved byte for byte.
`lock.json` records the sizes and hashes of all five files.

The normal art target copies this approved set. It never renders, converts,
recolours, fits palettes or invokes an editor. `tools/locked_art.py --check`
rejects altered approved files or generated metadata. Do not regenerate the
reference or update its hashes as part of gameplay work.
