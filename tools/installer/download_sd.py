#!/usr/bin/env python3
"""Eagle Software - Flashcart & SD Card Image Downloader.

Downloads hardware-compatible ROM packages for:
1. NeoSD / NeoSD Pro (.neo images for MVS & AES)
2. Darksoft Neo Geo Multi (games folder format with interleaved C-ROMs)
3. BackBit Platinum (microSD folder format with raw MAME chip parts)

Directly from official GitHub releases (eaglesoftware777/neogeosdk)
with automatic fallback to local SDK build if available.
"""

import argparse
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys
import time
import urllib.request
import zipfile

GITHUB_RELEASES_API = "https://api.github.com/repos/eaglesoftware777/neogeosdk/releases"
REAL_WINDOWS = sys.platform == "win32"


def is_wsl():
    return "microsoft" in platform.uname().release.lower() and not REAL_WINDOWS


def get_default_sd_dir(base_dir=None):
    if base_dir:
        return Path(base_dir) / "sd_images"
    return Path(__file__).resolve().parent / "sd_images"


# Fallback when GitHub cannot be reached: this release's flashcart files.
# BEGIN KNOWN_ASSETS (tools/installer/make_wizard_zip.py rewrites this block)
KNOWN_ASSETS = {
    "version": "v1.7.3",
    "neosd": {
        "mvs": {
            "filename": "Maiya-WIP-NeoSD_MVS_v4.neo",
            "url": "https://github.com/eaglesoftware777/neogeosdk/releases/download/v1.7.3/Maiya-WIP-NeoSD_MVS_v4.neo",
            "size": 17567744,
            "desc": "NeoSD MVS Arcade flashcart image (ready to run on NeoSD / NeoSD Pro MVS)"
        },
        "aes": {
            "filename": "Maiya-WIP-NeoSD_AES_v4.neo",
            "url": "https://github.com/eaglesoftware777/neogeosdk/releases/download/v1.7.3/Maiya-WIP-NeoSD_AES_v4.neo",
            "size": 17567744,
            "desc": "NeoSD AES Console flashcart image (ready to run on NeoSD / NeoSD Pro AES)"
        }
    },
    "darksoft": {
        "mvs": {
            "filename": "Maiya-Darksoft-MVS_v4.zip",
            "url": "https://github.com/eaglesoftware777/neogeosdk/releases/download/v1.7.3/Maiya-Darksoft-MVS_v4.zip",
            "size": 9057719,
            "desc": "Darksoft Neo Geo Multi MVS folder (interleaved crom0)"
        },
        "aes": {
            "filename": "Maiya-Darksoft-AES_v4.zip",
            "url": "https://github.com/eaglesoftware777/neogeosdk/releases/download/v1.7.3/Maiya-Darksoft-AES_v4.zip",
            "size": 9057718,
            "desc": "Darksoft Neo Geo Multi AES folder (interleaved crom0)"
        }
    },
    "backbit": {
        "mvs": {
            "filename": "Maiya-BackBit-MVS_v4.zip",
            "url": "https://github.com/eaglesoftware777/neogeosdk/releases/download/v1.7.3/Maiya-BackBit-MVS_v4.zip",
            "size": 9121511,
            "desc": "BackBit Platinum MVS microSD folder (cartridge files)"
        },
        "aes": {
            "filename": "Maiya-BackBit-AES_v4.zip",
            "url": "https://github.com/eaglesoftware777/neogeosdk/releases/download/v1.7.3/Maiya-BackBit-AES_v4.zip",
            "size": 9121510,
            "desc": "BackBit Platinum AES microSD folder (cartridge files)"
        }
    }
}
# END KNOWN_ASSETS


