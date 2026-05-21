# Added Assets

This directory contains additional assets for the updated example games:

* `artbox/in/characters/i_are_spaceship.png` – a 64x64 sprite sheet downloaded from OpenGameArt (CC0 licensed) containing a player ship, bullets, asteroids, explosions and enemies. It can be processed with the SDK's `make art` command to import it into the Neo Geo artbox. You may slice individual sprites or use the sheet as is.
* `sound/laser.wav` – a short 880 Hz beep generated as a placeholder laser shot sound effect.
* `sound/explosion.wav` – a white‑noise explosion effect that fades out.  Both audio files are 16‑bit WAVs at 22.05 kHz.

To integrate these assets into your game you will need to run the `make art` command in the game directory.  Refer to `artbox` documentation for how to use sprite sheets and sounds.  You can modify the code to play the sound effects via the SDK’s YM2610 sound API and assign the generated sprite tiles to your entities.
