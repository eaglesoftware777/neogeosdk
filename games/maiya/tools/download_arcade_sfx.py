"""Download and convert authentic Kenney CC0 arcade adventure sound effects
to 16-bit mono 22050Hz WAV files for Maiya: Super Nature Girl.
"""

import subprocess
import urllib.request
from pathlib import Path

GAME = Path(__file__).resolve().parents[1]
DEST_DIR = GAME / "sound/samples/in_wav_a"
DEST_DIR.mkdir(parents=True, exist_ok=True)
TMP_DIR = Path("/tmp/maiya_sfx")
TMP_DIR.mkdir(parents=True, exist_ok=True)

SFX_MAP = {
    "1.wav": ("https://raw.githubusercontent.com/Boyquotes/kenney-rpg-audio-for-godot/main/addons/kenney%20rpg%20audio/knife_slice.ogg", "Rose whip slash"),
    "2.wav": ("https://raw.githubusercontent.com/Boyquotes/kenney-digital-audio-for-godot/main/addons/kenney%20digital%20audio/phase_jump_1.ogg", "Rose thorn projectile toss"),
    "3.wav": ("https://raw.githubusercontent.com/Boyquotes/kenney-impact-sounds-for-godot/main/addons/kenney%20impact%20sounds/impact_generic_light_001.ogg", "Enemy hit / squish"),
    "4.wav": ("https://raw.githubusercontent.com/Boyquotes/kenney-rpg-audio-for-godot/main/addons/kenney%20rpg%20audio/metal_latch.ogg", "Armor clank / shield deflect"),
    "5.wav": ("https://raw.githubusercontent.com/Boyquotes/kenney-digital-audio-for-godot/main/addons/kenney%20digital%20audio/pep_sound_2.ogg", "Toxic acid spit"),
    "6.wav": ("https://raw.githubusercontent.com/Boyquotes/kenney-digital-audio-for-godot/main/addons/kenney%20digital%20audio/laser_1.ogg", "Drone laser pulse"),
    "7.wav": ("https://raw.githubusercontent.com/Boyquotes/kenney-rpg-audio-for-godot/main/addons/kenney%20rpg%20audio/creak_1.ogg", "Buzzsaw / mechanical rev"),
    "8.wav": ("https://raw.githubusercontent.com/Boyquotes/kenney-digital-audio-for-godot/main/addons/kenney%20digital%20audio/low_down.ogg", "Flame burst / fire breath"),
    "9.wav": ("https://raw.githubusercontent.com/Boyquotes/kenney-impact-sounds-for-godot/main/addons/kenney%20impact%20sounds/impact_glass_medium_000.ogg", "Crystal / ice shatter"),
    "10.wav": ("https://raw.githubusercontent.com/Boyquotes/kenney-impact-sounds-for-godot/main/addons/kenney%20impact%20sounds/impact_bell_heavy_000.ogg", "Arcade explosion"),
    "11.wav": ("https://raw.githubusercontent.com/Boyquotes/kenney-rpg-audio-for-godot/main/addons/kenney%20rpg%20audio/handle_coins.ogg", "Gem / seed pickup chime"),
    "12.wav": ("https://raw.githubusercontent.com/Boyquotes/kenney-digital-audio-for-godot/main/addons/kenney%20digital%20audio/high_up.ogg", "Life heart restore chime"),
    "13.wav": ("https://raw.githubusercontent.com/Boyquotes/kenney-rpg-audio-for-godot/main/addons/kenney%20rpg%20audio/door_open_1.ogg", "Nature gate / door open"),
    "14.wav": ("https://raw.githubusercontent.com/Boyquotes/kenney-impact-sounds-for-godot/main/addons/kenney%20impact%20sounds/impact_bell_heavy_002.ogg", "Boss roar / alert"),
    "15.wav": ("https://raw.githubusercontent.com/Boyquotes/kenney-digital-audio-for-godot/main/addons/kenney%20digital%20audio/phase_jump_2.ogg", "Jump whoosh"),
    "16.wav": ("https://raw.githubusercontent.com/Boyquotes/kenney-impact-sounds-for-godot/main/addons/kenney%20impact%20sounds/impact_generic_light_003.ogg", "Player hurt impact"),
}

def main():
    print(f"Downloading and converting {len(SFX_MAP)} arcade adventure sound effects...")
    for filename, (url, desc) in SFX_MAP.items():
        ogg_path = TMP_DIR / f"{filename}.ogg"
        wav_path = DEST_DIR / filename
        req = urllib.request.Request(url, headers={"User-Agent": "Mozilla/5.0"})
        downloaded = False
        for attempt in range(5):
            try:
                with urllib.request.urlopen(req, timeout=10) as resp, open(ogg_path, "wb") as out_f:
                    out_f.write(resp.read())
                downloaded = True
                break
            except Exception as ex:
                import time
                time.sleep(1)
        if not downloaded:
            raise RuntimeError(f"Failed to download {filename} after 5 attempts")
        # Convert with ffmpeg to 16-bit mono 22050Hz WAV
        subprocess.run([
            "ffmpeg", "-y", "-i", str(ogg_path),
            "-ar", "22050", "-ac", "1", str(wav_path)
        ], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True)
        print(f"  [OK] {filename}: {desc} ({wav_path.stat().st_size} bytes)")
    print("All custom arcade adventure sound effects installed successfully!")

if __name__ == "__main__":
    main()
