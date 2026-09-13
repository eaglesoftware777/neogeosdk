# NeoGeo SDK installers

One-shot installers for every supported host.  Pick the one that matches
your environment.

| Host                                            | Script              | Notes                                                  |
|-------------------------------------------------|---------------------|--------------------------------------------------------|
| Generic Linux (apt / dnf / yum / pacman / zypper) | `install-linux.sh`  | Auto-detects the package manager.                      |
| Ubuntu / Debian                                 | `install-ubuntu.sh` | Apt fast path matching the README's documented commands. |
| Native Windows 11 (cmd.exe, MSYS-free)          | `install-windows.bat` | Uses `winget` for everything except the m68k compiler. |
| Windows + WSL (Ubuntu)                          | `install-wsl.bat`   | Sets up WSL, drops into Ubuntu, runs the Ubuntu installer. |

All installers are idempotent — re-running is safe; already-installed
components are detected and skipped.

## What gets installed

Every installer ends in the same place:

| Component        | Linux/WSL                                                | Windows                                         |
|------------------|----------------------------------------------------------|-------------------------------------------------|
| Compilers / make | `build-essential`, `cmake`, `make`, `srecord`            | `Git.Git`, `GnuWin32.Make`, `srecord.srecord`   |
| Cross-compiler   | `x-tools-v3/m68k-unknown-elf` (release archive)          | `x-tools-v3-win` (release archive) **or** `C:\SysGCC\m68k-elf` |
| Z80 assembler    | `wla-z80`, `wlalink` (built from `vhelin/wla-dx` source) | `vhelin.wla-dx` winget package                  |
| Python           | system `python3` + `numpy`, `pillow`, `pypng`            | `py` launcher + `numpy`, `pillow`, `pypng`      |
| Emulator         | `mame` (system package)                                  | `MAMEDev.MAME` winget package                   |
| Optional         | `sox`                                                    | `ChrisAnt996.SoX`                               |

The cross-compiler archives are fetched from the latest release
(`x-tools-v3.tar.xz` for Linux, `x-tools-v3-win.zip` for Windows), with the
`x-tools-v2` archives of the v1.3.0 release as the fallback.

The SDK itself is cloned from the **`neo_universal_2d`** branch (the v1.7.0 line).

Each installer writes / persists `SDKHOME` so subsequent shells can find the
toolchain layout the makefiles expect (`$SDKHOME/x-tools-v3`, `$SDKHOME/neogeosdk`).

## Quick start

### Generic Linux

```bash
cd install
chmod +x install-linux.sh
./install-linux.sh --yes
. ~/neogeo/setenv.sh
cd ~/neogeo/neogeosdk
make all && make test
```

### Ubuntu / Debian

```bash
cd install
chmod +x install-ubuntu.sh
./install-ubuntu.sh --yes
. ~/neogeo/setenv.sh
cd ~/neogeo/neogeosdk
make all && make test
```

### Native Windows 11

Run from an **elevated** `cmd.exe` (Right-click → Run as administrator):

```bat
cd install
install-windows.bat
cd /d %USERPROFILE%\neogeo\neogeosdk
make -f MakefileWin32.mak all
make -f MakefileWin32.mak test
```

### WSL (Ubuntu) on Windows

Run from an **elevated** `cmd.exe`:

```bat
cd install
install-wsl.bat
```

The first invocation may install the WSL feature itself and require a
reboot; re-run after rebooting to finish the install.  The script forwards
to `install-ubuntu.sh` inside the distro automatically.

## Common flags

| Flag                | install-linux.sh / install-ubuntu.sh                   |
|---------------------|--------------------------------------------------------|
| `--prefix DIR`      | Root holding `neogeosdk/` + `x-tools-v3/` (default `$HOME/neogeo`) |
| `--no-toolchain`    | Skip the m68k cross toolchain download                 |
| `--no-wladx`        | Skip the WLA-DX build                                  |
| `--yes` / `-y`      | Non-interactive apt/dnf/pacman                         |

`install-windows.bat` takes an optional first positional argument that
overrides `SDKHOME` (default `%USERPROFILE%\neogeo`).

`install-wsl.bat` takes two optional positional arguments:
1. WSL distro name (default `Ubuntu`)
2. Prefix path inside the distro (default `~/neogeo`)

## Verifying after install

Every installer ends with a smoke-test block that prints `OK` or `MISSING`
for each required tool.  If anything's `MISSING`, install it by hand and
re-run the installer — it will only retry the missing pieces.

## Troubleshooting

- **`winget` not found on Windows** — install "App Installer" from the Microsoft Store.
- **`m68k-unknown-elf-gcc` not found after `install-windows.bat`** — the script auto-fetches `x-tools-v3-win.zip` from the latest release (falling back to `x-tools-v2-win.tar` from v1.3.0) when `curl` and `tar` are both on `PATH` (Win10 1803+ / Win11 ship both by default).  If that step is skipped, download `x-tools-v3-win.zip` manually from the release page and extract it into `%SDKHOME%` so that `%SDKHOME%\x-tools-v3-win\m68k-unknown-elf\bin` exists, or install [SysGCC m68k-elf](http://gnutoolchains.com/m68k-elf/) as the fallback.
- **WSL `pypng` install needs `--break-system-packages` on Ubuntu 24.04** — re-run with `python3 -m pip install --user --break-system-packages pypng`.
- **WSL PulseAudio for MAME audio** — see the main README's "WSL2 + Ubuntu 24.04 + PulseAudio" section.
- **Wrong branch checked out** — installers default to `neo_universal_2d`.  Override with `git -C "$SDKHOME/neogeosdk" checkout <other-branch>` after the install completes.
