# Demo Audio Sources

The current demo samples are original synthesized arcade-style sounds made for this SDK test ROM. They are short, mono, and designed to survive YM2610 ADPCM conversion without noisy tails.

### ADPCM-A Samples (SFX)
Mapped to `playSFX(n)`.
- `1.adpcma`: Coin insert chime (Index 0).
- `2.adpcma`: Start slash and metal hit (Index 1).
- `3.adpcma`: Title gong (Index 2).
- `4.adpcma`: Intro taiko hit (Index 3).
- `5.adpcma`: Character footstep (Index 4).
- `6.adpcma`: Short voice shout (Index 5).
- `7.adpcma`: Blade whoosh (Index 6).
- `8.adpcma`: Impact hit (Index 7).
- `9.adpcma`: Plucked string phrase (Index 8).
- `10.adpcma`: Low drum accent (Index 9).
- `11.adpcma`: Ready voice cue (Index 10).
- `12.adpcma`: Attack voice cue (Index 11).

### ADPCM-B Samples (Music/Ambience)
Mapped to `playSFXB(n)`.
- `1.adpcmb`: Title theme (Index 0).
- `2.adpcmb`: Stage loop 1 /  TRACK (Index 1).
- `3.adpcmb`: Stage loop 2 / TRACK (Index 2).
- `4.adpcmb`: Ending / results scene theme (Index 3).

Current high-level mapping from `sdk/sound_ids.h`:

- `SOUND_TRACK_A = 0`
- `SOUND_TRACK_B = 1`
- `SOUND_TRACK_D = 3`
- `SOUND_TRACK_C = 2`

The tracked `out_a` and `out_b` files are the encoded ADPCM payloads used by `sound/tools/vrom.py` to build the V ROM.
