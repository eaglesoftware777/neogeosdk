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
| SYS_CREDIT_CHECK / DOWN | done | `bios_io.c`; used by every SDK game's PLAYER_START |
| Message stream (SYS_MESS_OUT) | done | `bios_video.c`; streams 1-13 including Japanese common FIX (stream 9) |
| Memory card (SYS_CARD) | done | `bios_card.c`; 8-bit & 16-bit card detection, save inspection, error buzzer |
| Backup RAM, high scores, soft DIPs | done | `bios_backup.c`; checksummed EGS2 format, corruption recovery, soft DIPs menu |
| Real-time clock | done | `bios_rtc.c`; uPD4990A serial read/write, leap year calculation, weekday formula |
| Region / operating mode | done | `bios_menu.c`; persisted settings, dynamic BIOS_ID reporting |
| Multi-slot MVS | done | `bios_main.c`; multi-slot detection and slot cycling stub |
| Hardware diagnostics | done | `bios_menu.c`; color bars, sound test, input test, calendar test, work RAM test |
| Cartridge verification (ROM checks) | done | `bios_menu.c`; cartridge info and ROM checksum verification |
| HOWTOPLAY | done | `bios_io.c`; animated 4-button arcade tutorial screen with countdown |
| BIOS test menu suite | done | `bios_menu.c`; 11-entry expanded BIOS menu (Settings, DIPs, Card, Diag, Sound, Input, Clock, Cart, Clear, Boot) |
| Retail cartridge compatibility | hardware | only homebrew verified in MAME |
| Physical MVS/AES boards, flash carts | hardware | NeoSD, Darksoft, BackBit packages built; not verified on hardware |

## 0xC00400 Service Jump Table Audit

| Address | Entry Symbol | Description | Input Registers | Output / Flags | RAM Effects | MVS vs AES | Test Status |
|---|---|---|---|---|---|---|---|
| `0xC00400` | `BIOS_ID` | Platform and region identifier word | None | None | Read-only constant | MVS: bit 7 clear; AES: bit 7 set | done (`test_bios.py`) |
| `0xC00402` | `bios_reset` | Cold / Warm boot reset entry point | None | None | Initializes BIOS work RAM (`0x10FD00..0x10FFFF`), VRAM, palettes | MVS checks test switch / cabinet DIPs; AES boots or shows card error | done (`test_bios.py`) |
| `0xC00408` | `bios_bus_err_wrap` | Bus error exception vector | Stack frame | None | Halts CPU, renders blue exception dump screen with PC/address/IR | Identical | done |
| `0xC0040E` | `bios_addr_err_wrap` | Address error exception vector | Stack frame | None | Halts CPU, renders red exception dump screen with PC/address/IR | Identical | done |
| `0xC00414` | `bios_illegal` | Illegal instruction trap vector | Stack frame | None | Halts CPU, renders red exception dump screen | Identical | done |
| `0xC0041A` | `bios_privilege` | Privilege violation trap vector | Stack frame | None | Halts CPU, renders red exception dump screen | Identical | done |
| `0xC00420` | `bios_trace` | Trace exception trap vector | Stack frame | None | Halts CPU, renders red exception dump screen | Identical | done |
| `0xC00426` | `bios_trap` | Generic unhandled trap vector | Stack frame | None | Halts CPU, renders red exception dump screen | Identical | done |
| `0xC0042C` | `bios_uninit` | Uninitialized interrupt handler | Stack frame | None | Halts CPU, renders red exception dump screen | Identical | done |
| `0xC00432` | `bios_spurious` | Spurious interrupt handler | Stack frame | None | Halts CPU, renders red exception dump screen | Identical | done |
| `0xC00438` | `SYS_INT1` | VBlank interrupt housekeeping (Level 1 IRQ) | None | None | Acks IRQ1 (`0x3C000C`), kicks watchdog (`0x300001`), increments `BIOS_FRAME_COUNTER`, updates coins/credits, reads RTC | MVS processes coin chutes 1 & 2; AES does not | done (`test_bios.py`) |
| `0xC0043E` | `SYS_INT2` | Timer / HBlank interrupt (Level 2 IRQ) | None | None | Acks IRQ2 (`0x3C000C`), updates line timer counter | Identical | done (`test_bios.py`) |
| `0xC00444` | `SYS_RETURN` | Game dispatcher returning to BIOS supervisor mode | None | None | Restores supervisor SP (`0x10F300`), maps BIOS vectors (`REG_SWPBIOS`), unprotects backup RAM, dispatches USER request | Identical | done |
| `0xC0044A` | `SYS_IO` | Controller & coin reader | D0 = Request | None | Reads joypads (`0x300000`, `0x340000`), computes edge transitions (`BIOS_STATCHANGE`), credit computation | MVS tracks coin counters; AES handles Select/Start mapping | done (`test_bios.py`) |
| `0xC00450` | `SYS_CREDIT_CHECK` | Coin credit verification | D0 = Player (0 or 1) | D0 = credits, CCR Carry: 0=OK, 1=No credit | None | MVS checks credit balance; AES returns 255 credits with Carry clear | done (`test_bios.py`) |
| `0xC00456` | `SYS_CREDIT_DOWN` | Decrement game credit | D0 = Player (0 or 1) | None | Decrements credit count in BIOS RAM `0x10FDB0..0x10FDB1`, outputs coin pulses | MVS decrements credits; AES no-op | done (`slf_game.c`, `test_bios.py`) |
| `0xC0045C` | `SYS_READ_CALENDAR` | Read uPD4990A real-time clock | None | None | Unpacks serial BCD clock into `0x10FD70..0x10FD76` (yr, mo, day, wkd, hr, min, sec) | MVS communicates with serial uPD4990A RTC; AES supports simulated clock | done (`test_bios.py`) |
| `0xC00462` | `SYS_SET_CALENDAR` | Set uPD4990A real-time clock | None | None | Serializes BCD from `0x10FD70..0x10FD76` into uPD4990A RTC chip | MVS programs physical/emulated RTC; AES no-op | done |
| `0xC00468` | `SYS_CARD` | Memory card read/write/format | D0 = command, D1 = game id/offset, A0 = buffer | CCR Carry: 0=OK, 1=Err | JEIDA/PCMCIA card access at `0x800000..0x8FFFFF` (8-bit and 16-bit) | Identical | done |
| `0xC0046E` | `SYS_CARD_ERROR` | Display memory card error modal | D0 = error code | None | Clears FIX layer, shows error prompt and plays buzzer audio cue | Identical | done |
| `0xC00474` | `SYS_HOWTOPLAY` | Interactive arcade tutorial screen | None | None | Renders 4-button joystick controls with color badges, animated countdown | Identical | done |
| `0xC0047A..BC` | `Reserved` | Reserved system dispatch slots | None | None | Immediate `rts` return | Identical | done |
| `0xC004C2` | `SYS_FIX_CLEAR` | Clear FIX layer text tiles | None | None | Clears 40x30 tile area of VRAM FIX window (`0x7000..0x74FF`) with tile 0x0020 | Identical | done |
| `0xC004C8` | `SYS_LSP_1ST` | Sprite allocator reset | None | None | Resets sprite control blocks SCB1/SCB2/SCB3/SCB4 in VRAM, clears active sprite count | Identical | done |
| `0xC004CE` | `SYS_MESS_OUT` | Text formatting stream renderer | A0 = message descriptor, D0 = stream id | None | Renders string to FIX layer using font streams 1-13 (including Japanese stream 9) | Identical | done (`test_bios.py`) |
| `0xC004D4` | `CONTROLLER_SETUP`| Controller hardware setup | None | None | Sets up controller reading masks and port configuration | Identical | done |
| `0xC004DA` | `SYS_INT2` | Secondary timer interrupt wrapper | None | None | Acknowledges IRQ2 | Identical | done |

