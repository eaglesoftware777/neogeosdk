# Sound Sample Sources

Provenance and licensing for the WAV samples that ship in `in_wav_a/`
and `in_wav_b/`.  This file is a manifest so contributors know which
samples are safe to redistribute under the SDK's MIT license and which
were created in-house.

## ADPCM-A (in_wav_a/) — trigger samples

| File              |
|-------------------|
| `1.wav`           |
| `2.wav`           |
| `3.wav`           |
| `4.wav`           |
| `5.wav`           |
| `6.wav`           |
| `7.wav`           |
| `8.wav`           |
| `9.wav`           |
| `10.wav`          |
| `11.wav`          |
| `12.wav`          |

## ADPCM-B (in_wav_b/) — streamed TRACKs

| File     | 
|----------|-
| `1.wav`  | 
| `2.wav`  | 
| `3.wav`  | 
| `4.wav`  | 
| `5.wav`  | 
| `5.wav`  | 
| `6.wav`  | 
| `7.wav`  | 
| `8.wav`  | 
| `9.wav`  | 

## Replacing a sample

1. Drop a 16-bit signed little-endian WAV into `in_wav_a/` (mono,
   18.5 kHz is the safe upper bound) or `in_wav_b/` (mono, 18.5 kHz)
2. `make samples && make vrom && make sound`
3. Add a row above documenting the source and license
