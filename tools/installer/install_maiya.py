#!/usr/bin/env python3
"""Eagle Software - Maiya: Super Nature Girl Setup Wizard & Engine.

Cross-platform graphical installer (Windows and Linux).
Installs Maiya MVS and AES builds with EagleBIOS, detects or downloads MAME
from official release pages, configures run scripts, and creates Desktop shortcuts
with custom Maiya icons.
"""

import argparse
import base64
import http.server
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys
import threading
import time
import urllib.request
import webbrowser
import zipfile

ROOT = Path(__file__).resolve().parents[2]
INSTALLER_DIR = Path(__file__).resolve().parent
PARTS = ("p1", "s1", "m1", "v1", "c1", "c2")
EAGLE = {
    "neogeo": ("sp-s2.sp1", "sm1.sm1", "sfix.sfix", "000-lo.lo"),
    "aes": ("neo-epo.bin", "000-lo.lo")
}

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
                <rom loadflag="load16_byte" name="780-c1.c1" offset="0x000000" size="0x400000" crc="48638505" sha1="b2517c2a09efe0b196335c616ba3b2f13969f052" />
                <rom loadflag="load16_byte" name="780-c2.c2" offset="0x000001" size="0x400000" crc="66a5c0e2" sha1="9f3d77f2284925c82d8e26f725243083a73d6d92" />
            </dataarea>
        </part>
    </software>
