#!/usr/bin/env python3
"""Eagle Software - Maiya: Super Nature Girl Online Updater.

Checks official GitHub releases (eaglesoftware777/neogeosdk),
compares installed version against latest remote release,
and downloads/installs updated ROM sets, EagleBIOS, and software lists.
"""

import argparse
import json
import os
from pathlib import Path
import shutil
import sys
import urllib.request
import zipfile

PARTS = ("p1", "s1", "m1", "v1", "c1", "c2")
GITHUB_API_RELEASES = "https://api.github.com/repos/eaglesoftware777/neogeosdk/releases?per_page=10"
GITHUB_LATEST_RELEASE = "https://api.github.com/repos/eaglesoftware777/neogeosdk/releases/latest"

NEOGEO_XML = """<?xml version="1.0"?>
<!DOCTYPE softwarelist PUBLIC "-//MAME//DTD Software List//EN" "https://raw.githubusercontent.com/mamedev/mame/master/src/mame/mame.dtd">
<softwarelist name="neogeo" description="SNK Neo-Geo cartridges">
    <!--
    ID-0780
    . NGM-780
    NEO-MVS PROG-G2 (SNK-9201) / NEO-MVS CHA 42G-2
    . NGH-780
    -->

    <software name="maiya">
        <description>NeoGeo SDK maiya v1.7.0 / Eagle Software</description>
        <year>2026</year>
        <publisher>Eagle Software</publisher>
        <info name="serial" value="NGM-780 (MVS), NGH-780 (AES)"/>
        <info name="release" value="20260914 (MVS), 20260914 (AES)"/>
        <info name="alt_title" value="Eagle SDK"/>
        <sharedfeat name="release" value="MVS,AES" />
        <sharedfeat name="compatibility" value="MVS,AES" />
        <part name="cart" interface="neo_cart">
            <feature name="slot" value="rom_fatfur2" />
            <dataarea name="maincpu" width="16" endianness="big" size="0x100000">
                <rom loadflag="load16_word_swap" name="780-p1.p1" offset="0x000000" size="0x080000" crc="5fe28a51" sha1="2d1481933b0ba9a9f0f0e48e6e046cdabbd79b35" />
            </dataarea>
            <dataarea name="fixed" size="0x020000">
                <rom offset="0x000000" size="0x020000" name="780-s1.s1" crc="5c7951e1" sha1="b0a45c2085b5ef49f7a197bf403f103be2f21f4f" />
            </dataarea>
            <dataarea name="audiocpu" size="0x020000">
                <rom offset="0x000000" size="0x020000" name="780-m1.m1" crc="ce7717bd" sha1="66f3ef28d76104adeed931bc2f6b65a5f5e4c4e8" />
            </dataarea>
            <dataarea name="ymsnd:adpcma" size="0x79ab00">
                <rom name="780-v1.v1" offset="0x000000" size="0x79ab00" crc="127b89b4" sha1="8ec212c59da5e650ed6978cf358e24220b5d33c3" />
            </dataarea>
            <dataarea name="sprites" size="0x800000">
                <rom loadflag="load16_byte" name="780-c1.c1" offset="0x000000" size="0x400000" crc="1ec8e364" sha1="9d2a88d32fa067dc4008e8da415772cb56b2c46f" />
                <rom loadflag="load16_byte" name="780-c2.c2" offset="0x000001" size="0x400000" crc="f324e7d4" sha1="68eafc9a5a51998fe02614dea7704c77ce597edf" />
            </dataarea>
        </part>
    </software>
</softwarelist>
"""


def get_game_dir():
    return Path(__file__).resolve().parent


def get_installed_version_info(game_dir):
    v_file = game_dir / "version.json"
    if v_file.is_file():
        try:
            return json.loads(v_file.read_text(encoding="utf-8"))
        except Exception:
            pass
    return {
        "version": "unknown",
        "release_name": "Maiya (Initial Install)",
        "tag": "v1.7.0"
    }


def save_installed_version_info(game_dir, info):
    v_file = game_dir / "version.json"
    try:
        v_file.write_text(json.dumps(info, indent=2), encoding="utf-8")
    except Exception as e:
        print(f"[WARN] Could not update version.json: {e}")


