#!/usr/bin/env python3
"""Eagle Software - Maiya: Super Nature Girl Uninstaller.

Removes all installed game packages, emulator configs, firmware files,
desktop shortcuts, and application entries cleanly from the system.
"""

import argparse
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys
import time

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


def get_desktop_dir():
    if REAL_WINDOWS:
        desktop = os.path.join(os.environ.get("USERPROFILE", "C:\\"), "Desktop")
        if os.path.isdir(desktop):
            return Path(desktop)
    if is_wsl():
        win_prof = get_windows_userprofile()
        desktop = os.path.join(win_prof, "Desktop")
        if os.path.isdir(desktop):
            return Path(desktop)
    desktop = Path.home() / "Desktop"
    if desktop.is_dir():
        return desktop
    return Path.home()


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


def remove_shortcuts(desktop_dir, log_fn=print):
    log_fn("[UNINSTALL] Removing Desktop shortcuts...")
    shortcuts = [
        "Maiya MVS (Arcade).url",
        "Maiya AES (Console).url",
        "Maiya - Check for Updates.url",
        "Maiya - Flashcart SD Images.url",
        "Maiya - Uninstall.url",
        "Maiya_MVS.desktop",
        "Maiya_AES.desktop",
        "Maiya_Update.desktop",
        "Maiya_SD_Images.desktop",
        "Maiya_Uninstall.desktop"
    ]
    for s in shortcuts:
        p = desktop_dir / s
        if p.exists():
            try:
                p.unlink()
                log_fn(f"  Removed: {s}")
            except Exception as e:
                log_fn(f"  Failed to remove {s}: {e}")

    # Windows Start Menu folder
    sm_dir = get_windows_start_menu_dir()
    if sm_dir and sm_dir.exists():
        try:
            shutil.rmtree(sm_dir)
            log_fn(f"  Removed Windows Start Menu folder: {sm_dir}")
        except Exception as e:
            log_fn(f"  Failed to remove Start Menu folder: {e}")

    # Linux Application menu entries
    user_apps = Path.home() / ".local/share/applications"
    if user_apps.is_dir():
        for f in user_apps.glob("maiya_*.desktop"):
            try:
                f.unlink()
                log_fn(f"  Removed app menu entry: {f.name}")
            except Exception:
                pass

    user_icon = Path.home() / ".local/share/icons/maiya_icon.png"
    if user_icon.exists():
        try:
            user_icon.unlink()
        except Exception:
            pass


def perform_uninstall(target_dir, auto_yes=False, log_fn=print):
    target = Path(target_dir).resolve()
    if not target.exists():
        log_fn(f"[ERROR] Target directory not found: {target}")
        return False

    log_fn("============================================================")
    log_fn(" Eagle Software - Maiya: Super Nature Girl Uninstaller")
    log_fn("============================================================")
    log_fn(f" Target installation folder: {target}")
    log_fn("")

    if not auto_yes:
        try:
            ans = input("Are you sure you want to completely uninstall Maiya? [y/N]: ").strip().lower()
            if ans not in ("y", "yes"):
                log_fn("Uninstallation cancelled by user.")
                return False
        except (KeyboardInterrupt, EOFError):
            log_fn("\nUninstallation cancelled.")
            return False

    # 1. Remove Desktop and Application shortcuts
    desktop_dir = get_desktop_dir()
    remove_shortcuts(desktop_dir, log_fn=log_fn)

    # 2. Remove game directories and files
    log_fn("[UNINSTALL] Deleting game packages, ROMs, BIOS, and launchers...")
    subdirs = ["roms", "hash", "tests", "mame"]
    for s in subdirs:
        p = target / s
        if p.is_dir():
            try:
                shutil.rmtree(p, ignore_errors=True)
                log_fn(f"  Deleted folder: {s}/")
            except Exception as e:
                log_fn(f"  Could not delete {s}/: {e}")

    for f in list(target.glob("run_maiya*")) + list(target.glob("update_maiya*")) + [
        target / "version.json",
        target / "maiya_icon.ico",
        target / "maiya_icon.png",
        target / "card.bin",
        target / "LICENSE",
        target / "README.txt",
        target / "RUN.txt",
        target / "EagleBIOS.md"
    ]:
        if f.exists():
            try:
                f.unlink()
            except Exception:
                pass

    # 3. Clean up the target folder itself
    is_self = Path(__file__).resolve().parent == target
    if not is_self:
        try:
            shutil.rmtree(target, ignore_errors=True)
            log_fn(f"[UNINSTALL] Removed directory: {target}")
        except Exception:
            pass
    else:
        # Self-deletion on Windows: spawn detached worker
        if REAL_WINDOWS or is_wsl():
            try:
                if is_wsl():
                    win_target = subprocess.check_output(["wslpath", "-w", str(target)]).decode().strip()
                else:
                    win_target = str(target)
                cleanup_bat = Path(os.environ.get("TEMP", "/tmp")) / f"maiya_rm_{int(time.time())}.bat"
                bat_code = f"""@echo off
ping 127.0.0.1 -n 2 >nul
rmdir /s /q "{win_target}" >nul 2>&1
del "%~f0" >nul 2>&1
"""
                cleanup_bat.write_text(bat_code)
                if REAL_WINDOWS:
                    subprocess.Popen(["cmd.exe", "/c", str(cleanup_bat)], close_fds=True)
                elif is_wsl():
                    win_cleanup = subprocess.check_output(["wslpath", "-w", str(cleanup_bat)]).decode().strip()
                    subprocess.Popen(["/mnt/c/Windows/System32/cmd.exe", "/c", win_cleanup], close_fds=True)
            except Exception:
                pass

    log_fn("\n============================================================")
    log_fn(" [SUCCESS] Maiya: Super Nature Girl was completely uninstalled.")
    log_fn("============================================================\n")
    return True


def main():
    parser = argparse.ArgumentParser(description="Maiya: Super Nature Girl Uninstaller")
    parser.add_argument("--target", default="", help="Target installation directory to uninstall")
    parser.add_argument("-y", "--yes", action="store_true", help="Non-interactive auto-confirm")
    args = parser.parse_args()

    target_dir = args.target or str(Path(__file__).resolve().parent)
    success = perform_uninstall(target_dir, auto_yes=args.yes)
    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
