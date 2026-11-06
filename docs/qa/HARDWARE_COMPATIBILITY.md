# Hardware Compatibility

Statuses: **PASS**, **FAIL**, **NOT TESTED**, **BLOCKED**, **EXPERIMENTAL**.
A platform is PASS only with evidence from that platform; MAME results say
nothing about a cartridge or a console.

## Matrix (2026-10-28)

| Game (current release) | MAME | HBMAME | FBNeo | AES | MVS | NeoSD / Pro | BackBit | Darksoft | AES+ | Neo Geo CD |
|---|---|---|---|---|---|---|---|---|---|---|
| Sky Lance v2 | PASS (boot, MVS+AES) | set present, older v2 build | set present, older v2 build | NOT TESTED | NOT TESTED | **FAIL** (tester: image not shown correctly full screen, v1.7.7 build) | NOT TESTED | NOT TESTED | NOT TESTED | EXPERIMENTAL |
| Sky Lance v2.1.0 candidate | PASS (boot, shot probe) | entry prepared | entry prepared | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | not built |
| Maiya v4 | PASS (boot, MVS+AES) | set present, older v4 build | set present, another older v4 build | NOT TESTED | NOT TESTED (an MVS sound fix awaits a board) | NOT TESTED (confirmation requested in NEOSD_V171_TEST_IMAGES.md) | NOT TESTED | NOT TESTED | NOT TESTED | EXPERIMENTAL |
| Maiya & Luna v2 | PASS (boot, MVS+AES) | set present, p1 not ours | none | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | not built |
| No Signal v3 | PASS (boot, MVS+AES) | set present = v2 | none | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | EXPERIMENTAL (CD v3 package) |
| Shadow League v3 | PASS (boot, MVS+AES) | set present, older v3 build | set present, another older v3 build | NOT TESTED | NOT TESTED | NOT TESTED (cartridge compatibility awaits a tester) | NOT TESTED | NOT TESTED | NOT TESTED | not built |
| Shadow League v3.1.0 candidate | PASS (boot, MVS+AES duels) | entry prepared | entry prepared | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | not built |
| Seven Wonders v1 | PASS (boot, MVS+AES) | set present, older v1 build | none | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | not built |
| Mahjong Demo v1 | PASS (boot, MVS+AES) | set present, older v1 build | none | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | not built |
| SDK Demo v1.7.0 | PASS (boot, MVS) | set present = v1.7.0 asset | none | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | not built |

MAME column: the unattended boot check of this pass (`tools/boot_versions.py`,
Linux MAME 0.289, EagleBIOS): both CPUs kept executing, video memory and the
palette were written, MAME exited 0, MVS and AES sets. It does not judge
gameplay or sound; the snapshots in `C:\mame\neogeosdk\screenshots\` are for a
person to look at. HBMAME/FBNeo: whether the database has the set and which of
our builds it describes (`EXTERNAL_DATABASE_COMPATIBILITY.md`); no HBMAME or
FBNeo binary was run.

## Evidence from players and testers

| Report | Build | Status after this pass |
|---|---|---|
| "On NeoSD Pro the image is not shown correctly full screen" | Sky Lance v2 as published with v1.7.7 (HUD in columns 0-2 and 37-39) | The title-safe rebuild of 2026-10-26 keeps the HUD in columns 3-36 (x 24-295) and is in v2.0.1/v2.1.0; the cause is not confirmed on the cartridge. Needs a NeoSD Pro capture of v2.1.0 next to a MAME capture. |
| "Bullets are sometimes hard to see: with several enemies the picture flickers" | Sky Lance v2 | Enemy shots no longer hold the first sprites the LSPC drops; fire is held when a line nears 96; needs hardware confirmation. |
| "The white cloud follows the aircraft" | Sky Lance v2 | Fixed in v2.1.0 and verified in MAME (1 cloud position over 648 frames before, 15 after). |
| "Finished it on the first try" | Sky Lance v2 | Per-sector difficulty table in v2.1.0; needs playtesting. |
| Projectiles stuck on wings | Sky Lance v2 | Reproduced in MAME: with the plane held under each enemy, the v2 release had a shot standing still for 2999 frames; the v2.1.0 candidate none (`docs/qa/skylance-probe/`). Needs hardware confirmation. |

## What a hardware test should record

Board or console and revision, BIOS, cartridge (NeoSD model and firmware,
BackBit, Darksoft), the exact file name and its SHA-256 from
`releases/SHA256SUMS.txt`, cold and warm boot, video output (RGB, composite,
HDMI mod) and a photo or capture of the title, a sortie and the HUD corners.
