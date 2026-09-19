"""Process Juhani Junkala's CC0 sound effects and music for Maiya: Super Nature Girl.
Replaces the default demo sound with authentic arcade adventure audio.
"""

import os
from pathlib import Path
import subprocess

GAME = Path(__file__).resolve().parents[1]
ROOT = GAME.parents[1]

SFX_SRC = Path("/tmp/junkala_audio/sfx/The Essential Retro Video Game Sound Effects Collection [512 sounds] By Juhani Junkala")
ADV_SRC = Path("/tmp/junkala_audio/adv")
ACT_SRC = Path("/tmp/junkala_audio/act")

DEST_WAV_A = GAME / "sound/samples/in_wav_a"
DEST_WAV_B = GAME / "sound/samples/in_wav_b"
DEST_WAV_A.mkdir(parents=True, exist_ok=True)
DEST_WAV_B.mkdir(parents=True, exist_ok=True)

# 16 Sound Effects
SFX_PICKS = {
    "1.wav": SFX_SRC / "Weapons/Melee/sfx_wpn_sword1.wav",
    "2.wav": SFX_SRC / "Weapons/Single Shot Sounds/sfx_weapon_singleshot1.wav",
    "3.wav": SFX_SRC / "General Sounds/Impacts/sfx_sounds_impact1.wav",
    "4.wav": SFX_SRC / "Weapons/Melee/sfx_wpn_punch1.wav",
    "5.wav": SFX_SRC / "General Sounds/Weird Sounds/sfx_sound_depressurizing.wav",
    "6.wav": SFX_SRC / "Weapons/Lasers/sfx_wpn_laser1.wav",
    "7.wav": SFX_SRC / "General Sounds/Weird Sounds/sfx_sound_mechanicalnoise1.wav",
    "8.wav": SFX_SRC / "Explosions/Short/sfx_exp_short_hard1.wav",
    "9.wav": SFX_SRC / "General Sounds/High Pitched Sounds/sfx_sounds_high1.wav",
    "10.wav": SFX_SRC / "Explosions/Medium Length/sfx_exp_medium1.wav",
    "11.wav": SFX_SRC / "General Sounds/Coins/sfx_coin_cluster1.wav",
    "12.wav": SFX_SRC / "General Sounds/Positive Sounds/sfx_sounds_powerup1.wav",
    "13.wav": SFX_SRC / "Movement/Opening Doors/sfx_movement_dooropen1.wav",
    "14.wav": SFX_SRC / "Death Screams/Alien/sfx_deathscream_alien1.wav",
    "15.wav": SFX_SRC / "Movement/Jumping and Landing/sfx_movement_jump1.wav",
    "16.wav": SFX_SRC / "General Sounds/Simple Damage Sounds/sfx_damage_hit1.wav",
}

# 9 Music Tracks
MUSIC_PICKS = {
    "1.wav": ACT_SRC / "Juhani Junkala [Retro Game Music Pack] Title Screen.wav",
    "2.wav": ADV_SRC / "Juhani Junkala [Chiptune Adventures] 1. Stage 1.ogg",
    "3.wav": ADV_SRC / "Juhani Junkala [Chiptune Adventures] 2. Stage 2.ogg",
    "4.wav": ACT_SRC / "Juhani Junkala [Retro Game Music Pack] Level 1.wav",
    "5.wav": ACT_SRC / "Juhani Junkala [Retro Game Music Pack] Level 2.wav",
    "6.wav": ACT_SRC / "Juhani Junkala [Retro Game Music Pack] Level 3.wav",
    "7.wav": ADV_SRC / "Juhani Junkala [Chiptune Adventures] 4. Stage Select.ogg",
    "8.wav": ADV_SRC / "Juhani Junkala [Chiptune Adventures] 3. Boss Fight.ogg",
    "9.wav": ACT_SRC / "Juhani Junkala [Retro Game Music Pack] Ending.wav",
}

def main():
    print("== 1. Installing 16 Juhani Junkala Sound Effects ==")
    for name, src in SFX_PICKS.items():
        dst = DEST_WAV_A / name
        subprocess.run([
            "ffmpeg", "-y", "-i", str(src),
            "-ar", "22050", "-ac", "1", str(dst)
        ], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True)
        print(f"  [SFX] {name} <- {src.name} ({dst.stat().st_size} bytes)")

    print("== 2. Installing 9 Juhani Junkala Music Tracks ==")
    for name, src in MUSIC_PICKS.items():
        dst = DEST_WAV_B / name
        # Convert to 32000Hz mono WAV for ADPCM-B
        subprocess.run([
            "ffmpeg", "-y", "-i", str(src),
            "-ar", "32000", "-ac", "1", str(dst)
        ], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True)
        print(f"  [BGM] {name} <- {src.name} ({dst.stat().st_size} bytes)")

    print("\nAll Juhani Junkala audio files successfully prepared!")

if __name__ == "__main__":
    main()
