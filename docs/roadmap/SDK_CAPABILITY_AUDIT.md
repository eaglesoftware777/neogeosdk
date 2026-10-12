# SDK capability audit (2026-10-12)

Baseline: branch `fix/maiya-arenas-eagle-bios`, NeoGeoSDK v1.7.5 (Framework
1.4), with the `ng_fight` work described in `docs/roadmap/V1.7.5_STATUS.md`.
Classes: **done** (implemented and verified), **partial** (implemented but
incomplete), **untested** (implemented, insufficiently tested), **missing**,
**hardware** (needs physical hardware), **deferred**, **infeasible**.

## EagleBIOS (v1.7.4 target)

| Capability | Class | Where / evidence |
|---|---|---|
| Boot, vectors, exception screen | done | `bios/src/bios_entry.s`, `bios_main.c`; `bios/test_bios.py` |
| USER requests, eye-catcher, title, fanfare | done | `bios/README.md` "The USER contract"; Maiya/Sky Lance boot in MAME |
| SYS_IO pads, start/select, coins, credits | done | `bios_io.c`; shared/separate chutes from DIP bit 1 |
| SYS_CREDIT_CHECK / DOWN | done | used by every SDK game's PLAYER_START |
| Message stream (SYS_MESS_OUT) | partial | streams 1-8, 10-13; Japanese common FIX (9) missing |
| Memory card (SYS_CARD) | partial | 8-bit cards; 16-bit cards missing (`bios_card.c`) |
| Backup RAM, high scores, soft DIPs | partial | `bios_backup.c`; per-game settings menu not exposed beyond region/free play |
| Real-time clock | missing | calendar answers a fixed date (`bios_rtc.c`) |
| Region / operating mode | partial | service-menu setting; firmware id word fixes the reported region |
| Multi-slot MVS | missing | one slot |
| Hardware diagnostics | partial | pad test, colour bars, sound codes; no RAM/VRAM/ROM tests |
| Cartridge verification (ROM checks) | missing | |
| HOWTOPLAY | missing | no-op |
| Jukebox / in-game menu / developer tools | missing | |
| Retail cartridge compatibility | hardware | only homebrew verified in MAME |
| Physical MVS/AES boards, flash carts | hardware | NeoSD, Darksoft, BackBit packages built; not verified on hardware |

## Fighting engine (v1.7.5 target)

| Capability | Class | Where / evidence |
|---|---|---|
| Two-player buffered input, motions, charge, 360, dash, SOCD | done | `ng_fight.c` input; `tests/fight_test.c` |
| Negative edge | partial | releases tracked (`NGFInput.released`); no move uses them yet |
| Data-driven fighters (ROM tables) | done | `NGFFighterDef`; Shadow League JSON cook |
| Fighter state machine, physics, auto-facing, push boxes | done | `fighter_step`, `resolve_space` |
| Hurt/hit boxes from art | done | `games/shadowleague/tools/fighter_art.py` |
| Guard levels, chip, guard break, throws and escapes | done | `apply_hit`, `start_move`; host tests |
| Cancels, chains, counter hits, counter stances, combo scaling | done | `try_moves`, `try_counter` |
| Power (3 stocks), guard, spirit meters | done | |
| Level 2/3 supers | partial | stock costs supported; Shadow League has level-1 supers only |
| Projectiles (spawn, travel, clash) | untested | engine and host test only; no Shadow League fighter uses one; game does not draw them |
| Rounds, timer, KO, time up, draw, perfect | done | MAME QA (`games/shadowleague/tools/qa.py`) |
| CPU opponent with levels | done | `ngf_cpu_input`; difficulty soft DIP |
| Fighting camera | partial | follow and clamp; no zoom |
| Hardware zoom | deferred | |
| FIX HUD (life, timer, names, power, wins, combo) | done | `slf_draw.c` |
| Team, striker, tag modes | missing | |
| Training mode, input display, replay record/playback | missing | determinism and `ngf_match_hash` are in place |
| Sound integration | done | events drive ADPCM-A effects; ADPCM-B music |
| Bank switching for large games | deferred | Shadow League P1 uses 24% of 384K |
| Physical hardware | hardware | |

## Existing SDK modules reused

`ng_sprite_group` (fighters, stage, effects, deferred VRAM), `ng_fix` (HUD),
`ng_palette_fx` (fades), artbox `palette_banks`/`tile_codec` (stage and C
ROM), `sound/tools/adpcm_enc` (effects and music), EagleBIOS packaging and
the MAME harness pattern from Maiya.