</softwarelist>
"""

IS_WINDOWS = sys.platform == "win32" or "microsoft" in platform.uname().release.lower()
REAL_WINDOWS = sys.platform == "win32"


def is_wsl():
    return "microsoft" in platform.uname().release.lower() and not REAL_WINDOWS


def get_windows_userprofile():
    if REAL_WINDOWS:
        return os.environ.get("USERPROFILE", "C:\\")
    if is_wsl():
        try:
            out = subprocess.check_output(
                ["/mnt/c/Windows/System32/cmd.exe", "/c", "echo %USERPROFILE%"],
                stderr=subprocess.DEVNULL
            ).decode().strip()
            if len(out) >= 2 and out[1] == ':':
                drive = out[0].lower()
                rel = out[2:].replace('\\', '/')
                return f"/mnt/{drive}{rel}"
        except Exception:
            pass
    return str(Path.home())


def detect_mame():
    """Detect MAME binary on host system (Windows or Linux)."""
    if REAL_WINDOWS or is_wsl():
        win_candidates = [
            "C:\\mame\\mame.exe",
            "C:\\mame\\mame64.exe",
            "C:\\Games\\mame\\mame.exe",
            "C:\\Program Files\\mame\\mame.exe",
            "C:\\Program Files (x86)\\mame\\mame.exe"
        ]
        if is_wsl():
            win_candidates = [
                "/mnt/c/mame/mame.exe",
                "/mnt/c/mame/mame64.exe",
                "/mnt/c/Games/mame/mame.exe",
                "/mnt/c/Program Files/mame/mame.exe"
            ]
        for c in win_candidates:
            if os.path.isfile(c):
                return True, c
    
    for cmd in ("mame", "mame64"):
        p = shutil.which(cmd)
        if p and os.path.isfile(p):
            return True, p
    
    if os.path.isfile("/usr/games/mame"):
        return True, "/usr/games/mame"

    return False, ""


def get_default_install_dir():
    if REAL_WINDOWS:
        return "C:\\Games\\Maiya"
    if is_wsl():
        return "/mnt/c/Games/Maiya"
    return str(Path.home() / "Games/Maiya")


def get_desktop_dir():
    if REAL_WINDOWS:
        desktop = os.path.join(os.environ.get("USERPROFILE", "C:\\"), "Desktop")
        if os.path.isdir(desktop):
            return desktop
    if is_wsl():
        win_prof = get_windows_userprofile()
        desktop = os.path.join(win_prof, "Desktop")
        if os.path.isdir(desktop):
            return desktop
    desktop = str(Path.home() / "Desktop")
    if os.path.isdir(desktop):
        return desktop
    return str(Path.home())


def get_windows_start_menu_dir():
    if REAL_WINDOWS:
        appdata = os.environ.get("APPDATA")
        if appdata:
            return Path(appdata) / "Microsoft/Windows/Start Menu/Programs/Maiya"
        userprof = os.environ.get("USERPROFILE")
        if userprof:
            return Path(userprof) / "AppData/Roaming/Microsoft/Windows/Start Menu/Programs/Maiya"
    if is_wsl():
        win_prof = get_windows_userprofile()
        return Path(win_prof) / "AppData/Roaming/Microsoft/Windows/Start Menu/Programs/Maiya"
    return None


def get_mame_releases():
    """Fetches list of official MAME releases from GitHub API with fallback."""
    releases = []
    try:
        req = urllib.request.Request(
            "https://api.github.com/repos/mamedev/mame/releases?per_page=10",
            headers={"User-Agent": "EagleSoftwareInstaller"}
        )
        with urllib.request.urlopen(req, timeout=5) as resp:
            data = json.loads(resp.read().decode())
            for rel in data:
                tag = rel.get("tag_name", "")
                name = rel.get("name", tag)
                assets = rel.get("assets", [])
                win_asset = None
                linux_asset = None
                for a in assets:
                    aname = a.get("name", "")
                    aurl = a.get("browser_download_url", "")
                    if "b_x64.exe" in aname or ("x64" in aname and aname.endswith(".zip")):
                        win_asset = {"name": aname, "url": aurl, "size": a.get("size", 0)}
                    if aname.endswith("lx.zip") or "linux" in aname.lower():
                        linux_asset = {"name": aname, "url": aurl, "size": a.get("size", 0)}
                if win_asset or linux_asset:
                    releases.append({
                        "tag": tag,
                        "name": name,
                        "win_asset": win_asset,
                        "linux_asset": linux_asset
                    })
    except Exception:
        pass

    if not releases:
        for tag, name in [
            ("mame0289", "MAME 0.289 (Official)"),
            ("mame0288", "MAME 0.288 (Official)"),
            ("mame0287", "MAME 0.287 (Official)"),
            ("mame0286", "MAME 0.286 (Official)"),
            ("mame0285", "MAME 0.285 (Official)")
        ]:
            releases.append({
                "tag": tag,
                "name": name,
                "win_asset": {
                    "name": f"{tag}b_x64.exe",
                    "url": f"https://github.com/mamedev/mame/releases/download/{tag}/{tag}b_x64.exe",
                    "size": 89000000
                },
                "linux_asset": {
                    "name": f"{tag}lx.zip",
                    "url": f"https://github.com/mamedev/mame/releases/download/{tag}/{tag}lx.zip",
                    "size": 94000000
                }
            })
    return releases


def get_game_releases():
    """Returns available game packages: local build if present and official release zips."""
    releases = []
    
    # Check if local workspace build exists
    local_mvs_ok = (ROOT / "dist/neo/maiya/mvs/roms/maiya/780-p1.p1").exists() or (ROOT / "roms/maiya/780-p1.p1").exists()
    if local_mvs_ok:
        releases.append({
            "tag": "local",
            "name": "Local build (this SDK checkout, with EagleBIOS)",
            "is_local": True,
            "mvs_url": "",
            "aes_url": ""
        })

    # Official GitHub releases from eaglesoftware777/neogeosdk
    try:
        req = urllib.request.Request(
            "https://api.github.com/repos/eaglesoftware777/neogeosdk/releases?per_page=5",
            headers={"User-Agent": "EagleSoftwareInstaller"}
        )
        with urllib.request.urlopen(req, timeout=5) as resp:
            data = json.loads(resp.read().decode())
            for rel in data:
                tag = rel.get("tag_name", "")
                name = rel.get("name", tag)
                mvs_url = None
                aes_url = None
                for a in rel.get("assets", []):
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
                        "is_local": False,
                        "mvs_url": mvs_url or "",
                        "aes_url": aes_url or ""
                    })
    except Exception:
        pass

    if not any(r["tag"] == "v1.7.3" for r in releases):
        releases.append({
            "tag": "v1.7.3",
            "name": "NeoGeoSDK v1.7.3 Pre-Release: Maiya v4 (Official EagleBIOS)",
            "is_local": False,
            "mvs_url": "https://github.com/eaglesoftware777/neogeosdk/releases/download/v1.7.3/Maiya-WIP-EagleBIOS_v4.zip",
            "aes_url": "https://github.com/eaglesoftware777/neogeosdk/releases/download/v1.7.3/Maiya-WIP-AES-EagleBIOS_v4.zip"
        })
    # BEGIN OLDER RELEASES (sets of this game only)
    if not any(r["tag"] == "v1.7.2" for r in releases):
        releases.append({
            "tag": "v1.7.2",
            "name": "NeoGeoSDK v1.7.2 Pre-Release: Maiya v3 (Official EagleBIOS)",
            "is_local": False,
            "mvs_url": "https://github.com/eaglesoftware777/neogeosdk/releases/download/v1.7.2/Maiya-WIP-EagleBIOS_v3.zip",
            "aes_url": "https://github.com/eaglesoftware777/neogeosdk/releases/download/v1.7.2/Maiya-WIP-AES-EagleBIOS_v3.zip"
        })
    if not any(r["tag"] == "v1.7.1" for r in releases):
        releases.append({
            "tag": "v1.7.1",
            "name": "NeoGeoSDK v1.7.1 Milestone: Maiya v2 (EagleBIOS)",
            "is_local": False,
            "mvs_url": "https://github.com/eaglesoftware777/neogeosdk/releases/download/v1.7.1/Maiya-WIP-EagleBIOS_v2.zip",
            "aes_url": "https://github.com/eaglesoftware777/neogeosdk/releases/download/v1.7.1/Maiya-WIP-AES-EagleBIOS_v2.zip"
        })
    # END OLDER RELEASES

    return releases


def download_file(url, out_path, log_fn, label="File"):
    """Downloads a file with chunking and progress reporting."""
    log_fn(f"[DOWNLOAD] Downloading {label} from {url}...")
    req = urllib.request.Request(url, headers={"User-Agent": "EagleSoftwareInstaller"})
    with urllib.request.urlopen(req, timeout=30) as resp, open(out_path, "wb") as f:
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
                    log_fn(f"[DOWNLOAD] {label}: {pct}% ({mb_cur} MB / {mb_tot} MB)")
                    last_pct = pct


def download_mame(target_folder, log_fn, release_url=None):
    """Download chosen or latest MAME release for Windows or Linux."""
    is_win_target = REAL_WINDOWS or is_wsl() or "mnt/c" in str(target_folder).lower()
    download_url = release_url
    filename = None

    if download_url:
        filename = download_url.split("/")[-1]
        log_fn(f"[DOWNLOAD] Using selected MAME release: {filename}")
    else:
        rels = get_mame_releases()
        if rels:
            chosen = rels[0]
            asset = chosen.get("win_asset") if is_win_target else chosen.get("linux_asset")
            if asset:
                download_url = asset["url"]
                filename = asset["name"]
        if not download_url:
            if is_win_target:
                download_url = "https://github.com/mamedev/mame/releases/download/mame0289/mame0289b_x64.exe"
                filename = "mame0289b_x64.exe"
            else:
                download_url = "https://github.com/mamedev/mame/releases/download/mame0289/mame0289lx.zip"
                filename = "mame0289lx.zip"

    mame_dir = Path(target_folder) / "mame"
    mame_dir.mkdir(parents=True, exist_ok=True)
    archive_path = mame_dir / filename

    download_file(download_url, archive_path, log_fn, label="MAME Emulator")

    log_fn(f"[DOWNLOAD] Extracting MAME archive to {mame_dir}...")
    if filename.endswith(".zip"):
        with zipfile.ZipFile(archive_path, 'r') as zip_ref:
            zip_ref.extractall(mame_dir)
    elif filename.endswith(".exe"):
        has_7z = shutil.which("7z") is not None
        if has_7z:
            subprocess.run(["7z", "x", "-y", f"-o{mame_dir}", str(archive_path)], check=False)
        elif REAL_WINDOWS:
            subprocess.run([str(archive_path), "-y"], cwd=str(mame_dir), check=False)
        elif is_wsl():
            try:
                win_archive = subprocess.check_output(["wslpath", "-w", str(archive_path)]).decode().strip()
                win_mame_dir = subprocess.check_output(["wslpath", "-w", str(mame_dir)]).decode().strip()
                subprocess.run(
                    ["/mnt/c/Windows/System32/cmd.exe", "/c", f"cd /d \"{win_mame_dir}\" && \"{win_archive}\" -y"],
                    check=False
                )
            except Exception:
                pass

    for cand in ("mame.exe", "mame64.exe", "mame"):
        found = list(mame_dir.rglob(cand))
        if found:
            log_fn(f"[DOWNLOAD] MAME executable ready at {found[0]}")
            return str(found[0])

    return str(mame_dir / ("mame.exe" if is_win_target else "mame"))


def perform_install(
    target_path_str,
    mame_source="existing",
    user_mame_path="",
    mame_release_url="",
    game_source="local",
    game_mvs_url="",
    game_aes_url="",
    install_mvs=True,
    install_aes=True,
    create_shortcuts=True,
    download_sd=False,
    sd_format="all",
    log_fn=print,
    progress_fn=lambda p, s: None
):
    """Executes the full installation."""
    target = Path(target_path_str).resolve()
    target.mkdir(parents=True, exist_ok=True)
    progress_fn(10, "Setting up destination folder...")
    log_fn(f"[INSTALL] Target installation directory: {target}")

    # Copy custom Maiya icons into target installation
    icon_ico = INSTALLER_DIR / "maiya_icon.ico"
    icon_png = INSTALLER_DIR / "maiya_icon.png"
    if icon_ico.exists():
        shutil.copy2(icon_ico, target / "maiya_icon.ico")
    if icon_png.exists():
        shutil.copy2(icon_png, target / "maiya_icon.png")
    log_fn("[INSTALL] Deployed Maiya custom icon assets (.ico / .png).")

    # Step 1: MAME Resolution
    mame_exe_path = ""
    if mame_source == "existing" and user_mame_path:
        mame_exe_path = user_mame_path
        log_fn(f"[INSTALL] Using specified existing MAME: {mame_exe_path}")
    elif mame_source == "download":
        progress_fn(20, "Downloading selected MAME emulator from official release...")
        mame_exe_path = download_mame(target, log_fn, release_url=mame_release_url)
    else:
        found, det_path = detect_mame()
        if found:
            mame_exe_path = det_path
            log_fn(f"[INSTALL] Detected host MAME: {mame_exe_path}")
        else:
            progress_fn(20, "Downloading MAME emulator...")
            mame_exe_path = download_mame(target, log_fn, release_url=mame_release_url)

    roms_dir = target / "roms"
    roms_dir.mkdir(parents=True, exist_ok=True)

    # Step 2: Game ROMs Deployment (Local or Download from Release Pages)
    local_mvs_source = ROOT / "dist/neo/maiya/mvs/roms/maiya"
    if not local_mvs_source.exists():
        local_mvs_source = ROOT / "roms/maiya"
    use_local = (game_source == "local") and (local_mvs_source / "780-p1.p1").is_file()

    if use_local:
        log_fn("[INSTALL] Deploying game packages from local SDK workspace build...")
        progress_fn(40, "Deploying Maiya MVS Arcade ROMs from local build...")
        if install_mvs:
            mvs_roms = roms_dir / "maiya"
            mvs_roms.mkdir(parents=True, exist_ok=True)
            for part in PARTS:
                f = local_mvs_source / f"780-{part}.{part}"
                if f.is_file():
                    shutil.copy2(f, mvs_roms / f.name)
            with zipfile.ZipFile(roms_dir / "maiya.zip", "w", zipfile.ZIP_DEFLATED) as z:
                for part in PARTS:
                    f = mvs_roms / f"780-{part}.{part}"
                    if f.is_file():
                        z.write(f, arcname=f.name)
            log_fn("[INSTALL] Maiya MVS package deployed.")

        if install_aes:
            progress_fn(60, "Deploying Maiya AES Console Edition...")
            aes_roms = target / "tests/aes/roms/maiya"
            aes_roms.mkdir(parents=True, exist_ok=True)
            aes_source = ROOT / "dist/neo/maiya/aes/roms/maiya"
            if not aes_source.exists():
                aes_source = local_mvs_source
            for part in PARTS:
                f = aes_source / f"780-{part}.{part}"
                if f.is_file():
                    shutil.copy2(f, aes_roms / f.name)
            with zipfile.ZipFile(target / "tests/aes/roms/maiya.zip", "w", zipfile.ZIP_DEFLATED) as z:
                for part in PARTS:
                    f = aes_roms / f"780-{part}.{part}"
                    if f.is_file():
                        z.write(f, arcname=f.name)
            memcard = target / "tests/aes/memcard.bin"
            if not memcard.exists():
                memcard.write_bytes(bytes(2048))
            log_fn("[INSTALL] Maiya AES package deployed.")

        # Deploy EagleBIOS from workspace
        progress_fn(75, "Deploying EagleBIOS system files...")
        for board, files in EAGLE.items():
            board_dir = roms_dir / board
            board_dir.mkdir(parents=True, exist_ok=True)
            for fname in files:
                src = ROOT / "bios" / fname
                if src.is_file():
                    shutil.copy2(src, board_dir / fname)
            with zipfile.ZipFile(roms_dir / f"{board}.zip", "w", zipfile.ZIP_DEFLATED) as z:
                for fname in files:
                    bf = board_dir / fname
                    if bf.is_file():
                        z.write(bf, arcname=fname)
        log_fn("[INSTALL] EagleBIOS deployed successfully.")

    else:
        # Standalone end-user mode: Download from release pages!
        log_fn("[INSTALL] Deploying game packages from official GitHub release pages...")
        mvs_dl_url = game_mvs_url or "https://github.com/eaglesoftware777/neogeosdk/releases/download/v1.7.3/Maiya-WIP-EagleBIOS_v4.zip"
        aes_dl_url = game_aes_url or "https://github.com/eaglesoftware777/neogeosdk/releases/download/v1.7.3/Maiya-WIP-AES-EagleBIOS_v4.zip"
        sys.path.insert(0, str(INSTALLER_DIR))
        from update_maiya import deploy_release_zip

        if install_mvs:
            progress_fn(40, "Downloading Maiya MVS Edition from release page...")
            mvs_zip = target / "maiya_mvs_release.zip"
            download_file(mvs_dl_url, mvs_zip, log_fn, label="Maiya MVS Release")
            log_fn("[INSTALL] Extracting MVS package...")
            deploy_release_zip(mvs_zip, target, "mvs", log_fn)
            try:
                mvs_zip.unlink()
            except Exception:
                pass
            # Ensure maiya.zip has files at root
            mvs_roms = roms_dir / "maiya"
            if mvs_roms.is_dir():
                with zipfile.ZipFile(roms_dir / "maiya.zip", "w", zipfile.ZIP_DEFLATED) as z:
                    for f in mvs_roms.iterdir():
                        if f.is_file():
                            z.write(f, arcname=f.name)
            log_fn("[INSTALL] Maiya MVS package extracted.")

        if install_aes:
            progress_fn(60, "Downloading Maiya AES Edition from release page...")
            aes_zip = target / "maiya_aes_release.zip"
            download_file(aes_dl_url, aes_zip, log_fn, label="Maiya AES Release")
            log_fn("[INSTALL] Extracting AES package...")
            deploy_release_zip(aes_zip, target, "aes", log_fn)
            try:
                aes_zip.unlink()
            except Exception:
                pass
            memcard = target / "tests/aes/memcard.bin"
            memcard.parent.mkdir(parents=True, exist_ok=True)
            if not memcard.exists():
                memcard.write_bytes(bytes(2048))
            aes_roms = target / "tests/aes/roms/maiya"
            if aes_roms.is_dir():
                with zipfile.ZipFile(target / "tests/aes/roms/maiya.zip", "w", zipfile.ZIP_DEFLATED) as z:
                    for f in aes_roms.iterdir():
                        if f.is_file():
                            z.write(f, arcname=f.name)
            log_fn("[INSTALL] Maiya AES package extracted.")

    # Deploy software list definition (hash/neogeo.xml)
    hash_dir = target / "hash"
    hash_dir.mkdir(parents=True, exist_ok=True)
    hash_maiya = hash_dir / "maiya"
    hash_maiya.mkdir(parents=True, exist_ok=True)

    # Each set keeps its own list: the MVS one in hash/, the AES one (its P1
    # differs) in tests/aes/hash/. A downloaded set brought its list with it;
    # a local build takes the build's; the built-in list is the last resort.
    xml_content = None
    if use_local:
        xml_sources = [
            ROOT / "dist/neo/maiya/mvs/hash/neogeo.xml",
            ROOT / "hash_eagle/maiya/neogeo.xml",
            ROOT / "dist/hash_eagle/maiya/neogeo.xml"
        ]
    else:
        xml_sources = [hash_dir / "neogeo.xml"]
    for src in xml_sources:
        if src.is_file():
            xml_content = src.read_bytes()
            break
    if not xml_content:
        xml_content = NEOGEO_XML.encode("utf-8")
    (hash_dir / "neogeo.xml").write_bytes(xml_content)
    (hash_maiya / "neogeo.xml").write_bytes(xml_content)
    aes_hash = target / "tests/aes/hash/neogeo.xml"
    local_aes_hash = ROOT / "dist/neo/maiya/aes/hash/neogeo.xml"
    if install_aes and use_local and local_aes_hash.is_file():
        aes_hash.parent.mkdir(parents=True, exist_ok=True)
        aes_hash.write_bytes(local_aes_hash.read_bytes())
    log_fn("[INSTALL] Software list XML (hash/neogeo.xml) deployed.")

    # Deploy updater script, batch and shell launchers, and version.json
    updater_src = INSTALLER_DIR / "update_maiya.py"
    if updater_src.is_file():
        shutil.copy2(updater_src, target / "update_maiya.py")
        (target / "update_maiya.py").chmod(0o755)
    log_fn("[INSTALL] Online updater script (update_maiya.py) deployed.")

    update_bat_content = """@echo off