def fetch_github_releases():
    """Fetches releases list from GitHub API with graceful fallback."""
    releases = []
    headers = {"User-Agent": "EagleSoftwareMaiyaUpdater"}
    try:
        req = urllib.request.Request(GITHUB_API_RELEASES, headers=headers)
        with urllib.request.urlopen(req, timeout=10) as resp:
            data = json.loads(resp.read().decode("utf-8"))
            for r in data:
                tag = r.get("tag_name", "")
                name = r.get("name", tag)
                body = r.get("body", "")
                published_at = r.get("published_at", "")
                assets = r.get("assets", [])
                mvs_url = None
                aes_url = None
                for a in assets:
                    aname = a.get("name", "")
                    aurl = a.get("browser_download_url", "")
                    # Only this game's sets: a release can carry several games.
                    if aname.startswith("Maiya-WIP-EagleBIOS") and aname.endswith(".zip"):
                        mvs_url = aurl
                    if aname.startswith("Maiya-WIP-AES-EagleBIOS") and aname.endswith(".zip"):
                        aes_url = aurl
                if mvs_url or aes_url:
                    releases.append({
                        "tag": tag,
                        "name": name,
                        "description": body,
                        "published_at": published_at,
                        "mvs_url": mvs_url or "",
                        "aes_url": aes_url or "",
                        "html_url": r.get("html_url", "")
                    })
    except Exception as e:
        print(f"[WARN] GitHub API request failed: {e}")

    if not releases:
        # Fallback known official releases
        releases = [
            {
                "tag": "v1.7.3",
                "name": "NeoGeoSDK v1.7.3 WIP pre-release: Maiya v4",
                "description": "Her walk and run redrawn; Sky Lance v1 in the same release.",
                "published_at": "2026-10-12",
                "mvs_url": "https://github.com/eaglesoftware777/neogeosdk/releases/download/v1.7.3/Maiya-WIP-EagleBIOS_v4.zip",
                "aes_url": "https://github.com/eaglesoftware777/neogeosdk/releases/download/v1.7.3/Maiya-WIP-AES-EagleBIOS_v4.zip",
                "html_url": "https://github.com/eaglesoftware777/neogeosdk/releases/tag/v1.7.3"
            },
            # BEGIN OLDER RELEASES (sets of this game only)
            {
                "tag": "v1.7.2",
                "name": "NeoGeoSDK v1.7.2 WIP pre-release: Maiya v3",
                "description": "EagleBIOS v3, full locomotion and commercial sound updates.",
                "published_at": "2026-09-14",
                "mvs_url": "https://github.com/eaglesoftware777/neogeosdk/releases/download/v1.7.2/Maiya-WIP-EagleBIOS_v3.zip",
                "aes_url": "https://github.com/eaglesoftware777/neogeosdk/releases/download/v1.7.2/Maiya-WIP-AES-EagleBIOS_v3.zip",
                "html_url": "https://github.com/eaglesoftware777/neogeosdk/releases/tag/v1.7.2"
            },
            {
                "tag": "v1.7.1",
                "name": "NeoGeoSDK v1.7.1 WIP Milestone Maiya EagleBIOS",
                "description": "EagleBIOS v2 milestone release.",
                "published_at": "2026-09-01",
                "mvs_url": "https://github.com/eaglesoftware777/neogeosdk/releases/download/v1.7.1/Maiya-WIP-EagleBIOS_v2.zip",
                "aes_url": "https://github.com/eaglesoftware777/neogeosdk/releases/download/v1.7.1/Maiya-WIP-AES-EagleBIOS_v2.zip",
                "html_url": "https://github.com/eaglesoftware777/neogeosdk/releases/tag/v1.7.1"
            }
            # END OLDER RELEASES
        ]

    return releases


def download_file(url, target_path, label="Release Package"):
    print(f"[DOWNLOAD] Downloading {label}...")
    headers = {"User-Agent": "EagleSoftwareMaiyaUpdater"}
    req = urllib.request.Request(url, headers=headers)
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
                    mb_cur = downloaded // 1048576
                    mb_tot = total // 1048576
                    print(f"  -> {pct}% ({mb_cur} MB / {mb_tot} MB)")
                    last_pct = pct
    print(f"[DOWNLOAD] {label} downloaded successfully.")


def package_root_zip(source_dir, output_zip):
    if not source_dir.is_dir():
        return
    with zipfile.ZipFile(output_zip, "w", zipfile.ZIP_DEFLATED) as z:
        for f in source_dir.iterdir():
            if f.is_file():
                z.write(f, arcname=f.name)