def fetch_latest_sd_assets():
    """Queries GitHub for this game's flashcart assets across releases, with a fallback list."""
    headers = {"User-Agent": "EagleSoftwareFlashcartDownloader"}
    found_assets = {
        "version": KNOWN_ASSETS["version"],
        "neosd": {},
        "darksoft": {},
        "backbit": {}
    }

    try:
        req = urllib.request.Request(GITHUB_RELEASES_API, headers=headers)
        with urllib.request.urlopen(req, timeout=8) as resp:
            data = json.loads(resp.read().decode("utf-8"))
            for rel in data:
                tag = rel.get("tag_name", "")
                for a in rel.get("assets", []):
                    name = a.get("name", "")
                    url = a.get("browser_download_url", "")
                    size = a.get("size", 0)
                    if not name.startswith("Maiya-"):
                        continue        # a release can carry several games

                    # NeoSD (.neo)
                    if "NeoSD_MVS" in name and name.endswith(".neo") and "mvs" not in found_assets["neosd"]:
                        found_assets["neosd"]["mvs"] = {"filename": name, "url": url, "size": size, "desc": f"NeoSD MVS ({tag})"}
                    if "NeoSD_AES" in name and name.endswith(".neo") and "aes" not in found_assets["neosd"]:
                        found_assets["neosd"]["aes"] = {"filename": name, "url": url, "size": size, "desc": f"NeoSD AES ({tag})"}

                    # Darksoft (.zip)
                    if "Darksoft-MVS" in name and name.endswith(".zip") and "mvs" not in found_assets["darksoft"]:
                        found_assets["darksoft"]["mvs"] = {"filename": name, "url": url, "size": size, "desc": f"Darksoft Multi MVS ({tag})"}
                    if "Darksoft-AES" in name and name.endswith(".zip") and "aes" not in found_assets["darksoft"]:
                        found_assets["darksoft"]["aes"] = {"filename": name, "url": url, "size": size, "desc": f"Darksoft Multi AES ({tag})"}

                    # BackBit (.zip)
                    if "BackBit-MVS" in name and name.endswith(".zip") and "mvs" not in found_assets["backbit"]:
                        found_assets["backbit"]["mvs"] = {"filename": name, "url": url, "size": size, "desc": f"BackBit Platinum MVS ({tag})"}
                    if "BackBit-AES" in name and name.endswith(".zip") and "aes" not in found_assets["backbit"]:
                        found_assets["backbit"]["aes"] = {"filename": name, "url": url, "size": size, "desc": f"BackBit Platinum AES ({tag})"}

                if all(len(found_assets[k]) == 2 for k in ("neosd", "darksoft", "backbit")):
                    found_assets["version"] = tag
                    break
    except Exception as e:
        pass

    # Merge with KNOWN_ASSETS fallback
    for cat in ("neosd", "darksoft", "backbit"):
        for plat in ("mvs", "aes"):
            if plat not in found_assets[cat]:
                found_assets[cat][plat] = KNOWN_ASSETS[cat][plat]

    return found_assets


def download_asset_file(url, target_path, label, log_fn=print, progress_fn=None):
    log_fn(f"[DOWNLOAD] Downloading {label}...")
    headers = {"User-Agent": "EagleSoftwareFlashcartDownloader"}
    req = urllib.request.Request(url, headers=headers)
    target_path.parent.mkdir(parents=True, exist_ok=True)

    with urllib.request.urlopen(req, timeout=30) as resp, open(target_path, "wb") as f:
        total = int(resp.headers.get("Content-Length", 0))
        downloaded = 0
        last_pct = 0
        while True:
            chunk = resp.read(65536)
            if not chunk:
                break
            f.write(chunk)
            downloaded += len(chunk)
            if total > 0:
                pct = int((downloaded / total) * 100)
                if pct - last_pct >= 10:
                    mb_cur = downloaded / 1048576
                    mb_tot = total / 1048576
                    log_fn(f"  -> {pct}% ({mb_cur:.1f} MB / {mb_tot:.1f} MB)")
                    if progress_fn:
                        progress_fn(pct, f"Downloading {label} ({pct}%)")
                    last_pct = pct
    log_fn(f"[SUCCESS] Saved to: {target_path.name}")


def find_local_sdk_file(filename):
    """Checks if file is available in current or parent SDK dist/release directories."""
    candidates = [
        Path(__file__).resolve().parents[2] / "dist/release" / filename,
        Path("/mnt/c/neogeo/neogeosdk/dist/release") / filename,
        Path("C:/neogeo/neogeosdk/dist/release") / filename,
    ]
    for c in candidates:
        if c.is_file():
            return c
    return None


