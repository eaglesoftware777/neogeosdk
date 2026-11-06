#!/usr/bin/env python3
"""
The setup packages uploaded with a pre-release, into dist/:

  Maiya_Setup_Wizard.zip   Maiya's setup wizard (tools/installer): installs
                           the MVS and AES MAME sets side by side with
                           launchers, and downloads the SD-card files
  SkyLance_Setup.zip       Sky Lance's setup (tools/installer/skylance):
                           install_skylance.bat (Windows PowerShell) or
                           install_skylance.sh puts the MVS and AES MAME sets,
                           EagleBIOS and the launchers into a MAME folder,
                           checking the release zips against the manifest

Both download the game's files from the release (or take them from beside
the script). Rebuilt for each release and not tracked.

    python3 tools/make_setup_zips.py
"""

import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
INSTALLER = ROOT / "tools/installer"
MAIYA = ["install_windows.bat", "install_linux.sh"] + [f"tools/installer/{n}" for n in (
    "assets_b64.json", "download_sd.py", "download_sd_images.bat", "download_sd_images.sh",
    "installer_wizard.html", "install_maiya.py", "locales.json", "maiya_icon.ico", "maiya_icon.png",
    "story_en.json", "uninstall_maiya.py", "update_maiya.py")]
SKY_LAUNCHERS = ("run_skylance.bat", "run_skylance_mvs.bat", "run_skylance_aes.bat",
                 "run_skylance_mvs_scanlines.bat", "run_skylance_aes_scanlines.bat", "skylance_display.bat")


def crlf(data):
    return data.replace(b"\r\n", b"\n").replace(b"\n", b"\r\n")


def write(path, files):
    with zipfile.ZipFile(path, "w", zipfile.ZIP_DEFLATED) as z:
        for name, (data, executable) in files.items():
            info = zipfile.ZipInfo(name, date_time=(2026, 10, 12, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = (0o755 if executable else 0o644) << 16
            z.writestr(info, data)
    print(f"setup: {path.relative_to(ROOT)} ({path.stat().st_size} bytes, {len(files)} files)")


def main():
    out = ROOT / "dist"
    write(out / "Maiya_Setup_Wizard.zip",
          {n: ((ROOT / n).read_bytes(), n.endswith(".sh")) for n in MAIYA})
    sky = "SkyLance_Setup/"
    files = {}
    for name in ("install_skylance.ps1", "install_skylance.bat"):
        files[sky + name] = (crlf((INSTALLER / "skylance" / name).read_bytes()), False)
    for name in ("install_skylance.sh", "run_skylance.sh"):
        files[sky + name] = ((INSTALLER / "skylance" / name).read_bytes(), True)
    for name in SKY_LAUNCHERS:
        files[sky + name] = (crlf((ROOT / "tools/mame_tests" / name).read_bytes()), False)
    readme = (b"SKY LANCE v1 -- NeoGeoSDK v1.7.3 pre-release setup\r\n\r\n"
              b"Windows: run install_skylance.bat (asks for the folder, default C:\\mame\\neogeosdk;\r\n"
              b"  MAME's mame.exe one folder up, or its path in mame-path.txt there).\r\n"
              b"Linux/macOS: ./install_skylance.sh [folder] (default ~/mame/neogeosdk).\r\n"
              b"The MVS and AES MAME sets are downloaded from the release (or taken from\r\n"
              b"beside the script) and checked against SkyLance-MANIFEST_v1.txt.\r\n\r\n"
              b"Then: run_skylance_mvs.bat (arcade) or run_skylance_aes.bat (console), both\r\n"
              b"with EagleBIOS; add 'stock' or 'unibios40' to use a BIOS you installed in\r\n"
              b"MAME's roms folder. _scanlines.bat versions use MAME's CRT shader.\r\n"
              b"NeoSD, Darksoft and BackBit files: download them from the release page.\r\n\r\n")
    files[sky + "SKYLANCE-README.txt"] = (readme + crlf((ROOT / "games/skylance/CONTROLS.txt").read_bytes()), False)
    files[sky + "SkyLance-MANIFEST_v1.txt"] = ((ROOT / "dist/release/SkyLance-MANIFEST_v1.txt").read_bytes(), False)
    write(out / "SkyLance_Setup.zip", files)


if __name__ == "__main__":
    main()
