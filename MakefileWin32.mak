#######
#https://eaglesoftware.biz
#https://github.com/eaglesoftware777
#https://github.com/eaglesoftware777/neogeosdk
#######

# Force cmd.exe as the shell so built-in commands (mklink, if, del, copy, rmdir) work correctly.
# Without this, GNU make may try to run them directly via CreateProcess and fail.
SHELL = $(COMSPEC)
.SHELLFLAGS = /c

# Game selection — default is demo
# Usage: make -f MakefileWin32.mak GAME=helloworld p1
#        make -f MakefileWin32.mak GAME=tutorial p1
#        make -f MakefileWin32.mak GAME=neogeogame p1
GAME ?= demo
-include games/$(GAME)/game.mk

# Per-game sound folder — defined early so FM_MMLS wildcards resolve correctly
GAME_SOUND = games/$(GAME)/sound

ifndef SDKHOME
SDKHOME := $(abspath $(CURDIR)/..)
endif

# Toolchain selection order:
# - TOOLCHAIN=legacy: force existing M68K_ELF_ROOT fallback (e.g. SysGCC)
# - TOOLCHAIN=auto (default):
#   1) $(SDKHOME)\x-tools-v2-win\m68k-unknown-elf\bin
#   2) $(SDKHOME)\x-tools-v2-win\m68k-elf\bin
#   3) existing M68K_ELF_ROOT fallback (e.g. SysGCC)
TOOLCHAIN?=auto
XTOOLS_V2_WIN_U1:=$(SDKHOME)/x-tools-v2-win/m68k-unknown-elf/bin
XTOOLS_V2_WIN_U2:=$(SDKHOME)/x-tools-v2-win/m68k-elf/bin

ifeq ($(TOOLCHAIN),legacy)
  M68K_ELF_ROOT?=C:\SysGCC\m68k-elf
  M68K_ELF_BIN=$(M68K_ELF_ROOT)\bin
  M68K_ELF_PREFIX=m68k-elf
else
  ifeq ($(wildcard $(XTOOLS_V2_WIN_U1)/m68k-unknown-elf-gcc.exe),)
    ifeq ($(wildcard $(XTOOLS_V2_WIN_U2)/m68k-elf-gcc.exe),)
      M68K_ELF_ROOT?=C:\SysGCC\m68k-elf
      M68K_ELF_BIN=$(M68K_ELF_ROOT)\bin
      M68K_ELF_PREFIX=m68k-elf
    else
      M68K_ELF_BIN:=$(subst /,\,$(XTOOLS_V2_WIN_U2))
      M68K_ELF_PREFIX=m68k-elf
    endif
  else
    M68K_ELF_BIN:=$(subst /,\,$(XTOOLS_V2_WIN_U1))
    M68K_ELF_PREFIX=m68k-unknown-elf
  endif
endif

REPO_WIN=$(subst /,\,$(CURDIR))

CC=$(M68K_ELF_BIN)\$(M68K_ELF_PREFIX)-gcc.exe
CFLAGS= -c  -O0 -fomit-frame-pointer   -Wall  -fno-zero-initialized-in-bss  -march=68000 -mcpu=68000 -mtune=68000 -m68000 -ffreestanding -std=gnu99 -I. -Isdk -Isdk/2d_engine -Igames/$(GAME)/scenes -Wa,-march=68000,-mcpu=68000,-W,--warn
CFLAGS1=-S -O0 -fomit-frame-pointer  -Wall -fno-zero-initialized-in-bss -march=68000  -mcpu=68000 -mtune=68000 -m68000  -ffreestanding
LD=$(M68K_ELF_BIN)\$(M68K_ELF_PREFIX)-ld.exe
LDFLAGS=  -nostdlib
OBJCP=$(M68K_ELF_BIN)\$(M68K_ELF_PREFIX)-objcopy.exe
OBJDUMP=$(M68K_ELF_BIN)\$(M68K_ELF_PREFIX)-objdump.exe
GDB=$(M68K_ELF_BIN)\$(M68K_ELF_PREFIX)-gdb.exe
NM=$(M68K_ELF_BIN)\$(M68K_ELF_PREFIX)-nm.exe
READELF=$(M68K_ELF_BIN)\$(M68K_ELF_PREFIX)-readelf.exe
ADDR2LINE=$(M68K_ELF_BIN)\$(M68K_ELF_PREFIX)-addr2line.exe
SIZE=$(M68K_ELF_BIN)\$(M68K_ELF_PREFIX)-size.exe