def write_instructions(out_dir):
    readme_path = out_dir / "README_FLASHCARTS.txt"
    content = """================================================================================
EAGLE SOFTWARE - MAIYA FLASHCART & SD CARD IMAGES
================================================================================

This directory contains hardware-ready images of "Maiya: Super Nature Girl"
formatted specifically for real Neo Geo flash cartridges and optical/SD drive systems.

1. NEOSD & NEOSD PRO (Folder: neosd/)
   ---------------------------------
   - Files: Maiya-WIP-NeoSD_MVS.neo / Maiya-WIP-NeoSD_AES.neo
   - HOW TO USE:
     Copy the .neo files directly to your microSD card (in root or your games/ folder).
     Insert the microSD card into your NeoSD or NeoSD Pro cartridge.
     Select Maiya from the on-screen NeoSD cartridge menu and press fire to flash & play!

2. DARKSOFT NEO GEO MULTI (Folder: darksoft/)
   ------------------------------------------
   - Folders: Maiya_MVS/ and Maiya_AES/
   - Files inside: prom, srom, m1rom, vroma0, crom0 (2-byte interleave), fpga
   - HOW TO USE:
     Copy the 'Maiya_MVS' or 'Maiya_AES' folder directly into the 'games/' directory
     on your Darksoft Neo Geo Multi SD card.
     Boot your arcade cabinet or console, scroll through the game selector menu,
     and load Maiya!

3. BACKBIT PLATINUM (Folder: backbit/)
   -----------------------------------
   - Folder: maiya/
   - Files inside: maiya.p1, maiya.s1, maiya.m1, maiya.v1, maiya.c1, maiya.c2
   - HOW TO USE:
     Copy the 'maiya/' folder directly to the root of your BackBit Platinum microSD card.
     The BackBit cartridge firmware will detect the game chips and boot natively.

Enjoy playing Maiya on genuine SNK Neo Geo hardware!
================================================================================
"""
    readme_path.write_text(content, encoding="utf-8")


def download_flashcart_images(formats=("neosd", "darksoft", "backbit"),
                             platforms=("mvs", "aes"),
                             out_dir=None,
                             extract_zips=True,
                             log_fn=print,
                             progress_fn=None):
    """Main function to download and organize flashcart images."""
    if out_dir is None:
        out_dir = get_default_sd_dir()
    else:
        out_dir = Path(out_dir)

    out_dir.mkdir(parents=True, exist_ok=True)
    log_fn(f"[INFO] Target SD images directory: {out_dir}")

    assets_catalog = fetch_latest_sd_assets()
    log_fn(f"[INFO] Using assets from release: {assets_catalog.get('version', 'latest')}")

    total_tasks = len(formats) * len(platforms)
    task_idx = 0

    for fmt in formats:
        if fmt not in assets_catalog:
            continue
        fmt_dir = out_dir / fmt
        fmt_dir.mkdir(parents=True, exist_ok=True)

        for plat in platforms:
            if plat not in assets_catalog[fmt]:
                continue
            task_idx += 1
            asset_info = assets_catalog[fmt][plat]
            fname = asset_info["filename"]
            url = asset_info["url"]
            dest_file = fmt_dir / fname

            if progress_fn:
                pct = int((task_idx / (total_tasks + 1)) * 100)
                progress_fn(pct, f"Processing {fmt.upper()} {plat.upper()}...")

            # 1. Check local SDK cache first
            local_src = find_local_sdk_file(fname)
            if local_src and local_src.is_file():
                log_fn(f"[LOCAL] Copying {fname} from local SDK release archive...")
                shutil.copy2(local_src, dest_file)
            else:
                # 2. Download from GitHub
                download_asset_file(url, dest_file, label=f"{fmt.upper()} ({plat.upper()})", log_fn=log_fn, progress_fn=progress_fn)

            # 3. Create friendly named aliases (e.g. Maiya-NeoSD-MVS.neo)
            if fmt == "neosd" and dest_file.suffix == ".neo":
                alias_name = f"Maiya-NeoSD-{plat.upper()}.neo"
                alias_file = fmt_dir / alias_name
                try:
                    if not alias_file.exists():
                        shutil.copy2(dest_file, alias_file)
                except Exception:
                    pass

            # 4. Extract zip files for Darksoft and BackBit if requested
            if extract_zips and dest_file.suffix == ".zip" and dest_file.is_file():
                log_fn(f"[EXTRACT] Unpacking {dest_file.name} for immediate SD card use...")
                try:
                    with zipfile.ZipFile(dest_file, "r") as z:
                        z.extractall(fmt_dir)
                    log_fn(f"  -> Extracted contents ready in {fmt_dir}")
                except Exception as e:
                    log_fn(f"  [WARN] Failed to unpack {dest_file.name}: {e}")

    write_instructions(out_dir)
    log_fn("[INFO] Created README_FLASHCARTS.txt hardware guide.")
    if progress_fn:
        progress_fn(100, "Flashcart and SD images downloaded successfully!")

    log_fn("\n============================================================")
    log_fn(" [SUCCESS] All requested flashcart images are ready!")
    log_fn(f" Location: {out_dir}")
    log_fn("============================================================\n")
    return True


