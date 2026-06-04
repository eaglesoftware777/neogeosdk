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
| Cross-compiler   | `x-tools-v2/m68k-unknown-elf` (release tarball)          | `x-tools-v2-win` (release tarball) **or** `C:\SysGCC\m68k-elf` |
| Z80 assembler    | `wla-z80`, `wlalink` (built from `vhelin/wla-dx` source) | `vhelin.wla-dx` winget package                  |
| Python           | system `python3` + `numpy`, `pillow`, `pypng`            | `py` launcher + `numpy`, `pillow`, `pypng`      |
| Emulator         | `mame` (system package)                                  | `MAMEDev.MAME` winget package                   |
| Optional         | `sox`                                                    | `ChrisAnt996.SoX`                               |

Both cross-compiler tarballs are fetched from the **v1.3.0** release:
<https://github.com/eaglesoftware777/neogeosdk/releases/tag/v1.3.0>
(`x-tools-v2.tar` for Linux, `x-tools-v2-win.tar` for Windows).

The SDK itself is cloned from the **`neo_universal_2d`** branch (the v1.3.x line).

Each installer writes / persists `SDKHOME` so subsequent shells can find the
toolchain layout the makefiles expect (`$SDKHOME/x-tools-v2`, `$SDKHOME/neogeosdk`).

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
| `--prefix DIR`      | Root holding `neogeosdk/` + `x-tools-v2/` (default `$HOME/neogeo`) |
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
- **`m68k-elf-gcc` not found after `install-windows.bat`** — the script auto-fetches `x-tools-v2-win.tar` from the v1.3.0 release when `curl` and `tar` are both on `PATH` (Win10 1803+ / Win11 ship both by default).  If that step is skipped, install [SysGCC m68k-elf](http://gnutoolchains.com/m68k-elf/) or download the tarball manually from the [v1.3.0 release page](https://github.com/eaglesoftware777/neogeosdk/releases/tag/v1.3.0) and extract it into `%SDKHOME%\x-tools-v2-win`.
- **WSL `pypng` install needs `--break-system-packages` on Ubuntu 24.04** — re-run with `python3 -m pip install --user --break-system-packages pypng`.
- **WSL PulseAudio for MAME audio** — see the main README's "WSL2 + Ubuntu 24.04 + PulseAudio" section.
- **Wrong branch checked out** — installers default to `neo_universal_2d`.  Override with `git -C "$SDKHOME/neogeosdk" checkout <other-branch>` after the install completes.
