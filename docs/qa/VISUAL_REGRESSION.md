# Visual Regression

Method: the same scripted input is played on two builds in the Linux MAME
(`games/skylance/tools/shot_probe.py`: coin, start, the pilot flying under
the nearest enemy and firing, invulnerable), snapshots are taken at the same
emulated times, and each pair is compared pixel by pixel (a channel change
above 24 counts). The difference image shows where the builds part; it is
supporting evidence, and a person decides whether a change is an
improvement. Game logic is deterministic for the same input, so differences
come from the builds; they grow over a run once one change (a shot fired or
held) shifts the rest of the game.

## Sky Lance: v2 release against the v2.1.0 candidate

`docs/qa/visual/skylance_v2_vs_v2.1.0.png`: release (left), candidate
(middle), changed pixels (right). Both builds were made from source today;
the release build is byte-identical to `SkyLance-WIP-EagleBIOS_v2.zip`.

| Time | Pixels changed | What differs |
|---|---|---|
| 22 s | 2.6% | the cloud bank: beside the plane in the release, gone in the candidate |
| 32 s | 6.8% | cloud; enemy fire pattern; a radar-lock label |
| 42 s | 6.9% | cloud; shots |
| 52 s | 7.4% | cloud; shots; a pickup |
| 62 s | 7.5% | cloud; shots |

The release column shows the reported defect plainly: the white cloud bank
sits beside the player's plane at 22, 32, 42, 52 and 62 seconds, because it
was projected with the distant-terrain weight and drifted with the plane.
The probe's numbers agree: the overhead cloud took **1** screen x position
in 648 frames on the release and **15** on the candidate while the camera
panned. Terrain, HUD layout, palettes and the aircraft art are unchanged
(identical pixels outside the cloud, shots and labels).

## Not yet compared

No Signal, Shadow League and Maiya & Luna have no art changes in this pass,
so there is nothing to compare yet. Their baseline snapshots come from the
boot checks (`C:\mame\neogeosdk\screenshots\<version>\boot_06s.png` and
`boot_26s.png`), which are the references for the art work to come.
