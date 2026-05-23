# Sound Sample Sources

Provenance and licensing for the WAV samples that ship in `in_wav_a/`
and `in_wav_b/`.  This file is a manifest so contributors know which
samples are safe to redistribute under the SDK's MIT license and which
were created in-house.

## ADPCM-A (in_wav_a/) — trigger samples

| Channel use | File              | Source                                  | License        |
|-------------|-------------------|-----------------------------------------|----------------|
| Coin chime  | `1.wav`           | Generated, in-house (sine + decay)      | MIT (SDK)      |
| Start slash | `2.wav`           | Generated, in-house (noise burst)       | MIT (SDK)      |
| Title gong  | `3.wav`           | Generated, in-house (low FM hit)        | MIT (SDK)      |
| Taiko       | `4.wav`           | Generated, in-house (low-freq impulse)  | MIT (SDK)      |
| Footstep    | `5.wav`           | Generated, in-house (mid noise burst)   | MIT (SDK)      |
| Short shout | `6.wav`           | Generated, in-house (formant)           | MIT (SDK)      |
| Blade whoosh| `7.wav`           | Generated, in-house (filtered noise)    | MIT (SDK)      |
| Impact hit  | `8.wav`           | Generated, in-house (transient + decay) | MIT (SDK)      |
| String hit  | `9.wav`           | Generated, in-house (FM string approx.) | MIT (SDK)      |
| Low drum    | `10.wav`          | Generated, in-house (sub-bass burst)    | MIT (SDK)      |
| Ready voice | `11.wav`          | Generated, in-house (formant approx.)   | MIT (SDK)      |
| Attack voice| `12.wav`          | Generated, in-house (formant approx.)   | MIT (SDK)      |

## ADPCM-B (in_wav_b/) — streamed beds

| Bed name         | File     | Source                       | License   |
|------------------|----------|------------------------------|-----------|
| Title theme bed  | `1.wav`  | Generated, in-house          | MIT (SDK) |
| Stage one bed    | `2.wav`  | Generated, in-house          | MIT (SDK) |
| Stage two bed    | `3.wav`  | Generated, in-house          | MIT (SDK) |
| Ending theme bed | `4.wav`  | Generated, in-house          | MIT (SDK) |
| Eyecatcher bed   | `5.wav`  | Generated, in-house          | MIT (SDK) |

## Replacing a sample

1. Drop a 16-bit signed little-endian WAV into `in_wav_a/` (mono,
   18.5 kHz is the safe upper bound) or `in_wav_b/` (mono, 18.5 kHz)
2. `make samples && make vrom && make sound`
3. Add a row above documenting the source and license