setlocal
set "HERE=%~dp0"
cd /d "%HERE%"

echo ============================================================
echo   Eagle Software - Maiya: Super Nature Girl Updater
echo ============================================================
echo.

set "PY_EXE="
for %%P in (python.exe py.exe) do (
    if not defined PY_EXE (
        where %%P >nul 2>&1 && set "PY_EXE=%%P"
    )
)

if not defined PY_EXE (
    if exist "C:\\Users\\%USERNAME%\\AppData\\Local\\Programs\\Python\\Python311\\python.exe" (
        set "PY_EXE=C:\\Users\\%USERNAME%\\AppData\\Local\\Programs\\Python\\Python311\\python.exe"
    ) else if exist "C:\\Users\\%USERNAME%\\AppData\\Local\\Programs\\Python\\Python312\\python.exe" (
        set "PY_EXE=C:\\Users\\%USERNAME%\\AppData\\Local\\Programs\\Python\\Python312\\python.exe"
    ) else if exist "C:\\Users\\%USERNAME%\\AppData\\Local\\Programs\\Python\\Python313\\python.exe" (
        set "PY_EXE=C:\\Users\\%USERNAME%\\AppData\\Local\\Programs\\Python\\Python313\\python.exe"
    ) else if exist "C:\\Python311\\python.exe" (
        set "PY_EXE=C:\\Python311\\python.exe"
    ) else if exist "C:\\Python312\\python.exe" (
        set "PY_EXE=C:\\Python312\\python.exe"
    )
)