## Fighting engine (v1.7.5 target)

| Capability | Class | Where / evidence |
|---|---|---|
| Two-player buffered input, motions, charge, 360, 720, dash, SOCD | done | `ng_fight.c` input; `tests/fight_test.c` |
| Negative edge | done | `ng_fight.c`; release detection and command buffer integration |
| Data-driven fighters (ROM tables) | done | `NGFFighterDef`; 7 fighters cooked into ROM: Kenji, Jack, Min-Jun, Batu, Amara, Niran, Atlas |
| Fighter state machine, physics, auto-facing, push boxes | done | `fighter_step`, `resolve_space` |
| Hurt/hit boxes from art | done | `games/shadowleague/tools/fighter_art.py`; anatomical joint model |
| Guard levels, chip, guard break, throws and escapes | done | `apply_hit`, `start_move`; host tests |
| Cancels, chains, counter hits, counter stances, combo scaling | done | `try_moves`, `try_counter` |
| Power (3 stocks), guard, spirit meters | done | `ng_fight.c`; FIX HUD in `slf_draw.c` |
| Super freeze | done | `ngf_step`; 30-frame freeze with backdrop darkening and `NGF_EV_SUPER_FREEZE` |
| Level 2/3 supers | done | `try_moves`; stock expenditure and super freeze trigger |
| Projectiles (spawn, travel, clash) | done | `ng_fight.c`; clash physics; rendered in `slf_draw.c` (`SPR_PROJ`) |
| Rounds, timer, KO, time up, draw, perfect | done | MAME QA (`games/shadowleague/tools/qa.py`) |
| CPU opponent with levels | done | `ngf_cpu_input`; difficulty soft DIP |
| Fighting camera & Hardware zoom | done | `ngf_camera_step`; hardware zoom scale computation (`m->zoom_scale` 192..255) |
| FIX HUD (life, timer, names, power, wins, combo, guard, spirit, striker) | done | `slf_draw.c` |
| Team, striker, tag modes | done | `ngf_match_set_mode`, `ngf_team_set`, `ngf_call_striker`, `ngf_tag_switch` |
| Sound integration | done | events drive ADPCM-A effects; ADPCM-B music |
| Arcade flow & continue screen | done | 10-second countdown with MVS credit down, 7-fighter ladder, P2 title join |
| Parallax stage presentation | done | 2-layer parallax stage (512x224 dojo interior, 512x160 dawn mountains) |
| Bank switching for large games | deferred | Shadow League P1 uses 32% of 512K |
| Physical hardware | hardware | emulator (MAME) verified; hardware validation pending |

## Existing SDK modules reused

`ng_sprite_group` (fighters, stage, effects, deferred VRAM), `ng_fix` (HUD),
`ng_palette_fx` (fades), artbox `palette_banks`/`tile_codec` (stage and C
ROM), `sound/tools/adpcm_enc` (effects and music), EagleBIOS packaging and
the MAME harness pattern from Maiya.
