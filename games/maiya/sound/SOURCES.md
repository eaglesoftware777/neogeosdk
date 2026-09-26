# Maiya Audio Sources

Maiya has her own YM2610 bank: `game.mk` points the sound build at this
directory, so nothing in the ROM comes from the demo or any other game in the
SDK.

## Music (ADPCM-B, `samples/in_wav_b/`)

All nine tracks are by **Juhani Junkala**, released by him under the CC0
public-domain dedication, converted to 16-bit mono 32 kHz for the ADPCM-B
encoder:

| File | Track | Where it plays |
| --- | --- | --- |
| `1.wav` | Retro Game Music Pack - Title Screen | the hero select screen, the controls and the story |
| `2.wav` | Chiptune Adventures - Stage 1 | Mission 1, Emerald Forest |
| `3.wav` | Chiptune Adventures - Stage 2 | Mission 2, Valley of Sacred Falls |
| `4.wav` | Retro Game Music Pack - Level 1 | Mission 3, Azure Coral Coast; Mission 6, Sacred World Tree |
| `5.wav` | Retro Game Music Pack - Level 2 | Mission 4, Golden Autumn Grove |
| `6.wav` | Retro Game Music Pack - Level 3 | Mission 5, Crystal Grotto |
| `7.wav` | Chiptune Adventures - Stage Select | title screen, bonus rounds |
| `8.wav` | Chiptune Adventures - Boss Fight | every guardian, and Mission 7, Rio Negro Works |
| `9.wav` | Retro Game Music Pack - Ending | Sunboy between missions, the ending |

## Sound effects (ADPCM-A, `samples/in_wav_a/`)

The sixteen effects are taken from **Juhani Junkala**'s *The Essential Retro
Video Game Sound Effects Collection* (CC0), converted to 16-bit mono 22 kHz
and trimmed for the driver:

| File | Source effect | Used for |
| --- | --- | --- |
| `1.wav` | Weapons / Melee / sword 1 | rose whip crack |
| `2.wav` | Weapons / Single Shot / single shot 1 | thorn toss |
| `3.wav` | General / Impacts / impact 1 | creature hit, squash |
| `4.wav` | Weapons / Melee / punch 1 | guardian clank, thorn might |
| `5.wav` | General / Weird / depressurizing | slime spit |
| `6.wav` | Weapons / Lasers / laser 1 | drone pulse, elder's charm, gate |
| `7.wav` | General / Weird / mechanical noise 1 | saw |
| `8.wav` | Explosions / Short / hard 1 | guardian flame, mist veil |
| `9.wav` | General / High Pitched / high 1 | landing, forest friend |
| `10.wav` | Explosions / Medium / medium 1 | blast, death |
| `11.wav` | General / Coins / coin cluster 1 | coins, roses, seeds |
| `12.wav` | General / Positive / power-up 1 | hearts, flowers, extra life |
| `13.wav` | Movement / Opening Doors / door open 1 | Secret Art, key, gate, continue |
| `14.wav` | Death Screams / Alien / alien 1 | guardian roar |
| `15.wav` | Movement / Jumping / jump 1 | jump, dash, swiftness |
| `16.wav` | General / Damage / hit 1 | Maiya hurt |

## FM and SSG (`fm/`, `ssg/`, `mml/`)

The FM fanfare under the house eyecatcher (`fm/0_fm_example_a.mml`), the
two select-screen fanfares -- Maiya's (`fm/1_fm_example_b.mml`) and Luna's
(`fm/2_fm_example_c.mml`) -- and the remaining FM, SSG and MML tracks are
written for this game in the repository.