def deploy_release_zip(zip_path, game_dir, platform, log_fn=print):
    """Unpack a release's MAME zip into the install layout, one platform at a time:
    the MVS set in roms/maiya with hash/, the AES set in tests/aes/roms/maiya with
    tests/aes/hash/ (the two share file names, so they must not share folders),
    EagleBIOS in roms/neogeo or roms/aes, and the AES blank card as its memory card."""
    game_roms = game_dir / "roms/maiya" if platform == "mvs" else game_dir / "tests/aes/roms/maiya"
    hash_file = game_dir / "hash/neogeo.xml" if platform == "mvs" else game_dir / "tests/aes/hash/neogeo.xml"
    docs = game_dir / "docs"
    with zipfile.ZipFile(zip_path) as z:
        for info in z.infolist():
            if info.is_dir():
                continue
            name = info.filename
            if name.startswith("roms/maiya/"):
                dest = game_roms / Path(name).name
            elif name.startswith("roms/"):
                dest = game_dir / name
            elif name == "hash/neogeo.xml":
                dest = hash_file
            elif name == "card.bin":
                dest = game_dir / "tests/aes/memcard.bin"
                if dest.exists():
                    continue
            else:
                dest = docs / Path(name).name
            dest.parent.mkdir(parents=True, exist_ok=True)
            dest.write_bytes(z.read(info))
    if platform == "mvs" and hash_file.exists():
        (game_dir / "hash/maiya").mkdir(parents=True, exist_ok=True)
        (game_dir / "hash/maiya/neogeo.xml").write_bytes(hash_file.read_bytes())
    log_fn(f"[INSTALL] {platform.upper()} set unpacked into its own folders.")


def apply_update(game_dir, release_info):
    print(f"\n[UPDATE] Applying release: {release_info['tag']} ({release_info['name']})")
    roms_dir = game_dir / "roms"
    roms_dir.mkdir(parents=True, exist_ok=True)
    tests_aes_roms = game_dir / "tests/aes/roms"
    tests_aes_roms.mkdir(parents=True, exist_ok=True)

    mvs_url = release_info.get("mvs_url")
    aes_url = release_info.get("aes_url")

    # 1. Download & Deploy MVS Package
    if mvs_url:
        mvs_tmp = game_dir / "update_mvs_tmp.zip"
        try:
            download_file(mvs_url, mvs_tmp, label="Maiya MVS Arcade Package")
            print("[EXTRACT] Extracting MVS files...")
            deploy_release_zip(mvs_tmp, game_dir, "mvs")
        finally:
            if mvs_tmp.exists():
                mvs_tmp.unlink()

    # 2. Download & Deploy AES Package
    if aes_url:
        aes_tmp = game_dir / "update_aes_tmp.zip"
        try:
            download_file(aes_url, aes_tmp, label="Maiya AES Console Package")
            print("[EXTRACT] Extracting AES files...")
            deploy_release_zip(aes_tmp, game_dir, "aes")
        finally:
            if aes_tmp.exists():
                aes_tmp.unlink()

    # 3. Ensure zip sets have files at archive root
    print("[PACKAGE] Rebuilding clean root-level zip sets for MAME...")
    package_root_zip(roms_dir / "maiya", roms_dir / "maiya.zip")
    package_root_zip(roms_dir / "neogeo", roms_dir / "neogeo.zip")
    package_root_zip(roms_dir / "aes", roms_dir / "aes.zip")
    package_root_zip(tests_aes_roms / "maiya", tests_aes_roms / "maiya.zip")

    # 4. Hash definitions: each set's own came with it; the built-in list is
    # only a fallback for an install that has none.
    hash_dir = game_dir / "hash"
    hash_dir.mkdir(parents=True, exist_ok=True)
    if not (hash_dir / "neogeo.xml").exists():
        xml_bytes = NEOGEO_XML.encode("utf-8")
        (hash_dir / "neogeo.xml").write_bytes(xml_bytes)
        (hash_dir / "maiya").mkdir(parents=True, exist_ok=True)
        (hash_dir / "maiya/neogeo.xml").write_bytes(xml_bytes)

    # 5. Update version info
    new_info = {
        "version": release_info.get("tag", "latest"),
        "release_name": release_info.get("name", "Maiya: Super Nature Girl"),
        "tag": release_info.get("tag", "latest"),
        "updated_at": release_info.get("published_at", "")
    }
    save_installed_version_info(game_dir, new_info)

    print(f"\n============================================================")
    print(f" [SUCCESS] Maiya updated successfully to {release_info['tag']}!")
    print(f"============================================================\n")


