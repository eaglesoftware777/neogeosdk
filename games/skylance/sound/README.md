# Sky Lance sound

The numbered ADPCM-A effects are Sky Lance-specific procedural aviation and
weapon sounds. Regenerate their WAV sources, encoded bank and decoded diagnostic
previews with:

```sh
python3 games/skylance/tools/sfx.py
python3 games/skylance/tools/test_production.py
```

The ADPCM-B bank contains nine independent Sky Lance compositions rendered by
`python3 games/skylance/tools/music.py`: title, seven distinct stage scores,
and return-to-base/debrief. No Maiya ADPCM-B file is used. Tracks A through I
retain their existing driver IDs. WAV sources, 16 kHz playback metadata and
decoded diagnostic WAVs are included; `diagnostics/music.json` records hashes,
tempo, size, loudness and clipping checks. The unused voice bank remains legacy.
The complete independent FM/SSG soundtrack and continuous aircraft engine are
not implemented yet. See `THIRD_PARTY_AUDIO.md` for provenance and limitations.

```
samples/in_wav_a/     Reproducible original effect WAV sources
samples/out_a/        ADPCM-A SFX bank      (SOUND_SFX_1..16)
samples/out_a_voice/  ADPCM-A voice bank    (SOUND_VOICE_*)
samples/out_b/        ADPCM-B music tracks  (SOUND_TRACK_A..I)
fm/  mml/  ssg/       driver sources compiled into the M1 ROM
```

Voice WAV sources are not duplicated here. To rebuild the ROMs from
the encoded banks:

```
make GAME=skylance GAME_CFG_FILE=games/skylance/game.cfg vrom m1rom
```

The shared sound driver and BIOS protocol are unchanged. The original effects
retain IDs 1 through 16; Kira's primary uses 14 and Blaze's primary uses 15.
Normal shots are shorter than the firing interval and use the existing bounded
sound command queue. No engine loop or new raw driver command has been enabled.
