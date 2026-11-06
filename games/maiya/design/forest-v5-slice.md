# Forest opening: V5 candidate

This is a first playable section, not the completed campaign redesign. It covers
world x64..1408 (about15–16 seconds in the scripted route checks). The longer
60–90 second encounter/pacing proof and later mission sections remain to be built.

The ground route keeps the established road, hazards and jumping controls. The
canopy route branches at the first vine and rejoins beyond the key platform.
The Sun Key remains reachable by its own vine, so clearing the optional boughs
is not required to open the gate. Briefing text now states the key/gate/guardian
objective instead of implying that every enemy must be killed.

| Beat | Authored coordinates | Purpose |
|---|---|---|
| Clearing | Start64; slime360 | Familiar movement and one readable ground threat |
| Root step |448..512, top144 | A low jump and an optional raised path |
| Vine entry |608..672, top112; vine640 | Access the upper route without a running jump |
| Reward bough |736..832, top88; silver icon800,56 | Reward taking the upper route |
| Crossing |944..1008, top120;1008..1072, top112 | A full jump followed by a short controlled hop |
| Key bough |1184..1312 drawn, top88; vine1216; key1224,60 | Rejoin and collect the required key |
| Recovery |1328..1408 | Return to the road before later encounters |

The first beetle moves from520 to640; the crow encounter at1080 is removed from
the climb. Other encounter identities and later stage geometry are retained.
Pickup/secret boxes are32px with top-left coordinates; their centres must actually
sit over the ledge they reward. Rendered ledges retain rounded32px caps and the
existing four-pixel support forgiveness.

The fixed nine-block ledge pool is unchanged. Early layouts that exceeded it were
rejected and shortened. No extra capacity or ignored placement finding was added.

Forest alone opts into `camera_feet_y:144`, with the existing maximum upper camera
range256. The view follows high jumps while leaving the primary HUD and hint row
above ordinary canopy movement. The camera bounds upward/downward lag and all
world sprites use the same existing snapshot. Other missions omit the option and
keep their previous vertical framing. No actor size, art, movement speed, jump,
collision, enemy HP, palette or sound data is changed by this section.

Review tools:

```
python3 -B games/maiya/tools/levels.py --lint --strict
python3 -B games/maiya/tools/review_levels.py --out /path/to/route-review
python3 games/maiya/tools/review_section.py --workspace /path/to/private-build \
    --platform mvs --hero maiya --route canopy --out /path/to/new-evidence
```

Section replays use normal controls, verify the matching ELF/P1 and actual chooser,
and require the key, exit, no damage/life loss and visible sprite rows. The canopy
check additionally requires standing on its upper bough, collecting its reward and
keeping the heroine canvas inside the configured safe screen area. This is useful
regression evidence; it does not replace human difficulty/motion review or physical
MVS/AES/NeoSD/BackBit acceptance.