def interactive_menu(out_dir=None):
    if out_dir is None:
        out_dir = get_default_sd_dir()

    while True:
        os.system("cls" if REAL_WINDOWS else "clear")
        print("============================================================")
        print("   Eagle Software - Flashcart & SD Card Image Downloader")
        print("   Maiya: Super Nature Girl (SNK Neo-Geo)")
        print("============================================================")
        print(f" Target folder: {out_dir}\n")
        print(" [1] Download NeoSD Images (.neo for MVS & AES)")
        print(" [2] Download Darksoft Multi Images (.zip + ready folder)")
        print(" [3] Download BackBit Platinum Images (.zip + ready folder)")
        print(" [4] Download ALL Flashcart Images (NeoSD, Darksoft, BackBit)")
        print(" [5] Open Target Folder in File Explorer")
        print(" [6] Exit")
        print("============================================================")

        choice = input("Select an option [1-6]: ").strip()
        if choice == "1":
            print("")
            download_flashcart_images(formats=["neosd"], platforms=["mvs", "aes"], out_dir=out_dir)
            input("\nPress Enter to continue...")
        elif choice == "2":
            print("")
            download_flashcart_images(formats=["darksoft"], platforms=["mvs", "aes"], out_dir=out_dir)
            input("\nPress Enter to continue...")
        elif choice == "3":
            print("")
            download_flashcart_images(formats=["backbit"], platforms=["mvs", "aes"], out_dir=out_dir)
            input("\nPress Enter to continue...")
        elif choice == "4":
            print("")
            download_flashcart_images(formats=["neosd", "darksoft", "backbit"], platforms=["mvs", "aes"], out_dir=out_dir)
            input("\nPress Enter to continue...")
        elif choice == "5":
            out_dir.mkdir(parents=True, exist_ok=True)
            if REAL_WINDOWS:
                os.startfile(str(out_dir))
            elif is_wsl():
                try:
                    subprocess.run(["explorer.exe", subprocess.check_output(["wslpath", "-w", str(out_dir)]).decode().strip()])
                except Exception:
                    pass
            else:
                subprocess.run(["xdg-open", str(out_dir)])
        elif choice in ("6", "q", "exit"):
            break


def main():
    parser = argparse.ArgumentParser(description="Download NeoSD, Darksoft, and BackBit Platinum images.")
    parser.add_argument("--format", choices=["neosd", "darksoft", "backbit", "all"], default="all",
                        help="Cartridge image format to download")
    parser.add_argument("--platform", choices=["mvs", "aes", "both"], default="both",
                        help="Neo Geo platform variant")
    parser.add_argument("--out", type=str, default=None,
                        help="Target output directory")
    parser.add_argument("--no-extract", action="store_true",
                        help="Do not unpack zip archives")
    parser.add_argument("--interactive", "-i", action="store_true",
                        help="Launch interactive console menu")
    args = parser.parse_args()

    out_path = Path(args.out) if args.out else get_default_sd_dir()

    if args.interactive or (len(sys.argv) == 1 and sys.stdin.isatty()):
        interactive_menu(out_path)
        return

    formats = ["neosd", "darksoft", "backbit"] if args.format == "all" else [args.format]
    platforms = ["mvs", "aes"] if args.platform == "both" else [args.platform]

    download_flashcart_images(
        formats=formats,
        platforms=platforms,
        out_dir=out_path,
        extract_zips=not args.no_extract
    )


if __name__ == "__main__":
    main()
