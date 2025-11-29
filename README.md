# Neo Geo SDK

Neo Geo Development Kit for SNK Hardware <br/>

[https://github.com/eaglesoftware777](https://github.com/eaglesoftware777) <br/>
[https://github.com/eaglesoftware777/neogeosdk](https://github.com/eaglesoftware777/neogeosdk) <br/><br/>

The Neo Geo SDK constitutes a complete and self-contained development environment for the creation of software for the original Neo Geo arcade and home console systems. The toolchain adheres to the low-level programming methodology characteristic of early-1990s Neo Geo production workflows while incorporating modern build automation and rapid verification through the MAME emulator.
It grants the developer direct, unabstracted control over VRAM, DMA operations, palette registers, command lists, audio subsystems, and the deterministic construction of ROM images. <br/><br/>

*The purpose of this SDK is to maintain the practice of authentic Neo-Geo development — hardware-centered, low-level, and performance-critical — and to ensure that it remains accessible to current and future developers.* <br/><br/>

---

## Requirements

<br/>

Ubuntu Linux  ⇒  [https://ubuntu.com/](https://ubuntu.com/) <br/>
m68k compiler ⇒  [https://github.com/eaglesoftware777/neogeosdk/releases/download/v1.0/x-tools.tar](https://github.com/eaglesoftware777/neogeosdk/releases/download/v1.0/x-tools.tar) <br/>
mame          ⇒  [https://www.mamedev.org/](https://www.mamedev.org/) <br/>
srecord       ⇒  [https://packages.ubuntu.com/search?keywords=srecord](https://packages.ubuntu.com/search?keywords=srecord) <br/>
python 2.7    ⇒  [https://www.python.org/download/releases/2.7/](https://www.python.org/download/releases/2.7/) <br/>
numpy         ⇒  [https://numpy.org/](https://numpy.org/) <br/>
pypng         ⇒  [https://pypi.org/project/pypng/](https://pypi.org/project/pypng/) <br/>
sqlite3       ⇒  [https://www.sqlite.org/index.html](https://www.sqlite.org/index.html) <br/><br/>

Optional: <br/>
Geany IDE  ⇒  [https://www.geany.org/](https://www.geany.org/) <br/><br/>

---

## Installation on Linux

<br/>

```
sudo apt-get install mame
sudo apt-get install srecord
sudo apt-get install python2
pip install numpy
pip install pypng
```

Place the SDK in `~/neogeosdk` <br/>
Place the m68k toolchain in `~/x-tools` <br/><br/>

---

## Compilation / Test / Debug (Linux)

<br/>

```
cd ~/neogeosdk
compile      : make
test         : make test
debug        : make debug
dump         : make dump
artbox       : make art
clean        : make clean
artbox clean : make art-clean
```

<br/><br/>

---

# Windows

<br/>

Install Sysprogs m68k compiler:
[https://sysprogs.com/files/gnutoolchains/m68k-elf/m68k-elf-gcc4.8.0.exe](https://sysprogs.com/files/gnutoolchains/m68k-elf/m68k-elf-gcc4.8.0.exe) <br/>

Install Python 2.7:
[https://www.python.org/ftp/python/](https://www.python.org/ftp/python/) <br/><br/>

Install required Python packages:

```
py -2.7 -m pip install numpy
py -2.7 -m pip install pypng
py -2.7 -m pip install sqlite3
```

Specify the SDK path in the Makefile and in the Artbox batch files: <br/>
SDKHOME = directory containing the Neo Geo SDK <br/>
Default setting on Windows systems: <br/>
SDKHOME=C: <br/>
Full SDK path: `C:\neogeosdk` <br/><br/>

`srec_cat.exe` and `xxd.exe` are included in `neogeosdk\win` <br/>
`srec_cat.exe` : [http://srecord.sourceforge.net/download.html](http://srecord.sourceforge.net/download.html) <br/>
`xxd.exe` : [https://sourceforge.net/projects/xxd-for-windows/](https://sourceforge.net/projects/xxd-for-windows/) <br/><br/>

Install or extract MAME in a directory of your choice.
The Makefile is configured to use `%SDKHOME%\mame\mame.exe`.
This path may be modified if a different location is preferred. <br/><br/>

---

## Compilation / Test / Debug (Windows)

<br/>

```
cd %SDKHOME%\neogeosdk
compile : c:\SysGCC\m68k-elf\bin\make -f MakefileWin32.mak
test    : c:\SysGCC\m68k-elf\bin\make -f MakefileWin32.mak test
debug   : c:\SysGCC\m68k-elf\bin\make -f MakefileWin32.mak debug
dump    : c:\SysGCC\m68k-elf\bin\make -f MakefileWin32.mak dump
artbox  : c:\SysGCC\m68k-elf\bin\make -f MakefileWin32.mak art
clean   : c:\SysGCC\m68k-elf\bin\make -f MakefileWin32.mak clean
artbox clean : c:\SysGCC\m68k-elf\bin\make -f MakefileWin32.mak art-clean
```

<br/><br/>

---

## Repository File Overview

<br/>

**Core source**
`main.c` — demonstration entry point illustrating palette initialization and sprite animation <br/>
`user.c` — module intended for user-defined game logic <br/>
`sdk/macro.h` — hardware register definitions, palette indices, VRAM regions, and command codes <br/>
`sdk/neogeo.c`, `sdk/neogeolib.c` — rendering and hardware assistance library <br/>
`sdk/neogeo.ld`, `sdk/neogeo_win.ld` — linker scripts for Linux and Windows environments <br/><br/>

**Soft-Float library**
`softfloat/*.c`, `softfloat/*.S`, `softfloat/*.o`, `soft-float.a` <br/><br/>

**Artbox graphics and ROM asset pipeline**
`artbox/artbox.bat`, `artbox/artbox.sh`, `makeartbox.*`, `romts.*`, `romfx.*`, `clean.*` <br/>
`artbox/romtool` — binary utility used internally by the Artbox workflow <br/>
Input assets located within `artbox/in/` and `artbox/infix/` <br/>
Documentation included under `artbox/readme` <br/><br/>

**Sound and music subsystem**
`sound/ssg/config.ssg`, `sound/fm/config.fm`, `sound/driver/driver.asm` <br/>
`sound/tools/*` — ADPCM encoding and VROM generation utilities <br/>
`sound/samples/*` — WAV and ADPCM audio sample material <br/>
`sound/m1/m1.asm` — M1 sound program <br/>
`sound/mml/test.mml` — Music Macro Language example <br/><br/>

**ROM sample**
`roms/ssideki` — complete demonstration game ROM set <br/><br/>

**Windows helper tools**
`win/romtool_x64.exe`, `win/srec_cat.exe` <br/><br/>

---