if not defined PY_EXE (
    echo [ERROR] Python not found on system.
    echo Please install Python 3 or ensure it is in your PATH.
    echo Alternatively, PowerShell will be used to check for updates...
    echo.
    powershell -ExecutionPolicy Bypass -Command "& { Write-Host 'Checking GitHub for latest release...'; $rel = Invoke-RestMethod -Uri 'https://api.github.com/repos/eaglesoftware777/neogeosdk/releases/latest' -Headers @{'User-Agent'='EagleUpdater'}; Write-Host ('Latest Release on GitHub: ' + $rel.tag_name + ' - ' + $rel.name); Write-Host 'Download packages from: ' $rel.html_url; }"
    echo.
    pause
    exit /b 1
)

"%PY_EXE%" "%HERE%update_maiya.py" %*
if errorlevel 1 (
    echo.
    echo [WARN] Updater ended with code %ERRORLEVEL%.
)
echo.
pause
exit /b %ERRORLEVEL%
"""
    (target / "update_maiya.bat").write_bytes(update_bat_content.replace("\n", "\r\n").encode("ascii"))

    update_sh_content = """#!/usr/bin/env bash
set -e
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$HERE"

echo "============================================================"
echo "  Eagle Software - Maiya: Super Nature Girl Updater"
echo "============================================================"
echo ""

PY_BIN=""
for cmd in python3 python; do
    if command -v "$cmd" &>/dev/null; then
        PY_BIN="$cmd"
        break
    fi
done

if [ -z "$PY_BIN" ]; then
    echo "[ERROR] Python 3 not found on system. Please install python3."
    exit 1
fi

exec "$PY_BIN" "$HERE/update_maiya.py" "$@"
"""
    update_sh = target / "update_maiya.sh"
    update_sh.write_text(update_sh_content)
    update_sh.chmod(0o755)

    # Deploy SD card / flashcart downloader script and launchers
    sd_dl_src = INSTALLER_DIR / "download_sd.py"
    if sd_dl_src.is_file():
        shutil.copy2(sd_dl_src, target / "download_sd.py")
        (target / "download_sd.py").chmod(0o755)
    sd_bat_src = INSTALLER_DIR / "download_sd_images.bat"
    if sd_bat_src.is_file():
        shutil.copy2(sd_bat_src, target / "download_sd_images.bat")
    sd_sh_src = INSTALLER_DIR / "download_sd_images.sh"
    if sd_sh_src.is_file():
        shutil.copy2(sd_sh_src, target / "download_sd_images.sh")
        (target / "download_sd_images.sh").chmod(0o755)
    log_fn("[INSTALL] Flashcart SD downloader (download_sd.py, .bat, .sh) deployed.")

    if download_sd:
        progress_fn(78, "Downloading Flashcart SD images (NeoSD, Darksoft, BackBit)...")
        log_fn("[INSTALL] Fetching requested Flashcart SD images...")
        try:
            sys.path.insert(0, str(INSTALLER_DIR))
            import download_sd as dsd
            formats = ["neosd", "darksoft", "backbit"] if sd_format == "all" else [sd_format]
            dsd.download_flashcart_images(
                formats=formats,
                platforms=["mvs", "aes"],
                out_dir=target / "sd_images",
                extract_zips=True,
                log_fn=log_fn
            )
            log_fn("[INSTALL] Flashcart SD images ready in sd_images/ folder.")
        except Exception as e:
            log_fn(f"[WARN] Failed to download Flashcart SD images: {e}")

    # Deploy uninstaller script and launchers
    uninst_src = INSTALLER_DIR / "uninstall_maiya.py"
    if uninst_src.is_file():
        shutil.copy2(uninst_src, target / "uninstall_maiya.py")
        (target / "uninstall_maiya.py").chmod(0o755)
    log_fn("[INSTALL] Uninstaller script (uninstall_maiya.py) deployed.")

    uninst_bat_content = """@echo off
setlocal
set "HERE=%~dp0"
cd /d "%HERE%"

echo ============================================================
echo   Eagle Software - Maiya: Super Nature Girl Uninstaller
echo ============================================================
echo.
echo Target Installation: %HERE%
echo.
set /p "CONFIRM=Are you sure you want to completely remove Maiya? [y/N]: "
if /I not "%CONFIRM%"=="y" if /I not "%CONFIRM%"=="yes" (
    echo.
    echo Uninstallation cancelled by user.
    pause
    exit /b 0
)

echo.
echo [1/3] Removing Desktop shortcuts...
set "DESK=%USERPROFILE%\\Desktop"
if exist "%DESK%\\Maiya MVS (Arcade).url" del /f /q "%DESK%\\Maiya MVS (Arcade).url"
if exist "%DESK%\\Maiya AES (Console).url" del /f /q "%DESK%\\Maiya AES (Console).url"
if exist "%DESK%\\Maiya - Check for Updates.url" del /f /q "%DESK%\\Maiya - Check for Updates.url"
if exist "%DESK%\\Maiya - Flashcart SD Images.url" del /f /q "%DESK%\\Maiya - Flashcart SD Images.url"
if exist "%DESK%\\Maiya - Uninstall.url" del /f /q "%DESK%\\Maiya - Uninstall.url"
if exist "%DESK%\\Maiya_MVS.desktop" del /f /q "%DESK%\\Maiya_MVS.desktop"
if exist "%DESK%\\Maiya_AES.desktop" del /f /q "%DESK%\\Maiya_AES.desktop"
if exist "%DESK%\\Maiya_Update.desktop" del /f /q "%DESK%\\Maiya_Update.desktop"
if exist "%DESK%\\Maiya_SD_Images.desktop" del /f /q "%DESK%\\Maiya_SD_Images.desktop"
if exist "%DESK%\\Maiya_Uninstall.desktop" del /f /q "%DESK%\\Maiya_Uninstall.desktop"
set "SM=%APPDATA%\\Microsoft\\Windows\\Start Menu\\Programs\\Maiya"
if exist "%SM%" rmdir /s /q "%SM%"