def check_and_update(check_only=False, auto_yes=False, force=False):
    game_dir = get_game_dir()
    installed = get_installed_version_info(game_dir)
    cur_version = installed.get("tag") or installed.get("version") or "unknown"
    cur_name = installed.get("release_name", "Maiya: Super Nature Girl")

    print("============================================================")
    print(" Eagle Software - Maiya: Super Nature Girl Online Updater")
    print("============================================================")
    print(f" Installation directory : {game_dir}")
    print(f" Current version        : {cur_version} ({cur_name})")
    print(" Checking GitHub for releases...")

    releases = fetch_github_releases()
    if not releases:
        print("[ERROR] Could not retrieve release information from GitHub.")
        return 1

    latest = releases[0]
    lat_version = latest.get("tag", "")
    lat_name = latest.get("name", "")

    print(f" Latest GitHub release  : {lat_version} ({lat_name})")
    print("============================================================")

    is_newer = (cur_version != lat_version) and (cur_version != "unknown")

    if check_only:
        if is_newer:
            print(f"[NOTICE] A new release is available: {lat_version}")
            return 2
        else:
            print("[OK] Your installation is up-to-date.")
            return 0

    if not is_newer and not force:
        print("\n[OK] You already have the latest version installed!")
        if not auto_yes:
            ans = input("Would you like to re-download / repair game files anyway? [y/N]: ").strip().lower()
            if ans != "y" and ans != "yes":
                print("No changes made. Have fun playing Maiya!")
                return 0
        else:
            return 0

    if is_newer:
        print(f"\n[NOTICE] An update is available: {cur_version} -> {lat_version}")
        if latest.get("description"):
            desc_lines = latest["description"].strip().split("\n")[:5]
            print("\nRelease Highlights:")
            for line in desc_lines:
                print(f"  {line}")
            print("")

    if not auto_yes:
        ans = input(f"Proceed with updating to {lat_version}? [Y/n]: ").strip().lower()
        if ans in ("n", "no"):
            print("Update cancelled by user.")
            return 0

    apply_update(game_dir, latest)
    return 0


def handle_sd_download(fmt="all", plat="both", out_dir=None):
    try:
        from download_sd import download_flashcart_images, get_default_sd_dir
    except ImportError:
        import sys
        sys.path.insert(0, str(Path(__file__).resolve().parent))
        from download_sd import download_flashcart_images, get_default_sd_dir

    if out_dir is None:
        out_dir = get_default_sd_dir(get_game_dir())

    formats = ["neosd", "darksoft", "backbit"] if fmt == "all" else [fmt]
    platforms = ["mvs", "aes"] if plat == "both" else [plat]

    print("\n============================================================")
    print(" Eagle Software - Flashcart & SD Card Image Downloader")
    print("============================================================")
    download_flashcart_images(formats=formats, platforms=platforms, out_dir=out_dir)
    return 0


def main():
    parser = argparse.ArgumentParser(description="Maiya Online Game Updater & Flashcart Downloader")
    parser.add_argument("--check", action="store_true", help="Check for updates without downloading")
    parser.add_argument("-y", "--yes", action="store_true", help="Automatically accept update prompts")
    parser.add_argument("-f", "--force", action="store_true", help="Force update even if versions match")
    parser.add_argument("--download-sd", action="store_true", help="Download NeoSD, Darksoft, and BackBit SD images")
    parser.add_argument("--sd-format", choices=["neosd", "darksoft", "backbit", "all"], default="all",
                        help="Cartridge image format to download")
    parser.add_argument("--sd-platform", choices=["mvs", "aes", "both"], default="both",
                        help="Platform variant to download")
    parser.add_argument("--sd-out", type=str, default=None, help="Target folder for SD images")
    args = parser.parse_args()

    if args.download_sd:
        ret = handle_sd_download(fmt=args.sd_format, plat=args.sd_platform, out_dir=args.sd_out)
        sys.exit(ret)

    ret = check_and_update(check_only=args.check, auto_yes=args.yes, force=args.force)
    sys.exit(ret)


if __name__ == "__main__":
    main()