WLAZ80?=wla-z80
WLALINK?=wlalink
PY?=py
SOX?=
MAME?=mame
DEBUG?=0
GDB_REMOTE?=localhost:1234

FM_MMLS:=$(wildcard $(GAME_SOUND)/fm/*.mml)
MML_TRACKS:=$(wildcard $(GAME_SOUND)/mml/*.mml)
SSG_MMLS:=$(wildcard $(GAME_SOUND)/ssg/*.mml)

CROP=-crop 0x000000 0x080000
SCAT=$(REPO_WIN)\win\srec_cat.exe
INFO=$(REPO_WIN)\win\xxd.exe -g 2
SWAP= -byte-swap 2 -o
FILL= -fill 0xFF  0x000000 0x080000 -range-padding 4 -o
NG_ENGINE_OBJ0=out\ng_defs0.o out\ng_properties0.o out\ng_game_time0.o out\ng_timers0.o out\ng_progress0.o out\ng_status0.o out\ng_game_events0.o out\ng_level0.o out\ng_bg0.o out\ng_fix0.o out\ng_sprite_group0.o out\ng_actions0.o out\ng_chars0.o out\ng_npcs0.o out\ng_physics0.o out\ng_border_constraints0.o out\ng_game_interupt0.o out\ng_depthfx0.o out\ng_render_queue0.o out\ng_fixed0.o out\ng_camera0.o out\ng_palette_fx0.o out\ng_particles0.o out\ng_feedback0.o out\ng_debug0.o out\ng_demo_advanced0.o
GAME_SCENE_OBJS := $(addprefix out/,$(addsuffix 0.o,$(GAME_SCENES)))
NG_FIX_SDK_OBJ0=out\ng_fix_sdk0.o

ifeq ($(DEBUG),1)
CFLAGS += -g3 -gdwarf-2 -DNG_DEBUG=1
LDFLAGS += -Map=out\game.map
STRIP_SECTS:=-R .comment
else
STRIP_SECTS:=-R .comment -R .text -R .data -R .bss
endif

HASHPATH?=$(REPO_WIN)\hash_eagle\$(GAME);$(REPO_WIN)\hash_eagle;$(REPO_WIN)\hash
# Default BIOS for test/debug. Override: make -f MakefileWin32.mak test BIOS=euro
# Supported values (make -f MakefileWin32.mak bios-list for full table):
#   us  us-e  us-v2  us-u4  us-u3
#   euro  euro-s1  asia-mv1c  asia-mv1b
#   japan  japan-s2  japan-s1  japan-mv1b  japan-j3a  japan-mv1c  japan-hotel
#   unibios40 unibios33 unibios32 unibios31 unibios30
#   unibios23 unibios23o unibios22 unibios21 unibios20
#   unibios13 unibios12 unibios12o unibios11 unibios10
BIOS?=euro
ROM_DIR = roms\$(GAME)
MAME_COMMON=$(MAME) neogeo -rompath $(REPO_WIN)\roms -hashpath "$(HASHPATH)" -bios $(BIOS) -cart1 $(GAME)

# PLATFORM: mvs (default) or aes
PLATFORM?=mvs
ifeq ($(PLATFORM),mvs)
GAME_NEOGEO_C=games\$(GAME)\neogeo_mvs.c
PLATFORM_CFLAGS=-DNG_MVS=1
else
GAME_NEOGEO_C=games\$(GAME)\neogeo_aes.c
PLATFORM_CFLAGS=-DNG_AES=1
endif

.DEFAULT_GOAL := p1

.PHONY: all
all: art sfix sound p1

.PHONY: aes
aes:
	$(MAKE) -f MakefileWin32.mak PLATFORM=aes GAME=$(GAME) p1

.PHONY: mvs
mvs:
	$(MAKE) -f MakefileWin32.mak PLATFORM=mvs GAME=$(GAME) p1

.PHONY: p1
p1: game $(GAME_ID)-p1.p1

game:
	$(CC) $(CFLAGS) $(PLATFORM_CFLAGS) $(GAME_NEOGEO_C) -o out\neogeo0.o
	$(CC) $(CFLAGS) $(PLATFORM_CFLAGS) games\$(GAME)\user.c -o out\user0.o
	$(CC) $(CFLAGS) games\$(GAME)\main.c -o out\main0.o
	$(CC) $(CFLAGS) sdk\neogeolib.c -o out\neogeolib0.o
	$(CC) $(CFLAGS) sdk\ng_fix\ng_fix.c -o out\ng_fix_sdk0.o
	$(CC) $(CFLAGS) sdk\2d_engine\ng_defs.c -o out\ng_defs0.o
	$(CC) $(CFLAGS) sdk\2d_engine\ng_properties.c -o out\ng_properties0.o
	$(CC) $(CFLAGS) sdk\2d_engine\ng_game_time.c -o out\ng_game_time0.o
	$(CC) $(CFLAGS) sdk\2d_engine\ng_timers.c -o out\ng_timers0.o
	$(CC) $(CFLAGS) sdk\2d_engine\ng_progress.c -o out\ng_progress0.o
	$(CC) $(CFLAGS) sdk\2d_engine\ng_status.c -o out\ng_status0.o
	$(CC) $(CFLAGS) sdk\2d_engine\ng_game_events.c -o out\ng_game_events0.o
	$(CC) $(CFLAGS) sdk\2d_engine\ng_level.c -o out\ng_level0.o
	$(CC) $(CFLAGS) sdk\2d_engine\ng_bg.c -o out\ng_bg0.o
	$(CC) $(CFLAGS) sdk\2d_engine\ng_fix.c -o out\ng_fix0.o
	$(CC) $(CFLAGS) sdk\2d_engine\ng_sprite_group.c -o out\ng_sprite_group0.o
	$(CC) $(CFLAGS) sdk\2d_engine\ng_actions.c -o out\ng_actions0.o
	$(CC) $(CFLAGS) sdk\2d_engine\ng_chars.c -o out\ng_chars0.o
	$(CC) $(CFLAGS) sdk\2d_engine\ng_npcs.c -o out\ng_npcs0.o
	$(CC) $(CFLAGS) sdk\2d_engine\ng_physics.c -o out\ng_physics0.o
	$(CC) $(CFLAGS) sdk\2d_engine\ng_border_constraints.c -o out\ng_border_constraints0.o
	$(CC) $(CFLAGS) sdk\2d_engine\ng_game_interupt.c -o out\ng_game_interupt0.o
	$(CC) $(CFLAGS) sdk\2d_engine\ng_depthfx.c -o out\ng_depthfx0.o
	$(CC) $(CFLAGS) sdk\2d_engine\ng_render_queue.c -o out\ng_render_queue0.o
	$(CC) $(CFLAGS) sdk\2d_engine\ng_fixed.c -o out\ng_fixed0.o
	$(CC) $(CFLAGS) sdk\2d_engine\ng_camera.c -o out\ng_camera0.o
	$(CC) $(CFLAGS) sdk\2d_engine\ng_palette_fx.c -o out\ng_palette_fx0.o
	$(CC) $(CFLAGS) sdk\2d_engine\ng_particles.c -o out\ng_particles0.o
	$(CC) $(CFLAGS) sdk\2d_engine\ng_feedback.c -o out\ng_feedback0.o
	$(CC) $(CFLAGS) sdk\2d_engine\ng_debug.c -o out\ng_debug0.o
	$(CC) $(CFLAGS) sdk\2d_engine\ng_demo_advanced.c -o out\ng_demo_advanced0.o
	$(if $(GAME_SCENES),for %%f in ($(GAME_SCENES)) do $(CC) $(CFLAGS) games\$(GAME)\scenes\%%f.c -o out\%%f0.o)
	$(CC) $(CFLAGS) games\$(GAME)\eyecatcher.c -o out\eyecatcher0.o
	$(OBJCP) $(STRIP_SECTS) out\neogeo0.o out\neogeo.o
	$(OBJCP) $(STRIP_SECTS) out\user0.o out\user.o
	$(OBJCP) $(STRIP_SECTS) out\main0.o out\main.o
	$(OBJCP) $(STRIP_SECTS) out\neogeolib0.o out\neogeolib.o
	$(OBJCP) $(STRIP_SECTS) out\eyecatcher0.o out\eyecatcher.o
	$(LD) $(LDFLAGS) -T games/$(GAME)/neogeo.ld -o out\game out\neogeo.o out\user.o out\main.o out\neogeolib.o out\eyecatcher.o $(NG_FIX_SDK_OBJ0) $(NG_ENGINE_OBJ0) $(GAME_SCENE_OBJS)

$(GAME_ID)-p1.p1: game
	$(OBJCP) -O ihex out\game out\game0
	$(SCAT) out\game0 -Intel $(CROP) -o out\game0.rom -binary
	$(SCAT) out\game0.rom -binary $(SWAP) out\game1.rom -binary
	$(SCAT) out\game1.rom -binary $(FILL) out\game.rom -binary
	copy /Y out\game.rom out\$(GAME_ID)-p1.p1
	if not exist $(ROM_DIR) mkdir $(ROM_DIR)
	copy /Y out\$(GAME_ID)-p1.p1 $(ROM_DIR)\$(GAME_ID)-p1.p1
	set GAME=$(GAME)&& set GAME_ID=$(GAME_ID)&& $(PY) hash_eagle\gen_hash.py

.PHONY: hash
hash:
	$(PY) hash_eagle\gen_hash.py

.PHONY: samples
samples:
	$(if $(wildcard $(GAME_SOUND)/samples/in_wav_a),cd sound\tools && set PY=$(PY)&& set SOX=$(SOX)&& set GAME_SOUND=..\..\$(subst /,\,$(GAME_SOUND))&& call enc_wave16le_a.bat,@echo samples: no in_wav_a in $(GAME_SOUND)\samples\, skipping a)
	$(if $(wildcard $(GAME_SOUND)/samples/in_wav_b),cd sound\tools && set PY=$(PY)&& set SOX=$(SOX)&& set GAME_SOUND=..\..\$(subst /,\,$(GAME_SOUND))&& call enc_wave16le_b.bat,@echo samples: no in_wav_b in $(GAME_SOUND)\samples\, skipping b)
	cd sound\tools && set PY=$(PY)&& set GAME_SOUND=..\..\$(subst /,\,$(GAME_SOUND))&& call adpcm_enc_process.bat

.PHONY: vrom
vrom:
	set GAME=$(GAME)&& set GAME_ID=$(GAME_ID)&& call sound\tools\vrom.bat
	if not exist $(ROM_DIR) mkdir $(ROM_DIR)
	copy /Y out\$(GAME_ID)-v1.v1 $(ROM_DIR)\$(GAME_ID)-v1.v1

.PHONY: fmpatches
fmpatches:
	$(if $(wildcard $(GAME_SOUND)/fm/patches.fm),$(PY) sound\tools\fm_patch_compile.py $(GAME_SOUND)\fm\patches.fm -o sound\driver\fm_patch_table.inc,@echo fmpatches: no patches.fm in $(GAME_SOUND)\fm\, skipping)

.PHONY: fm
fm:
	$(if $(FM_MMLS),$(PY) sound/tools/fm_compile.py $(FM_MMLS) -o sound/driver/fm_data.inc,@echo fm: no MML files in $(GAME_SOUND)\fm\, skipping)

.PHONY: mml
mml:
	$(if $(MML_TRACKS),$(PY) sound/tools/mml_compile.py $(MML_TRACKS) -o sound/driver/music_data.inc,@echo mml: no MML files in $(GAME_SOUND)\mml\, skipping)

.PHONY: ssgconfig
ssgconfig:
	$(if $(wildcard $(GAME_SOUND)/ssg/config.ssg),$(PY) sound\tools\ssg_config_compile.py $(GAME_SOUND)\ssg\config.ssg -o sound\driver\ssg_config.inc,@echo ssgconfig: no config.ssg in $(GAME_SOUND)\ssg\, skipping)

.PHONY: ssg
ssg:
	$(if $(SSG_MMLS),$(PY) sound/tools/ssg_compile.py $(SSG_MMLS) -o sound/driver/ssg_data.inc,@echo ssg: no MML files in $(GAME_SOUND)\ssg\, skipping)

.PHONY: m1rom
m1rom: fmpatches fm mml ssgconfig ssg
	set GAME=$(GAME)&& set GAME_ID=$(GAME_ID)&& set GAME_SOUND=$(subst /,\,$(GAME_SOUND))&& set WLAZ80=$(WLAZ80)&& set WLALINK=$(WLALINK)&& set USE_Z80C=$(USE_Z80C)&& set Z80C_SRC=$(Z80C_SRC_WIN)&& call sound\tools\m1rom.bat

.PHONY: m1rom-asm
m1rom-asm:
	$(MAKE) -f MakefileWin32.mak GAME=$(GAME) m1rom USE_Z80C=0
	if not exist out\compare mkdir out\compare
	copy /Y out\$(GAME_ID)-m1.m1 out\compare\$(GAME_ID)-m1-asm.m1

.PHONY: m1rom-c
m1rom-c:
	$(MAKE) -f MakefileWin32.mak GAME=$(GAME) m1rom USE_Z80C=1 LINK_C_DRIVER=1
	if not exist out\compare mkdir out\compare
	copy /Y out\$(GAME_ID)-m1.m1 out\compare\$(GAME_ID)-m1-c.m1

.PHONY: compare-driver
compare-driver: m1rom-asm m1rom-c
	$(PY) sound\tools\compare_m1.py out\compare\$(GAME_ID)-m1-asm.m1 out\compare\$(GAME_ID)-m1-c.m1

.PHONY: sound
sound: samples vrom fmpatches fm mml ssgconfig ssg m1rom

.PHONY: sound-all
sound-all: sound

.PHONY: sfix
sfix:
	if exist artbox\infix rmdir artbox\infix
	mklink /J artbox\infix $(REPO_WIN)\games\$(GAME)\artbox\infix
	cd artbox && set GAME=$(GAME)&& set GAME_ID=$(GAME_ID)&& $(PY) romdbfiximport.py && $(PY) fixtiles.py
	rmdir artbox\infix
	if not exist $(ROM_DIR) mkdir $(ROM_DIR)
	copy /Y artbox\$(GAME_ID)-s1.s1 $(ROM_DIR)\$(GAME_ID)-s1.s1

.PHONY: srom
srom: sfix

.PHONY: art-clean
art-clean:
	call artbox\makeclean.bat

.PHONY: art
art:
	set GAME_ID=$(GAME_ID)&& call artbox\makeartbox.bat $(GAME)

.PHONY: dist
dist: p1
	$(PY) hash_eagle\gen_hash.py --dist

.PHONY: clean
clean:
	if exist out\game del /Q out\game
	if exist out\game0 del /Q out\game0
	if exist out\game0.rom del /Q out\game0.rom
	if exist out\game1.rom del /Q out\game1.rom
	if exist out\game.rom del /Q out\game.rom
	if exist out\$(GAME_ID)-p1.p1 del /Q out\$(GAME_ID)-p1.p1
	if exist out\game.map del /Q out\game.map
	if exist out\*.o del /Q out\*.o
	if exist out\*.s del /Q out\*.s
	if exist dump\*.dump del /Q dump\*.dump
	if exist dump\*.hex del /Q dump\*.hex
	if exist dump\*.txt del /Q dump\*.txt
	if exist dump\*.sym del /Q dump\*.sym
	if exist dump\*.gdb del /Q dump\*.gdb
	if exist dump\*.readelf del /Q dump\*.readelf
	if exist $(ROM_DIR)\$(GAME_ID)-p1.p1 del /Q $(ROM_DIR)\$(GAME_ID)-p1.p1

.PHONY: sound-clean
sound-clean:
	if exist out\$(GAME_ID)-m1.m1 del /Q out\$(GAME_ID)-m1.m1
	if exist out\$(GAME_ID)-v1.v1 del /Q out\$(GAME_ID)-v1.v1
	if exist out\driver.gen.asm del /Q out\driver.gen.asm
	if exist $(ROM_DIR)\$(GAME_ID)-m1.m1 del /Q $(ROM_DIR)\$(GAME_ID)-m1.m1
	if exist $(ROM_DIR)\$(GAME_ID)-v1.v1 del /Q $(ROM_DIR)\$(GAME_ID)-v1.v1
	if exist sound\samples\out_16el_a\*.wav del /Q sound\samples\out_16el_a\*.wav
	if exist sound\samples\out_16el_b\*.wav del /Q sound\samples\out_16el_b\*.wav
	if exist sound\samples\out_a\*.adpcma del /Q sound\samples\out_a\*.adpcma
	if exist sound\samples\out_b\*.adpcmb del /Q sound\samples\out_b\*.adpcmb
	if exist sound\driver\fm_data.inc del /Q sound\driver\fm_data.inc
	if exist sound\driver\music_data.inc del /Q sound\driver\music_data.inc
	if exist sound\driver\fm_patch_table.inc del /Q sound\driver\fm_patch_table.inc
	if exist sound\driver\sample_table.inc del /Q sound\driver\sample_table.inc
	if exist sound\driver\ssg_config.inc del /Q sound\driver\ssg_config.inc
	if exist sound\driver\ssg_data.inc del /Q sound\driver\ssg_data.inc

.PHONY: clean-all
clean-all: clean sound-clean art-clean
	if exist $(ROM_DIR)\$(GAME_ID)-c1.c1 del /Q $(ROM_DIR)\$(GAME_ID)-c1.c1
	if exist $(ROM_DIR)\$(GAME_ID)-c2.c2 del /Q $(ROM_DIR)\$(GAME_ID)-c2.c2

.PHONY: dump
dump:
	$(OBJDUMP) -Dht out\neogeo.o | more
	$(OBJDUMP) -Dht out\user.o | more
	$(OBJDUMP) -Dht out\main.o | more
	$(OBJDUMP) -Dht out\game | more
	$(OBJDUMP) -Dht out\neogeo.o > dump\neogeo.dump
	$(OBJDUMP) -Dht out\user.o > dump\user.dump
	$(OBJDUMP) -Dht out\main.o > dump\main.dump
	$(OBJDUMP) -Dht out\game > dump\game.dump
	$(INFO) out\game.rom | more
	$(INFO) out\game.rom > dump\game.hex

.PHONY: test
test:
	set GAME=$(GAME)&& set GAME_ID=$(GAME_ID)&& $(PY) hash_eagle\gen_hash.py
	if not exist $(ROM_DIR) mkdir $(ROM_DIR)
	copy /Y out\$(GAME_ID)-p1.p1 $(ROM_DIR)\$(GAME_ID)-p1.p1
	$(MAME_COMMON) -output console -nofilter -waitvsync -window

.PHONY: test-aes
test-aes:
	$(MAKE) -f MakefileWin32.mak PLATFORM=aes GAME=$(GAME) test

.PHONY: test-mvs
test-mvs:
	$(MAKE) -f MakefileWin32.mak PLATFORM=mvs GAME=$(GAME) test

.PHONY: bios-list
bios-list:
	@echo Supported BIOS values for: make -f MakefileWin32.mak test BIOS=^<name^>
	@echo.
	@echo   euro             Europe MVS (Ver. 2)
	@echo   euro-s1          Europe MVS (Ver. 1)
	@echo   asia-mv1c        Asia NEO-MVH MV1C
	@echo   asia-mv1b        Asia MV1B
	@echo   us               US MVS (Ver. 2?)          [default]
	@echo   us-e             US MVS (Ver. 1)
	@echo   us-v2            US MVS (4 slot, Ver 2)
	@echo   us-u4            US MVS (U4)
	@echo   us-u3            US MVS (U3)
	@echo   japan            Japan MVS (Ver. 3)
	@echo   japan-s2         Japan MVS (Ver. 2)
	@echo   japan-s1         Japan MVS (Ver. 1)
	@echo   japan-mv1b       Japan MV1B
	@echo   japan-j3a        Japan MVS (J3, alt)
	@echo   japan-mv1c       Japan NEO-MVH MV1C
	@echo   japan-hotel      Custom Japanese Hotel
	@echo   unibios40        Universe BIOS (Hack, Ver. 4.0)
	@echo   unibios33        Universe BIOS (Hack, Ver. 3.3)
	@echo   unibios32        Universe BIOS (Hack, Ver. 3.2)
	@echo   unibios31        Universe BIOS (Hack, Ver. 3.1)
	@echo   unibios30        Universe BIOS (Hack, Ver. 3.0)
	@echo   unibios23        Universe BIOS (Hack, Ver. 2.3)
	@echo   unibios23o       Universe BIOS (Hack, Ver. 2.3, older?)
	@echo   unibios22        Universe BIOS (Hack, Ver. 2.2)
	@echo   unibios21        Universe BIOS (Hack, Ver. 2.1)
	@echo   unibios20        Universe BIOS (Hack, Ver. 2.0)
	@echo   unibios13        Universe BIOS (Hack, Ver. 1.3)
	@echo   unibios12        Universe BIOS (Hack, Ver. 1.2)
	@echo   unibios12o       Universe BIOS (Hack, Ver. 1.2, older)
	@echo   unibios11        Universe BIOS (Hack, Ver. 1.1)
	@echo   unibios10        Universe BIOS (Hack, Ver. 1.0)

.PHONY: debug
debug:
	set GAME=$(GAME)&& set GAME_ID=$(GAME_ID)&& $(PY) hash_eagle\gen_hash.py
	if not exist $(ROM_DIR) mkdir $(ROM_DIR)
	copy /Y out\$(GAME_ID)-p1.p1 $(ROM_DIR)\$(GAME_ID)-p1.p1
	$(MAME_COMMON) -output console -debug -verbose -nofilter -waitvsync -window

.PHONY: debug-aes
debug-aes:
	$(MAKE) -f MakefileWin32.mak PLATFORM=aes GAME=$(GAME) debug

.PHONY: mame-trace
mame-trace: p1
	if not exist dump mkdir dump
	set GAME=$(GAME)&& set GAME_ID=$(GAME_ID)&& $(PY) hash_eagle\gen_hash.py
	if not exist $(ROM_DIR) mkdir $(ROM_DIR)
	copy /Y out\$(GAME_ID)-p1.p1 $(ROM_DIR)\$(GAME_ID)-p1.p1
	$(NM) -n out\game > dump\game.sym
	$(OBJDUMP) -Dht out\game > dump\game.debug.dump
	$(MAME_COMMON) -verbose -debug -debugscript dump\mame_trace.mds
	@echo Trace: dump\m68k_trace.txt  ^|  Symbols: dump\game.sym  ^|  Disasm: dump\game.debug.dump

.PHONY: debug-build
debug-build:
	$(MAKE) -f MakefileWin32.mak GAME=$(GAME) DEBUG=1 p1
	$(MAKE) -f MakefileWin32.mak GAME=$(GAME) DEBUG=1 debug-artifacts

.PHONY: debug-artifacts
debug-artifacts: out\game
	if not exist dump mkdir dump
	if exist dump\game.size.txt del /Q dump\game.size.txt
	if exist dump\game.sym del /Q dump\game.sym
	if exist dump\game.readelf del /Q dump\game.readelf
	if exist dump\game.debug.dump del /Q dump\game.debug.dump
	if exist dump\game.map del /Q dump\game.map
	$(SIZE) out\game > dump\game.size.txt
	$(NM) -n out\game > dump\game.sym
	$(READELF) -a out\game > dump\game.readelf
	$(OBJDUMP) -DhtS out\game > dump\game.debug.dump
	if exist out\game.map copy /Y out\game.map dump\game.map

.PHONY: gdb-script
gdb-script:
	if not exist dump mkdir dump
	@echo set pagination off> dump\gdb_trace.gdb
	@echo set confirm off>> dump\gdb_trace.gdb
	@echo file out/game>> dump\gdb_trace.gdb
	@echo info files>> dump\gdb_trace.gdb
	@echo info functions>> dump\gdb_trace.gdb
	@echo info variables>> dump\gdb_trace.gdb
	@echo maintenance info sections>> dump\gdb_trace.gdb
	@echo quit>> dump\gdb_trace.gdb

.PHONY: gdb-trace
gdb-trace: debug-build gdb-script
	$(GDB) --version > NUL 2> dump\gdb_trace.err && $(GDB) -batch -x dump\gdb_trace.gdb > dump\gdb_trace.txt 2>> dump\gdb_trace.err || echo GDB unavailable: $(GDB)> dump\gdb_trace.txt

.PHONY: gdb
gdb: debug-build
	$(GDB) out\game

.PHONY: gdb-remote
gdb-remote: debug-build
	$(GDB) -ex "target remote $(GDB_REMOTE)" out\game