echo [2/3] Cleaning game files and configurations...
set "TEMP_WORKER=%TEMP%\\maiya_uninst_%RANDOM%.bat"
(
echo @echo off
echo ping 127.0.0.1 -n 2 ^>nul
echo rmdir /s /q "%HERE%" ^>nul 2^>^&1
echo echo.
echo echo ============================================================
echo echo   [SUCCESS] Maiya: Super Nature Girl was completely uninstalled.
echo echo ============================================================
echo echo.
echo del "%%~f0" ^>nul 2^>^&1
echo pause
) > "%TEMP_WORKER%"

echo [3/3] Finalizing uninstallation...
start "" "%TEMP_WORKER%"
exit /b 0
"""
    (target / "uninstall_maiya.bat").write_bytes(uninst_bat_content.replace("\n", "\r\n").encode("ascii"))

    uninst_sh_content = """#!/usr/bin/env bash
set -e
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$HERE"

echo "============================================================"
echo "  Eagle Software - Maiya: Super Nature Girl Uninstaller"
echo "============================================================"
echo ""
read -r -p "Are you sure you want to completely remove Maiya? [y/N]: " CONFIRM
CONFIRM=$(echo "$CONFIRM" | tr '[:upper:]' '[:lower:]')
if [ "$CONFIRM" != "y" ] && [ "$CONFIRM" != "yes" ]; then
    echo "Uninstallation cancelled."
    exit 0
fi

