# Fighting component 1.7.7 candidate

This work is isolated from the SDK-wide 1.7.7 release. Only ng_fight.c/.h change
in the SDK. Other genre engines, BIOS sources and the root build system are
outside this candidate's source changes. Consumers must rebuild against the new
header; NGFMatch/NGFCpu layout additions are not a saved-state ABI guarantee.

`ngf_set_cpu_difficulty(match, side, rank)` accepts ranks 1–7; invalid values leave
state unchanged. Default rank 3 uses a 12-frame observed-attack reaction delay.
Reaction delays are 20, 16, 12, 10, 8, 6 and 4 simulation frames. Internal policy
levels are 1, 2, 4, 5, 6, 8 and 9. Legacy ngf_set_cpu remains available. Calibrated
rank survives round/team resets. CPU profiles learn projectile moves separately
from damaging melee moves. Replay hashes include CPU plans, roles and difficulty.

`ngf_set_round_seconds` accepts 10–99 and retains the selected duration through
round changes. Shadow League offers 30/60/90. Combo/damage counters saturate
instead of wrapping. Throws keep the victim aligned with the thrower during the
hold; NGF_ON_LIFT selects the higher lift. Release knockback follows the victim's
side, including reverse throws. Game move data chooses long and short launch values.

`NGFTraining` is caller-owned: initialize with ngf_training_init, select a dummy
with ngf_training_set_dummy, advance with ngf_training_step and reset with
ngf_training_reset. The eight dummy modes include human/CPU control and a bounded
240-input recording/playback buffer. Training prevents lethal damage, holds the
clock at 99 and optionally restores resources/life. It does not change normal
match knockout rules. The training object is independent of the match structure.

Shadow League's game-side changes include operator/AES options, practice UI,
command guides, original projectile definitions/effects and native artwork import.
The renderer owns three RAM pose packets and two hardware sprite banks per actor:
building, uploading and visible poses have distinct ownership. Only complete poses
become visible. Upload work is bounded by the existing drawing-window check. No
check was disabled to pass the candidate. Projectile/striker sprite reservations
are disjoint. Per-frame roots preserve wide poses without silently cutting limbs.

The current automated results establish deterministic gameplay, encoding integrity
and the measured emulator behavior. They do not establish visual approval, real
hardware compatibility, universal native-language voice coverage or resolution of
all player-reported pauses/flicker. Arena redesign and further movement/art polish
remain separate follow-up work. The parent coordination document records evidence,
installation paths, ownership and integration instructions.

Shadow League's linker places writable data in cleared NOLOAD RAM. Its new AES
settings therefore initialize defaults at runtime; writable C initializers are
not relied on. The MAME options gate checks initial level 3 / two wins / 60 seconds
before cycling difficulty. This is handled within the game's code; no common SDK
startup or linker file is changed.

The private hardware V1 packages are built with
`python3 games/shadowleague/tools/package_v1.py`. NeoSD region/lane reconstruction,
Darksoft deinterleaving and BackBit/MAME ZIP readback must preserve all six ROMs.
The program stays at 3 MiB with complete one-MiB banks, equal 8-MiB sprite lanes,
and power-of-two padded hardware sample regions. A larger mapper is not required
for this content. Physical hardware testing remains a separate acceptance step.
