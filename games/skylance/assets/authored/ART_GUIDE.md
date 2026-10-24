# Sky Lance native art guide

The player aircraft are original F-16, F-35 and F-15 inspired silhouettes on
48x48 canvases. The fixed aircraft root is (24,24), with a constant upward nose
axis. Rook's F-16 has one engine and blue identification panels; Kira's F-35 has
an angular broad fuselage, one engine and green panels; the F-15 has twin engine
pods, paired tails and orange panels. Angles are represented by individually
drawn wing projection, side faces and shading, while the heading stays fixed.

Light comes from the upper left. Metal uses connected dark/mid/light planes,
canopies use dark glass with one narrow cyan reflection, engines use amber and
cream. No noise, dithering, resampling or outline colours outside the stored
palette. Native Aseprite indices and embedded palette words survive the C-ROM
encode/decode unchanged. Every intact airframe has a transparent canvas margin.

The title is an opaque 320x224 background, with its own explicit full-canvas
coverage gate; it uses the native F-16's fifteen opaque colours. Editor reference
and shadow layers remain hidden on main sprite export. Shadow masks use the
existing bank-114 gray ramp and are projected into slots behind aircraft.

## Stage 1 — Mountain Dawn

A 352x4096 terrain document contains sixteen distinct sectors: dawn farms,
foothill hamlet, ridge AA apron, reservoir crossing, valley radar station,
terraced quarry, mountain command site, civil airfield, river industry, orchard
village, rail cutting, alpine pass, forward barracks, fortress switchback,
citadel outer works and Crimson Keep approach. Road traffic occupies world
X=92. Roofs, trees, towers and pads have separate parcels. The three military
pads remain at world X=144, at the established checkpoint rows.

Drones have compact swept red bodies; fighters have narrow metal noses and
wingtip weapons; helicopters have a distinct cabin, tail and fixed rotor hub;
tanks show separate tread assemblies and a central turret; bombers have four
engine pods and a broad delta hull; SAM batteries show three individual launch
rails; radar trucks have a dish with a support and vehicle base.

Ridge Warden is a dedicated 80x64 twin-rotor gunship with two nacelles, an armored
bridge, an intact central cabin and two forward cannon mounts. Its source is
`miniboss_0.aseprite`. Crimson Keep is a complete 112x112 airborne fortress with
engine bays, central armored hull, vent panels, cockpit glass and a warm core.
Two separately targetable 32x32 cannon modules recoil with the parent volley,
retain their own health and move with the hull.

The scene has small far clouds, separately drawn aircraft shadows and sparse
telegraphed foreground clouds. Critical bolts sort above cloud occlusion. The
camera has a sixteen-pixel dead zone, a twelve-pixel lateral bound, damped
recentering and zero vertical collision displacement. Ground objects receive
exactly the terrain's lateral offset.

## Production order

Stage 1 must pass native AES/MVS capture and physical SCB scanline checks before
Stage 2 naval authoring begins. Stage 2 then passes the same checks before the
remaining five stage families are authored. Stage 2–7 production remains open.