echo "[1/3] Removing Desktop shortcuts..."
DESK="$HOME/Desktop"
rm -f "$DESK/Maiya MVS (Arcade).url" "$DESK/Maiya AES (Console).url" "$DESK/Maiya - Check for Updates.url" "$DESK/Maiya - Flashcart SD Images.url" "$DESK/Maiya - Uninstall.url"
rm -f "$DESK/Maiya_MVS.desktop" "$DESK/Maiya_AES.desktop" "$DESK/Maiya_Update.desktop" "$DESK/Maiya_SD_Images.desktop" "$DESK/Maiya_Uninstall.desktop"
rm -f "$HOME/.local/share/applications/maiya_"*.desktop
rm -f "$HOME/.local/share/icons/maiya_icon.png"
for u in /mnt/c/Users/*; do
    if [ -d "$u/AppData/Roaming/Microsoft/Windows/Start Menu/Programs/Maiya" ]; then
        rm -rf "$u/AppData/Roaming/Microsoft/Windows/Start Menu/Programs/Maiya"
    fi
done

echo "[2/3] Cleaning game files..."
rm -rf "$HERE/roms" "$HERE/hash" "$HERE/tests" "$HERE/mame" "$HERE/sd_images"
rm -f "$HERE"/run_maiya* "$HERE"/update_maiya* "$HERE"/uninstall_maiya* "$HERE"/download_sd* "$HERE"/version.json "$HERE"/maiya_icon.*

echo "[3/3] Removing directory..."
cd "$HOME"
rmdir "$HERE" 2>/dev/null || true

echo ""
echo "============================================================"
echo " [SUCCESS] Maiya: Super Nature Girl was completely uninstalled."
echo "============================================================"
"""
    uninst_sh = target / "uninstall_maiya.sh"
    uninst_sh.write_text(uninst_sh_content)
    uninst_sh.chmod(0o755)

    # Write version.json
    version_info = {
        "version": "v1.7.3",
        "release_name": "NeoGeoSDK v1.7.3 WIP pre-release: Maiya v4 (Official EagleBIOS)",
        "tag": "v1.7.3",
        "installed_at": time.strftime("%Y-%m-%d %H:%M:%S")
    }
    (target / "version.json").write_text(json.dumps(version_info, indent=2), encoding="utf-8")
    log_fn("[INSTALL] Installed version metadata (version.json) recorded.")

    progress_fn(80, "Generating launcher scripts...")
    # Generate Launchers
    rel_mame = mame_exe_path
    if is_wsl() and rel_mame.startswith("/mnt/c/"):
        rel_mame = "C:" + rel_mame[6:].replace("/", "\\")

    bat_content = f"""@echo off
setlocal
set "HERE=%~dp0"
set "PLATFORM=%~1"
if "%PLATFORM%"=="" set "PLATFORM=mvs"
if /I "%PLATFORM%"=="mvs" goto mvs
if /I "%PLATFORM%"=="aes" goto aes
if /I "%PLATFORM%"=="update" goto update
if /I "%PLATFORM%"=="sd" goto sd
if /I "%PLATFORM%"=="uninstall" goto uninstall
echo Usage: run_maiya.bat [mvs ^| aes ^| update ^| sd ^| uninstall]
exit /b 1

:sd
if exist "%HERE%download_sd_images.bat" (
    call "%HERE%download_sd_images.bat" %2 %3 %4
) else if exist "%HERE%download_sd.py" (
    python "%HERE%download_sd.py" %2 %3 %4
) else (
    echo [ERROR] Flashcart downloader script not found.
    pause
)
exit /b %ERRORLEVEL%

:uninstall
if exist "%HERE%uninstall_maiya.bat" (
    call "%HERE%uninstall_maiya.bat"
) else if exist "%HERE%uninstall_maiya.py" (
    python "%HERE%uninstall_maiya.py"
) else (
    echo [ERROR] Uninstaller script not found.
    pause
)
exit /b %ERRORLEVEL%

:update
if exist "%HERE%update_maiya.bat" (
    call "%HERE%update_maiya.bat" %2 %3 %4 %5
) else if exist "%HERE%update_maiya.py" (
    python "%HERE%update_maiya.py" %2 %3 %4 %5
) else (
    echo [ERROR] Update script not found.
)
exit /b %ERRORLEVEL%

:mvs
set "PLATFORM=mvs"
set "DRIVER=neogeo"
set "BIOS=euro"
set "CARD="
goto launch

:aes
set "PLATFORM=aes"
set "DRIVER=aes"
set "BIOS=asia"
set "CARD="
if exist "%HERE%tests\\aes\\memcard.bin" set CARD=-memc "%HERE%tests\\aes\\memcard.bin"
goto launch

:launch
set "MAME_EXE={rel_mame}"
if not exist "%MAME_EXE%" if exist "%HERE%mame\\mame.exe" set "MAME_EXE=%HERE%mame\\mame.exe"
if not exist "%MAME_EXE%" if exist "%HERE%mame\\mame64.exe" set "MAME_EXE=%HERE%mame\\mame64.exe"
if not exist "%MAME_EXE%" if exist "C:\\mame\\mame.exe" set "MAME_EXE=C:\\mame\\mame.exe"
if not exist "%MAME_EXE%" if exist "C:\\mame\\mame64.exe" set "MAME_EXE=C:\\mame\\mame64.exe"
if not exist "%MAME_EXE%" if exist "C:\\Games\\mame\\mame.exe" set "MAME_EXE=C:\\Games\\mame\\mame.exe"
if not exist "%MAME_EXE%" set "MAME_EXE=mame"

set "MAME_DIR="
if exist "%MAME_EXE%" for %%I in ("%MAME_EXE%") do set "MAME_DIR=%%~dpI"

set "ROMPATH=%HERE%tests\\%PLATFORM%\\roms;%HERE%roms"
if defined MAME_DIR if exist "%MAME_DIR%roms" set "ROMPATH=%ROMPATH%;%MAME_DIR%roms"
if exist "C:\\mame\\roms" if not "%MAME_DIR%"=="C:\\mame\\" set "ROMPATH=%ROMPATH%;C:\\mame\\roms"

set "HASHPATH=%HERE%tests\\%PLATFORM%\\hash;%HERE%hash;%HERE%hash\\maiya"
if defined MAME_DIR if exist "%MAME_DIR%hash" set "HASHPATH=%HASHPATH%;%MAME_DIR%hash"
if exist "C:\\mame\\hash" if not "%MAME_DIR%"=="C:\\mame\\" set "HASHPATH=%HASHPATH%;C:\\mame\\hash"
set "HASHPATH=%HASHPATH%;hash"

if not exist "%HERE%tests\\%PLATFORM%\\cfg" mkdir "%HERE%tests\\%PLATFORM%\\cfg"
if not exist "%HERE%tests\\%PLATFORM%\\nvram" mkdir "%HERE%tests\\%PLATFORM%\\nvram"

"%MAME_EXE%" %DRIVER% -rompath "%ROMPATH%" -hashpath "%HASHPATH%" -bios %BIOS% -cart1 maiya %CARD% -cfg_directory "%HERE%tests\\%PLATFORM%\\cfg" -nvram_directory "%HERE%tests\\%PLATFORM%\\nvram" -nofilter -window %2 %3 %4 %5 %6 %7 %8 %9
exit /b %ERRORLEVEL%
"""
    (target / "run_maiya.bat").write_bytes(bat_content.replace("\n", "\r\n").encode("utf-8"))
    (target / "run_maiya_mvs.bat").write_bytes(
        b"@echo off\r\ncall \"%~dp0run_maiya.bat\" mvs %*\r\nexit /b %ERRORLEVEL%\r\n"
    )
    (target / "run_maiya_aes.bat").write_bytes(
        b"@echo off\r\ncall \"%~dp0run_maiya.bat\" aes %*\r\nexit /b %ERRORLEVEL%\r\n"
    )

    sh_content = f"""#!/usr/bin/env bash
HERE="$(cd "$(dirname "${{BASH_SOURCE[0]}}")" && pwd)"
PLATFORM="${{1:-mvs}}"
shift || true

if [ "$PLATFORM" = "update" ]; then
    exec "$HERE/update_maiya.sh" "$@"
fi

if [ "$PLATFORM" = "sd" ]; then
    exec "$HERE/download_sd_images.sh" "$@"
fi

if [ "$PLATFORM" = "uninstall" ]; then
    exec "$HERE/uninstall_maiya.sh" "$@"
fi

MAME_BIN="{mame_exe_path}"
if ! command -v "$MAME_BIN" &>/dev/null; then
    if [ -x "$HERE/mame/mame" ]; then
        MAME_BIN="$HERE/mame/mame"
    elif [ -x "/mnt/c/mame/mame.exe" ]; then
        MAME_BIN="/mnt/c/mame/mame.exe"
    elif [ -x "/mnt/c/mame/mame64.exe" ]; then
        MAME_BIN="/mnt/c/mame/mame64.exe"
    else
        MAME_BIN="mame"
    fi
fi

MAME_DIR="$(dirname "$MAME_BIN" 2>/dev/null || true)"
ROMPATH="$HERE/tests/$PLATFORM/roms:$HERE/roms"
[ -d "$MAME_DIR/roms" ] && ROMPATH="$ROMPATH:$MAME_DIR/roms"
[ -d "/mnt/c/mame/roms" ] && ROMPATH="$ROMPATH:/mnt/c/mame/roms"
[ -d "/usr/share/games/mame/roms" ] && ROMPATH="$ROMPATH:/usr/share/games/mame/roms"

HASHPATH="$HERE/tests/$PLATFORM/hash:$HERE/hash:$HERE/hash/maiya"
[ -d "$MAME_DIR/hash" ] && HASHPATH="$HASHPATH:$MAME_DIR/hash"
[ -d "/mnt/c/mame/hash" ] && HASHPATH="$HASHPATH:/mnt/c/mame/hash"
HASHPATH="$HASHPATH:hash"

mkdir -p "$HERE/tests/$PLATFORM/cfg" "$HERE/tests/$PLATFORM/nvram"

if [ "$PLATFORM" = "aes" ]; then
    CARD_ARG=""
    [ -f "$HERE/tests/aes/memcard.bin" ] && CARD_ARG="-memc $HERE/tests/aes/memcard.bin"
    exec "$MAME_BIN" aes -rompath "$ROMPATH" -hashpath "$HASHPATH" -bios asia -cart1 maiya $CARD_ARG -cfg_directory "$HERE/tests/aes/cfg" -nvram_directory "$HERE/tests/aes/nvram" -nofilter -window "$@"
else
    exec "$MAME_BIN" neogeo -rompath "$ROMPATH" -hashpath "$HASHPATH" -bios euro -cart1 maiya -cfg_directory "$HERE/tests/mvs/cfg" -nvram_directory "$HERE/tests/mvs/nvram" -nofilter -window "$@"
fi
"""
    run_sh = target / "run_maiya.sh"
    run_sh.write_text(sh_content)
    run_sh.chmod(0o755)
    (target / "run_maiya_mvs.sh").write_text("#!/usr/bin/env bash\nexec \"$(dirname \"$0\")/run_maiya.sh\" mvs \"$@\"\n")
    (target / "run_maiya_mvs.sh").chmod(0o755)
    (target / "run_maiya_aes.sh").write_text("#!/usr/bin/env bash\nexec \"$(dirname \"$0\")/run_maiya.sh\" aes \"$@\"\n")
    (target / "run_maiya_aes.sh").chmod(0o755)

    log_fn("[INSTALL] Direct launcher scripts created successfully (no interactive menu).")

    # Step 3: Desktop & Start Menu Shortcuts
    if create_shortcuts:
        progress_fn(90, "Creating Desktop and Start Menu shortcuts with custom Maiya icon...")
        desktop_dir = get_desktop_dir()
        log_fn(f"[INSTALL] Target Desktop folder: {desktop_dir}")

        # Windows Shortcuts
        if REAL_WINDOWS or is_wsl() or "mnt/c" in str(target).lower():
            try:
                if is_wsl():
                    win_target = subprocess.check_output(["wslpath", "-w", str(target)]).decode().strip()
                else:
                    win_target = str(target)
                win_target_norm = win_target.replace("\\", "/")
                win_icon_path = f"{win_target}\\maiya_icon.ico"

                # 1. Desktop Shortcuts
                if install_mvs:
                    mvs_url = f"[InternetShortcut]\r\nURL=file:///{win_target_norm}/run_maiya_mvs.bat\r\nIconFile={win_icon_path}\r\nIconIndex=0\r\n"
                    (Path(desktop_dir) / "Maiya MVS (Arcade).url").write_bytes(mvs_url.encode("ascii"))
                if install_aes:
                    aes_url = f"[InternetShortcut]\r\nURL=file:///{win_target_norm}/run_maiya_aes.bat\r\nIconFile={win_icon_path}\r\nIconIndex=0\r\n"
                    (Path(desktop_dir) / "Maiya AES (Console).url").write_bytes(aes_url.encode("ascii"))
                update_url = f"[InternetShortcut]\r\nURL=file:///{win_target_norm}/update_maiya.bat\r\nIconFile={win_icon_path}\r\nIconIndex=0\r\n"
                (Path(desktop_dir) / "Maiya - Check for Updates.url").write_bytes(update_url.encode("ascii"))
                sd_url = f"[InternetShortcut]\r\nURL=file:///{win_target_norm}/download_sd_images.bat\r\nIconFile={win_icon_path}\r\nIconIndex=0\r\n"
                (Path(desktop_dir) / "Maiya - Flashcart SD Images.url").write_bytes(sd_url.encode("ascii"))
                uninst_url = f"[InternetShortcut]\r\nURL=file:///{win_target_norm}/uninstall_maiya.bat\r\nIconFile={win_icon_path}\r\nIconIndex=0\r\n"
                (Path(desktop_dir) / "Maiya - Uninstall.url").write_bytes(uninst_url.encode("ascii"))
                log_fn("[SHORTCUT] Windows Desktop shortcuts created.")

                # 2. Windows Start Menu Folder Shortcuts
                sm_dir = get_windows_start_menu_dir()
                if sm_dir:
                    sm_dir.mkdir(parents=True, exist_ok=True)
                    if install_mvs:
                        (sm_dir / "Maiya MVS (Arcade).url").write_bytes(mvs_url.encode("ascii"))
                    if install_aes:
                        (sm_dir / "Maiya AES (Console).url").write_bytes(aes_url.encode("ascii"))
                    (sm_dir / "Check for Updates.url").write_bytes(update_url.encode("ascii"))
                    (sm_dir / "Flashcart SD Images.url").write_bytes(sd_url.encode("ascii"))
                    (sm_dir / "Uninstall Maiya.url").write_bytes(uninst_url.encode("ascii"))
                    log_fn(f"[SHORTCUT] Windows Start Menu shortcuts created in: {sm_dir}")
            except Exception as e:
                log_fn(f"[WARN] Failed to create Windows shortcuts: {e}")

        # Linux Shortcuts
        desktop_path = Path(desktop_dir)
        if desktop_path.is_dir():
            icon_file = target / "maiya_icon.png"
            # Also copy icon to user icons directory
            user_icons = Path.home() / ".local/share/icons"
            user_icons.mkdir(parents=True, exist_ok=True)
            if icon_file.exists():
                shutil.copy2(icon_file, user_icons / "maiya_icon.png")

            user_apps = Path.home() / ".local/share/applications"
            user_apps.mkdir(parents=True, exist_ok=True)

            items = []
            if install_mvs:
                items.append(("mvs", "Maiya MVS (Arcade)"))
            if install_aes:
                items.append(("aes", "Maiya AES (Console)"))
            items.append(("update", "Maiya - Check for Updates"))
            items.append(("sd", "Maiya - Flashcart SD Images"))
            items.append(("uninstall", "Maiya - Uninstall"))

            for mode, title in items:
                if mode == "update":
                    script_name = "update_maiya.sh"
                elif mode == "sd":
                    script_name = "download_sd_images.sh"
                elif mode == "uninstall":
                    script_name = "uninstall_maiya.sh"
                else:
                    script_name = f"run_maiya_{mode}.sh"
                dt_content = f"""[Desktop Entry]
Type=Application
Name={title}
Comment=Maiya: Super Nature Girl for Neo-Geo
Exec="{target}/{script_name}"
Path={target}
Icon={icon_file}
Terminal={"true" if mode in ("update", "sd", "uninstall") else "false"}
Categories=Game;Emulator;
"""
                dt_desktop = desktop_path / f"Maiya_{mode.capitalize()}.desktop"
                dt_desktop.write_text(dt_content)
                dt_desktop.chmod(0o755)

                dt_app = user_apps / f"maiya_{mode}.desktop"
                dt_app.write_text(dt_content)
                dt_app.chmod(0o755)

            log_fn("[SHORTCUT] Linux .desktop shortcuts created with custom Maiya icon.")

    progress_fn(100, "Installation complete!")
    log_fn("[SUCCESS] Maiya: Super Nature Girl setup completed successfully!")
    return True


class InstallerServer(http.server.SimpleHTTPRequestHandler):
    def log_message(self, format, *args):
        pass

    def do_GET(self):
        if self.path in ("/", "/index.html"):
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.end_headers()
            html_file = Path(__file__).parent / "installer_wizard.html"
            self.wfile.write(html_file.read_bytes())
        elif self.path == "/api/sysinfo":
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            mame_found, mame_path = detect_mame()
            info = {
                "os": "windows" if (REAL_WINDOWS or is_wsl()) else "linux",
                "default_target": get_default_install_dir(),
                "mame_found": mame_found,
                "mame_path": mame_path
            }
            self.wfile.write(json.dumps(info).encode())
        elif self.path == "/api/locales":
            self.send_response(200)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.end_headers()
            locales_file = Path(__file__).parent / "locales.json"
            if locales_file.exists():
                self.wfile.write(locales_file.read_bytes())
            else:
                self.wfile.write(b"{}")
        elif self.path == "/api/mame_releases":
            self.send_response(200)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.end_headers()
            rels = get_mame_releases()
            self.wfile.write(json.dumps(rels).encode())
        elif self.path == "/api/game_releases":
            self.send_response(200)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.end_headers()
            rels = get_game_releases()
            self.wfile.write(json.dumps(rels).encode())
        elif self.path == "/api/assets":
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            assets_file = Path(__file__).parent / "assets_b64.json"
            if assets_file.exists():
                self.wfile.write(assets_file.read_bytes())
            else:
                self.wfile.write(b"{}")
        elif self.path == "/api/check_update":
            self.send_response(200)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.end_headers()
            try:
                sys.path.insert(0, str(INSTALLER_DIR))
                import update_maiya
                rels = update_maiya.fetch_github_releases()
                latest = rels[0] if rels else {}
            except Exception as e:
                latest = {"error": str(e), "tag": "v1.7.3", "name": "Maiya: Super Nature Girl (Release)"}
            self.wfile.write(json.dumps(latest).encode())
        elif self.path == "/api/sd_assets":
            self.send_response(200)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.end_headers()
            try:
                sys.path.insert(0, str(INSTALLER_DIR))
                import download_sd
                catalog = download_sd.fetch_latest_sd_assets()
            except Exception as e:
                catalog = {"error": str(e)}
            self.wfile.write(json.dumps(catalog).encode())
        else:
            self.send_error(404)

    def do_POST(self):
        if self.path == "/api/install":
            content_length = int(self.headers.get("Content-Length", 0))
            body = self.rfile.read(content_length).decode()
            params = json.loads(body)

            self.send_response(200)
            self.send_header("Content-Type", "text/plain; charset=utf-8")
            self.send_header("Transfer-Encoding", "chunked")
            self.end_headers()

            def send_chunk(obj):
                chunk = (json.dumps(obj) + "\n").encode()
                hex_len = f"{len(chunk):X}\r\n".encode()
                try:
                    self.wfile.write(hex_len + chunk + b"\r\n")
                    self.wfile.flush()
                except Exception:
                    pass

            def log_fn(msg):
                send_chunk({"log": msg})

            def progress_fn(pct, status):
                send_chunk({"progress": pct, "status": status})

            try:
                target_dir = params.get("target_dir")
                mame_source = params.get("mame_source", "existing")
                mame_path = params.get("mame_path", "")
                mame_release_url = params.get("mame_release_url", "")
                game_source = params.get("game_source", "local")
                game_mvs_url = params.get("game_mvs_url", "")
                game_aes_url = params.get("game_aes_url", "")
                install_mvs = params.get("install_mvs", True)
                install_aes = params.get("install_aes", True)
                create_shortcuts = params.get("create_shortcuts", True)
                download_sd_opt = params.get("download_sd", False)
                sd_format_opt = params.get("sd_format", "all")

                perform_install(
                    target_dir,
                    mame_source=mame_source,
                    user_mame_path=mame_path,
                    mame_release_url=mame_release_url,
                    game_source=game_source,
                    game_mvs_url=game_mvs_url,
                    game_aes_url=game_aes_url,
                    install_mvs=install_mvs,
                    install_aes=install_aes,
                    create_shortcuts=create_shortcuts,
                    download_sd=download_sd_opt,
                    sd_format=sd_format_opt,
                    log_fn=log_fn,
                    progress_fn=progress_fn
                )
                send_chunk({"done": True, "success": True})
            except Exception as e:
                send_chunk({"done": True, "success": False, "error": str(e), "log": f"[FATAL] {e}"})

            self.wfile.write(b"0\r\n\r\n")
            self.wfile.flush()

        elif self.path == "/api/download_sd":
            content_length = int(self.headers.get("Content-Length", 0))
            body = self.rfile.read(content_length).decode()
            params = json.loads(body) if body else {}

            self.send_response(200)
            self.send_header("Content-Type", "text/plain; charset=utf-8")
            self.send_header("Transfer-Encoding", "chunked")
            self.end_headers()

            def send_chunk(obj):
                chunk = (json.dumps(obj) + "\n").encode()
                hex_len = f"{len(chunk):X}\r\n".encode()
                try:
                    self.wfile.write(hex_len + chunk + b"\r\n")
                    self.wfile.flush()
                except Exception:
                    pass

            def log_fn(msg):
                send_chunk({"log": msg})

            def progress_fn(pct, status):
                send_chunk({"progress": pct, "status": status})

            try:
                target_dir = params.get("target_dir") or get_default_install_dir()
                formats = params.get("formats", ["neosd", "darksoft", "backbit"])
                platforms = params.get("platforms", ["mvs", "aes"])
                sys.path.insert(0, str(INSTALLER_DIR))
                import download_sd
                out_dir = Path(target_dir) / "sd_images"
                download_sd.download_flashcart_images(
                    formats=formats,
                    platforms=platforms,
                    out_dir=out_dir,
                    extract_zips=True,
                    log_fn=log_fn,
                    progress_fn=progress_fn
                )
                send_chunk({"done": True, "success": True, "out_dir": str(out_dir)})
            except Exception as e:
                send_chunk({"done": True, "success": False, "error": str(e), "log": f"[ERROR] {e}"})

            self.wfile.write(b"0\r\n\r\n")
            self.wfile.flush()

        elif self.path == "/api/uninstall":
            content_length = int(self.headers.get("Content-Length", 0))
            body = self.rfile.read(content_length).decode()
            params = json.loads(body) if body else {}
            target_dir = params.get("target_dir") or get_default_install_dir()

            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            try:
                sys.path.insert(0, str(INSTALLER_DIR))
                import uninstall_maiya
                ok = uninstall_maiya.perform_uninstall(target_dir, auto_yes=True, log_fn=lambda m: None)
                self.wfile.write(json.dumps({"success": ok}).encode())
            except Exception as e:
                self.wfile.write(json.dumps({"success": False, "error": str(e)}).encode())

        elif self.path == "/api/shutdown":
            self.send_response(200)
            self.end_headers()
            self.wfile.write(b'{"status":"ok"}')
            threading.Thread(target=lambda: (time.sleep(1), os._exit(0))).start()
        else:
            self.send_error(404)


def run_gui(port=8765):
    server_address = ("127.0.0.1", port)
    try:
        httpd = http.server.HTTPServer(server_address, InstallerServer)
    except OSError:
        server_address = ("127.0.0.1", port + 1)
        httpd = http.server.HTTPServer(server_address, InstallerServer)
        port = port + 1

    url = f"http://127.0.0.1:{port}/"
    print(f"\n==========================================================")
    print(f" Eagle Software Setup Wizard - Maiya: Super Nature Girl")
    print(f" Running at: {url}")
    print(f"==========================================================\n")

    opened = False
    if is_wsl():
        try:
            subprocess.run(["/mnt/c/Windows/explorer.exe", url], check=False)
            opened = True
        except Exception:
            pass
    if not opened:
        try:
            webbrowser.open(url)
        except Exception:
            pass

    print("Installer GUI running. Press Ctrl+C in terminal to stop.")
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\nInstaller stopped.")


def run_cli(args):
    if args.uninstall:
        sys.path.insert(0, str(INSTALLER_DIR))
        import uninstall_maiya
        uninstall_maiya.perform_uninstall(args.target, auto_yes=True, log_fn=print)
        return

    if args.download_sd_only:
        sys.path.insert(0, str(INSTALLER_DIR))
        import download_sd
        formats = ["neosd", "darksoft", "backbit"] if args.sd_format == "all" else [args.sd_format]
        platforms = ["mvs", "aes"] if args.sd_platform == "both" else [args.sd_platform]
        out_dir = Path(args.target) / "sd_images"
        download_sd.download_flashcart_images(formats=formats, platforms=platforms, out_dir=out_dir)
        return

    def log_fn(msg):
        print(msg)

    def progress_fn(pct, status):
        print(f"[{pct}%] {status}")

    perform_install(
        args.target,
        mame_source=args.mame_source,
        user_mame_path=args.mame_path,
        mame_release_url=args.mame_url,
        game_source=args.game_source,
        game_mvs_url=args.game_mvs_url,
        game_aes_url=args.game_aes_url,
        install_mvs=not args.no_mvs,
        install_aes=not args.no_aes,
        create_shortcuts=not args.no_shortcuts,
        download_sd=args.download_sd,
        sd_format=args.sd_format,
        log_fn=log_fn,
        progress_fn=progress_fn
    )


def main():
    parser = argparse.ArgumentParser(description="Maiya: Super Nature Girl Official Installer")
    parser.add_argument("--cli", action="store_true", help="Run unattended in command-line mode")
    parser.add_argument("--uninstall", action="store_true", help="Uninstall game from target directory")
    parser.add_argument("--target", default=get_default_install_dir(), help="Install destination directory")
    parser.add_argument("--mame-source", choices=["existing", "download"], default="existing", help="MAME source option")
    parser.add_argument("--mame-path", default="", help="Path to MAME executable")
    parser.add_argument("--mame-url", default="", help="Direct MAME asset download URL")
    parser.add_argument("--game-source", choices=["local", "download"], default="local", help="Game package source")
    parser.add_argument("--game-mvs-url", default="", help="Direct Maiya MVS zip download URL")
    parser.add_argument("--game-aes-url", default="", help="Direct Maiya AES zip download URL")
    parser.add_argument("--no-mvs", action="store_true", help="Skip MVS edition")
    parser.add_argument("--no-aes", action="store_true", help="Skip AES edition")
    parser.add_argument("--no-shortcuts", action="store_true", help="Skip desktop shortcuts")
    parser.add_argument("--download-sd", action="store_true", help="Also download Flashcart SD images during install")
    parser.add_argument("--download-sd-only", action="store_true", help="Download Flashcart SD images only (skip MAME)")
    parser.add_argument("--sd-format", choices=["neosd", "darksoft", "backbit", "all"], default="all",
                        help="Flashcart image format (neosd, darksoft, backbit, or all)")
    parser.add_argument("--sd-platform", choices=["mvs", "aes", "both"], default="both",
                        help="Platform variant for flashcart images")
    parser.add_argument("--port", type=int, default=8765, help="Port for the GUI wizard web interface")

    args = parser.parse_args()
    if args.cli:
        run_cli(args)
    else:
        run_gui(port=args.port)


if __name__ == "__main__":
    main()
