"""Download and install Juhani Junkala's CC0 arcade sound effects and music
for Maiya: Super Nature Girl.
"""

import io
import os
from pathlib import Path
import subprocess
import urllib.request
import zipfile

GAME = Path(__file__).resolve().parents[1]
TMP_DIR = Path("/tmp/junkala_audio")
TMP_DIR.mkdir(parents=True, exist_ok=True)

DEST_SFX_DIR = GAME / "sound/samples/in_wav_a"
DEST_SFX_DIR.mkdir(parents=True, exist_ok=True)

DEST_MUS_DIR = GAME / "sound/samples/in_wav_b"
DEST_MUS_DIR.mkdir(parents=True, exist_ok=True)

URL_SFX = "https://opengameart.org/sites/default/files/The%20Essential%20Retro%20Video%20Game%20Sound%20Effects%20Collection%20%5B512%20sounds%5D.zip"
URL_ADV = "https://opengameart.org/sites/default/files/Juhani%20Junkala%20%5BChiptune%20Adventures%5D%20OGG.zip"
URL_ACT = "https://opengameart.org/sites/default/files/5%20Action%20Chiptunes%20By%20Juhani%20Junkala.zip"

def download_and_extract(url, out_dir):
    print(f"Downloading {url} ...")
    req = urllib.request.Request(url, headers={"User-Agent": "Mozilla/5.0"})
    with urllib.request.urlopen(req, timeout=30) as resp:
        data = resp.read()
    print(f"  Downloaded {len(data)} bytes. Extracting to {out_dir} ...")
    with zipfile.ZipFile(io.BytesIO(data)) as zf:
        zf.extractall(out_dir)

def main():
    sfx_extracted = TMP_DIR / "sfx"
    adv_extracted = TMP_DIR / "adv"
    act_extracted = TMP_DIR / "act"

    if not sfx_extracted.exists():
        download_and_extract(URL_SFX, sfx_extracted)
    if not adv_extracted.exists():
        download_and_extract(URL_ADV, adv_extracted)
    if not act_extracted.exists():
        download_and_extract(URL_ACT, act_extracted)

    print("\nListing extracted directories:")
    for root, dirs, files in os.walk(TMP_DIR):
        rel = os.path.relpath(root, TMP_DIR)
        wav_count = len([f for f in files if f.endswith(('.wav', '.ogg'))])
        if wav_count > 0:
            print(f"  {rel}: {wav_count} audio files")

if __name__ == "__main__":
    main()
