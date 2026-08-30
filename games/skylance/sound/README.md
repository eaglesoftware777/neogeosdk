# Sky Lance sound

The SFX and music banks are the SDK sample set shared with `games/demo`,
copied here in **encoded** form only:

```
samples/out_a/        ADPCM-A SFX bank      (SOUND_SFX_1..12)
samples/out_a_voice/  ADPCM-A voice bank    (SOUND_VOICE_*)
samples/out_b/        ADPCM-B music tracks  (SOUND_TRACK_A..I)
fm/  mml/  ssg/       driver sources compiled into the M1 ROM
```

The `in_wav_*` sources are deliberately **not** duplicated - they are 76 MB
and only `make sound` (the `samples` step) needs them.  To rebuild the ROMs
from the encoded banks, skip that step:

```
make GAME=skylance GAME_CFG_FILE=games/skylance/game.cfg vrom m1rom
```

Run the full `make ... sound` only after dropping fresh `samples/in_wav_a`,
`in_wav_a_voice` and `in_wav_b` WAVs in beside them.
