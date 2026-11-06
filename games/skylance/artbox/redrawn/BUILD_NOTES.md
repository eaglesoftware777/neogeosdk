# Sky Lance art sources

The current terrain, miniature console instruments, radar zones, chaff and
mini bosses are authored with deterministic whole-pixel drawing instructions in
`../../tools/pixel_scenery.py`. Their route palette conversion preserves every
source pixel exactly before Neo Geo color encoding. No photographs, resampling
or filtering enter that terrain path. Enlarged route previews use nearest
neighbour and are excluded from C-ROM input.

Accepted aircraft, enemy, strike and title masters were produced with the
built-in image generation/editing tool, then imported once into native canvases
by `../../tools/redrawn.py`. Original source files remain unchanged. These
masters are separate from the deterministic Maiya walk/run task, which forbids
that tool. `terrain-routes-v2.png` and `cockpits-v2.png` are retained experiments;
the current game uses the authored pixel route/console pipeline instead.

Production prompt specifications:

- `aircraft-roll-v2.png`: three rows of the F-16, F-35 and F-15 in blue/silver,
  olive and charcoal/crimson. Seven columns at -45/-30/-15/0/+15/+30/+45 degrees.
  Nose stays vertically up; rigid airframe rolls around its nose-to-tail axis,
  without yaw. Transparent background, constant pivot and size per plane.
- `aircraft-maneuvers-v2.png`: matching aircraft, four side-by-side poses per row:
  90-degree edge, 180-degree belly, 270-degree opposite edge and steep climb.
  Preserve type silhouette, color scheme and central anchor.
- `combat-v2.png`: a 4x5 transparent military atlas. Drone, fighter, interceptor,
  gunship; missile boat, tank, bomber, SAM; radar truck, submarine, bunker,
  radar site; eight distinct tracers, shells, rockets and guided missiles.
  Separate compact readable objects; airborne opponents point downward.
- `strikes-v2.png`: three rows, four stages each, of blue F-16 precision strike,
  green F-35 missile barrage and orange F-15 cluster bombing. Military aircraft,
  munitions and impact explosions, with transparent margins.
- `title-v2.png`: preserve the squadron title composition, lettering and pilot
  identities; correct stray yellow facial pixels to natural skin shading and
  keep recognizable F-16/F-35/F-15 silhouettes.

Proportion references (official fact sheets):

- F-16: https://www.af.mil/About-Us/Fact-Sheets/Display/Article/104505/f-16-fighting-falcon/
- F-35A: https://www.af.mil/About-Us/Fact-Sheets/Display/Article/478441/f-35a-lightning-ii/
- F-15: https://www.af.mil/About-Us/Fact-Sheets/Display/Article/104501/f-15-eagle/source/f-15-eagle/
- F-16 cockpit reference: https://www.nationalmuseum.af.mil/Visit/Museum-Exhibits/Fact-Sheets/Display/Article/198021/f-16-sit-in-cockpit/

Flight handling and instrument layouts are arcade miniatures. They are not
certified aerodynamic models or exact full-scale cockpit reproductions.
