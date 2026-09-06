#######
#https://eaglesoftware.biz
#https://github.com/eaglesoftware777
#https://github.com/eaglesoftware777/neogeosdk
#######

# Force cmd.exe as the shell so built-in commands (mklink, if, del, copy, rmdir) work correctly.
# Without this, GNU make may try to run them directly via CreateProcess and fail.
ifeq ($(strip $(COMSPEC)),)
COMSPEC := cmd.exe
endif
SHELL = $(COMSPEC)
.SHELLFLAGS = /c

# Game selection from game.cfg (override with GAME=...).
# Usage: make -f MakefileWin32.mak GAME=helloworld p1
#        make -f MakefileWin32.mak GAME=tutorial p1
#        make -f MakefileWin32.mak GAME=neogeogame p1
GAME_CFG_FILE ?= game.cfg
-include $(GAME_CFG_FILE)
ifeq ($(strip $(GAME)),)
  ifneq ($(strip $(CURRENT_GAME)),)
    GAME := $(strip $(CURRENT_GAME))
  else
    GAME := demo
  endif
endif
-include games/$(GAME)/game.mk

# Per-game sound folder — defined early so FM_MMLS wildcards resolve correctly.
# GAME_SOUND_FROM lets a game.mk name another game to take its tracks and
# samples from; the M1/V ROMs are still emitted under this game's own ID.
GAME_SOUND_FROM ?=
GAME_SOUND = games/$(or $(GAME_SOUND_FROM),$(GAME))/sound

# Optional per-game extra include paths and art source, matching the Linux
# Makefile: GAME_EXTRA_INCLUDES adds include directories, GAME_ART_FROM names
# another game whose artbox/in and artbox/infix supply the source artwork.
GAME_EXTRA_INCLUDES ?=
GAME_ART_FROM ?=

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

# 2D engine variant:
#   USE_2D_PLUS=0  (default) — compile sdk\2d_engine   (C, std=gnu99)
#   USE_2D_PLUS=1            — compile sdk\2d_engine_plus (C++14, freestanding)
USE_2D_PLUS ?= 0

ifeq ($(USE_2D_PLUS),1)
  ENGINE_DIR  := sdk\2d_engine_plus
  ENGINE_EXT  := cpp
  ENGINE_CC   := $(M68K_ELF_BIN)\$(M68K_ELF_PREFIX)-g++.exe
  # sdk/2d_engine trails sdk/2d_engine_plus because the plus engine ships only
  # .hpp headers, while game code written in C includes the matching ng_*.h.
  # Those declare the same extern "C" entry points the plus engine defines.
  CFLAGS= -c  -O0 -fomit-frame-pointer   -Wall  -fno-zero-initialized-in-bss  -march=68000 -mcpu=68000 -mtune=68000 -m68000 -ffreestanding -std=gnu99 -I. -Isdk -Isdk/2d_engine_plus -Isdk/2d_engine -Igames/$(GAME)/scenes -Igames/$(GAME)/artbox $(GAME_EXTRA_INCLUDES) -Wa,-march=68000,-mcpu=68000,-W,--warn
  CXXFLAGS= -c  -O0 -fomit-frame-pointer   -Wall  -fno-zero-initialized-in-bss  -march=68000 -mcpu=68000 -mtune=68000 -m68000 -ffreestanding -std=c++14 -fno-exceptions -fno-rtti -fno-threadsafe-statics -I. -Isdk -Isdk/2d_engine_plus -Igames/$(GAME)/scenes -Igames/$(GAME)/artbox $(GAME_EXTRA_INCLUDES) -Wa,-march=68000,-mcpu=68000,-W,--warn
else
  ENGINE_DIR  := sdk\2d_engine
  ENGINE_EXT  := c
  ENGINE_CC   := $(CC)
  CFLAGS= -c  -O0 -fomit-frame-pointer   -Wall  -fno-zero-initialized-in-bss  -march=68000 -mcpu=68000 -mtune=68000 -m68000 -ffreestanding -std=gnu99 -I. -Isdk -Isdk/2d_engine -Igames/$(GAME)/scenes -Igames/$(GAME)/artbox $(GAME_EXTRA_INCLUDES) -Wa,-march=68000,-mcpu=68000,-W,--warn
  CXXFLAGS= $(CFLAGS)
endif
CFLAGS1=-S -O0 -fomit-frame-pointer  -Wall -fno-zero-initialized-in-bss -march=68000  -mcpu=68000 -mtune=68000 -m68000  -ffreestanding

# Which compiler builds the game's own sources (main.c, user.c, the scene files,
# the SDK support .c files).  A USE_2D_PLUS game is a C++ build all the way
# through, not just in the engine, so its .c sources go through g++ as C++.
# The SDK headers carry extern "C" guards, so the declarations they provide keep
# C linkage and match what the engine and the cart entry vectors expect.
ifeq ($(USE_2D_PLUS),1)
  GAME_CC     := $(ENGINE_CC)
  GAME_CFLAGS := $(CXXFLAGS) -Isdk/2d_engine -x c++
else
  GAME_CC     := $(CC)
  GAME_CFLAGS := $(CFLAGS)
endif
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
GDB_HOST?=127.0.0.1
GDB_PORT?=23946
GDB_REMOTE?=$(GDB_HOST):$(GDB_PORT)

FM_MMLS:=$(wildcard $(GAME_SOUND)/fm/*.mml)
MML_TRACKS:=$(wildcard $(GAME_SOUND)/mml/*.mml)
SSG_MMLS:=$(wildcard $(GAME_SOUND)/ssg/*.mml)

CROP=-crop 0x000000 0x080000
SCAT=$(REPO_WIN)\win\srec_cat.exe
INFO=$(REPO_WIN)\win\xxd.exe -g 2
SWAP= -byte-swap 2 -o
FILL= -fill 0xFF  0x000000 0x080000 -range-padding 4 -o
NG_ENGINE_OBJ0=out\ng_defs0.o out\ng_properties0.o out\ng_game_time0.o out\ng_timers0.o out\ng_progress0.o out\ng_status0.o out\ng_game_events0.o out\ng_level0.o out\ng_vram0.o out\ng_sprite_window0.o out\ng_art_asset0.o out\ng_palette_assets0.o out\ng_bg0.o out\ng_fix0.o out\ng_sprite_group0.o out\ng_actions0.o out\ng_chars0.o out\ng_npcs0.o out\ng_physics0.o out\ng_border_constraints0.o out\ng_game_interupt0.o out\ng_scene0.o out\ng_depthfx0.o out\ng_render_queue0.o out\ng_fixed0.o out\ng_camera0.o out\ng_palette_fx0.o out\ng_particles0.o out\ng_feedback0.o out\ng_debug0.o out\ng_joystick0.o out\ng_demo_advanced0.o
# A game.mk may set GAME_SCENES_FROM to other game names; each scene is looked
# for in this game's scenes directory first and in those after, so a game can
# reuse another game's scene sources without copying them.
GAME_SCENES_FROM ?=
GAME_SCENE_DIRS := games/$(GAME)/scenes $(foreach g,$(GAME_SCENES_FROM),games/$(g)/scenes)
GAME_SCENE_SRCS := $(foreach s,$(GAME_SCENES),$(firstword $(wildcard $(addsuffix /$(s).c,$(GAME_SCENE_DIRS)))))
GAME_SCENE_OBJS := $(addprefix out/,$(addsuffix 0.o,$(GAME_SCENES)))
NG_FIX_SDK_OBJ0=out\ng_fix_sdk0.o

ifeq ($(DEBUG),1)
CFLAGS += -g3 -gdwarf-2 -DNG_DEBUG=1
LDFLAGS += -Map=out\game.map
STRIP_SECTS:=-R .comment
else
STRIP_SECTS:=-R .comment -R .text -R .data -R .bss
endif

# Intentionally pinned to game-first order; do not inherit ambient HASHPATH env.
HASHPATH:=$(REPO_WIN)\hash_eagle\$(GAME);$(REPO_WIN)\hash_eagle;$(REPO_WIN)\hash
# Default BIOS for test/debug. Override: make -f MakefileWin32.mak test BIOS=euro
# Supported values (make -f MakefileWin32.mak bios-list for full table):
#   us  us-e  us-v2  us-u4  us-u3
#   euro  euro-s1  asia-mv1c  asia-mv1b
#   japan  japan-s2  japan-s1  japan-mv1b  japan-j3a  japan-mv1c  japan-hotel
#   unibios40 unibios33 unibios32 unibios31 unibios30
#   unibios23 unibios23o unibios22 unibios21 unibios20
#   unibios13 unibios12 unibios12o unibios11 unibios10
#
# The bright green, garbled screen for the first ~3.5s of a run is not
# ours: it is the MVS BIOS power-on self-test painting its RAM/VRAM test
# patterns, and the 68k is still inside BIOS ROM the whole time it is up.
# It is byte-identical under us/euro/japan and does not appear at all
# under the Universe BIOS, which skips the self-test - so
#   make test BIOS=unibios40
# boots straight into the cart if you would rather not sit through it.
BIOS?=euro
ROM_DIR = roms\$(GAME)
DUMP_DIR = dump\$(GAME)
MAME_COMMON=$(MAME) neogeo -rompath $(REPO_WIN)\roms -hashpath "$(HASHPATH)" -bios $(BIOS) -cart1 $(GAME)
LOG_CTX=@echo [neogeosdk] target=$@ game=$(GAME) game_id=$(GAME_ID) platform=$(PLATFORM) rom_dir=$(ROM_DIR) hashpath=$(HASHPATH)

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

.PHONY: game-check
game-check:
	@set GAME=$(GAME)&& $(PY) tools\check_game_cfg.py --cfg $(GAME_CFG_FILE)

.PHONY: all
all: game-check art sfix sound p1
	$(LOG_CTX)

.PHONY: aes
aes:
	$(MAKE) -f MakefileWin32.mak PLATFORM=aes GAME=$(GAME) p1

.PHONY: mvs
mvs:
	$(MAKE) -f MakefileWin32.mak PLATFORM=mvs GAME=$(GAME) p1

.PHONY: p1
p1: game-check game $(GAME_ID)-p1.p1
	$(LOG_CTX)

game: game-check
	$(LOG_CTX)
	$(GAME_CC) $(GAME_CFLAGS) $(PLATFORM_CFLAGS) $(GAME_NEOGEO_C) -o out\neogeo0.o
	$(GAME_CC) $(GAME_CFLAGS) $(PLATFORM_CFLAGS) games\$(GAME)\user.c -o out\user0.o
	$(GAME_CC) $(GAME_CFLAGS) games\$(GAME)\main.c -o out\main0.o
	$(GAME_CC) $(GAME_CFLAGS) sdk\neogeolib.c -o out\neogeolib0.o
	$(GAME_CC) $(GAME_CFLAGS) sdk\ng_fix\ng_fix.c -o out\ng_fix_sdk0.o
	$(ENGINE_CC) $(CXXFLAGS) $(ENGINE_DIR)\ng_defs.$(ENGINE_EXT) -o out\ng_defs0.o
	$(ENGINE_CC) $(CXXFLAGS) $(ENGINE_DIR)\ng_properties.$(ENGINE_EXT) -o out\ng_properties0.o
	$(ENGINE_CC) $(CXXFLAGS) $(ENGINE_DIR)\ng_game_time.$(ENGINE_EXT) -o out\ng_game_time0.o
	$(ENGINE_CC) $(CXXFLAGS) $(ENGINE_DIR)\ng_timers.$(ENGINE_EXT) -o out\ng_timers0.o
	$(ENGINE_CC) $(CXXFLAGS) $(ENGINE_DIR)\ng_progress.$(ENGINE_EXT) -o out\ng_progress0.o
	$(ENGINE_CC) $(CXXFLAGS) $(ENGINE_DIR)\ng_status.$(ENGINE_EXT) -o out\ng_status0.o
	$(ENGINE_CC) $(CXXFLAGS) $(ENGINE_DIR)\ng_game_events.$(ENGINE_EXT) -o out\ng_game_events0.o
	$(ENGINE_CC) $(CXXFLAGS) $(ENGINE_DIR)\ng_level.$(ENGINE_EXT) -o out\ng_level0.o
	$(ENGINE_CC) $(CXXFLAGS) $(ENGINE_DIR)\ng_vram.$(ENGINE_EXT) -o out\ng_vram0.o
	$(ENGINE_CC) $(CXXFLAGS) $(ENGINE_DIR)\ng_sprite_window.$(ENGINE_EXT) -o out\ng_sprite_window0.o
	$(ENGINE_CC) $(CXXFLAGS) $(ENGINE_DIR)\ng_art_asset.$(ENGINE_EXT) -o out\ng_art_asset0.o
	$(ENGINE_CC) $(CXXFLAGS) $(ENGINE_DIR)\ng_palette_assets.$(ENGINE_EXT) -o out\ng_palette_assets0.o
	$(ENGINE_CC) $(CXXFLAGS) $(ENGINE_DIR)\ng_bg.$(ENGINE_EXT) -o out\ng_bg0.o
	$(ENGINE_CC) $(CXXFLAGS) $(ENGINE_DIR)\ng_fix.$(ENGINE_EXT) -o out\ng_fix0.o
	$(ENGINE_CC) $(CXXFLAGS) $(ENGINE_DIR)\ng_sprite_group.$(ENGINE_EXT) -o out\ng_sprite_group0.o
	$(ENGINE_CC) $(CXXFLAGS) $(ENGINE_DIR)\ng_actions.$(ENGINE_EXT) -o out\ng_actions0.o
	$(ENGINE_CC) $(CXXFLAGS) $(ENGINE_DIR)\ng_chars.$(ENGINE_EXT) -o out\ng_chars0.o
	$(ENGINE_CC) $(CXXFLAGS) $(ENGINE_DIR)\ng_npcs.$(ENGINE_EXT) -o out\ng_npcs0.o
	$(ENGINE_CC) $(CXXFLAGS) $(ENGINE_DIR)\ng_physics.$(ENGINE_EXT) -o out\ng_physics0.o
	$(ENGINE_CC) $(CXXFLAGS) $(ENGINE_DIR)\ng_border_constraints.$(ENGINE_EXT) -o out\ng_border_constraints0.o
	$(ENGINE_CC) $(CXXFLAGS) $(ENGINE_DIR)\ng_game_interupt.$(ENGINE_EXT) -o out\ng_game_interupt0.o
	$(ENGINE_CC) $(CXXFLAGS) $(ENGINE_DIR)\ng_scene.$(ENGINE_EXT) -o out\ng_scene0.o
	$(ENGINE_CC) $(CXXFLAGS) $(ENGINE_DIR)\ng_depthfx.$(ENGINE_EXT) -o out\ng_depthfx0.o
	$(ENGINE_CC) $(CXXFLAGS) $(ENGINE_DIR)\ng_render_queue.$(ENGINE_EXT) -o out\ng_render_queue0.o
	$(ENGINE_CC) $(CXXFLAGS) $(ENGINE_DIR)\ng_fixed.$(ENGINE_EXT) -o out\ng_fixed0.o
	$(ENGINE_CC) $(CXXFLAGS) $(ENGINE_DIR)\ng_camera.$(ENGINE_EXT) -o out\ng_camera0.o
	$(ENGINE_CC) $(CXXFLAGS) $(ENGINE_DIR)\ng_palette_fx.$(ENGINE_EXT) -o out\ng_palette_fx0.o
	$(ENGINE_CC) $(CXXFLAGS) $(ENGINE_DIR)\ng_particles.$(ENGINE_EXT) -o out\ng_particles0.o
	$(ENGINE_CC) $(CXXFLAGS) $(ENGINE_DIR)\ng_feedback.$(ENGINE_EXT) -o out\ng_feedback0.o
	$(ENGINE_CC) $(CXXFLAGS) $(ENGINE_DIR)\ng_debug.$(ENGINE_EXT) -o out\ng_debug0.o
	$(ENGINE_CC) $(CXXFLAGS) $(ENGINE_DIR)\ng_joystick.$(ENGINE_EXT) -o out\ng_joystick0.o
	$(ENGINE_CC) $(CXXFLAGS) $(ENGINE_DIR)\ng_demo_advanced.$(ENGINE_EXT) -o out\ng_demo_advanced0.o
	$(foreach src,$(GAME_SCENE_SRCS),$(GAME_CC) $(GAME_CFLAGS) $(src) -o out/$(notdir $(basename $(src)))0.o &&) rem scenes compiled
	$(GAME_CC) $(GAME_CFLAGS) games\$(GAME)\eyecatcher.c -o out\eyecatcher0.o
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
	set GAME=$(GAME)&& set GAME_ID=$(GAME_ID)&& $(PY) hash_eagle\gen_hash.py

.PHONY: samples
samples:
	$(if $(wildcard $(GAME_SOUND)/samples/in_wav_a),cd sound\tools && set PY=$(PY)&& set SOX=$(SOX)&& set GAME_SOUND=..\..\$(subst /,\,$(GAME_SOUND))&& call enc_wave16le_a.bat,@echo samples: no in_wav_a in $(GAME_SOUND)\samples\, skipping a)
	$(if $(wildcard $(GAME_SOUND)/samples/in_wav_a_voice),cd sound\tools && set PY=$(PY)&& set SOX=$(SOX)&& set GAME_SOUND=..\..\$(subst /,\,$(GAME_SOUND))&& call enc_wave16le_a_voice.bat,@echo samples: no in_wav_a_voice in $(GAME_SOUND)\samples\, skipping voice)
	$(if $(wildcard $(GAME_SOUND)/samples/in_wav_b),cd sound\tools && set PY=$(PY)&& set SOX=$(SOX)&& set GAME_SOUND=..\..\$(subst /,\,$(GAME_SOUND))&& call enc_wave16le_b.bat,@echo samples: no in_wav_b in $(GAME_SOUND)\samples\, skipping b)
	cd sound\tools && set PY=$(PY)&& set GAME_SOUND=..\..\$(subst /,\,$(GAME_SOUND))&& call adpcm_enc_process.bat

.PHONY: vrom
vrom:
	set GAME=$(GAME)&& set GAME_ID=$(GAME_ID)&& set GAME_SOUND=$(GAME_SOUND)&& call sound\tools\vrom.bat
	if not exist $(ROM_DIR) mkdir $(ROM_DIR)
	copy /Y out\$(GAME_ID)-v1.v1 $(ROM_DIR)\$(GAME_ID)-v1.v1

.PHONY: fmpatches
fmpatches:
	$(if $(wildcard $(GAME_SOUND)/fm/patches.fm),$(PY) sound\tools\fm_patch_compile.py $(GAME_SOUND)\fm\patches.fm -o sound\driver\fm_patch_table.inc,$(PY) sound\tools\fm_patch_compile.py --empty -o sound\driver\fm_patch_table.inc)

.PHONY: fm
fm:
	$(if $(FM_MMLS),$(PY) sound/tools/fm_compile.py $(FM_MMLS) -o sound/driver/fm_data.inc,$(PY) sound/tools/fm_compile.py -o sound/driver/fm_data.inc)

.PHONY: mml
mml:
	$(if $(MML_TRACKS),$(PY) sound/tools/mml_compile.py $(MML_TRACKS) -o sound/driver/music_data.inc,$(PY) sound/tools/mml_compile.py -o sound/driver/music_data.inc)

.PHONY: ssgconfig
ssgconfig:
	$(if $(wildcard $(GAME_SOUND)/ssg/config.ssg),$(PY) sound\tools\ssg_config_compile.py $(GAME_SOUND)\ssg\config.ssg -o sound\driver\ssg_config.inc,$(PY) sound\tools\ssg_config_compile.py --empty -o sound\driver\ssg_config.inc)

.PHONY: ssg
ssg:
	$(if $(SSG_MMLS),$(PY) sound/tools/ssg_compile.py $(SSG_MMLS) -o sound/driver/ssg_data.inc,$(PY) sound/tools/ssg_compile.py -o sound/driver/ssg_data.inc)

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
sound: game-check samples vrom fmpatches fm mml ssgconfig ssg m1rom
	$(LOG_CTX)

.PHONY: sound-all
sound-all: sound

.PHONY: sfix
sfix: game-check
	$(LOG_CTX)
	if not exist games\$(GAME)\artbox mkdir games\$(GAME)\artbox
	cd games\$(GAME)\artbox && set ARTBOX_DATA_DIR=$(REPO_WIN)\games\$(GAME)\artbox&& set ARTBOX_INFIX_DIR=$(REPO_WIN)\games\$(or $(GAME_ART_FROM),$(GAME))\artbox\infix&& set GAME=$(GAME)&& set GAME_ID=$(GAME_ID)&& $(PY) $(REPO_WIN)\artbox\romdbfiximport.py && $(PY) $(REPO_WIN)\artbox\fixtiles.py && call $(REPO_WIN)\artbox\romfx.bat
	$(PY) tools\verify_sfix_output.py --root "$(CURDIR)" --game "$(GAME)" --game-id "$(GAME_ID)"
	if not exist $(ROM_DIR) mkdir $(ROM_DIR)
	if not exist games\$(GAME)\artbox\$(GAME_ID)-s1.s1 (echo ERROR: missing games\$(GAME)\artbox\$(GAME_ID)-s1.s1 & exit /b 1)
	if not exist games\$(GAME)\artbox\$(GAME_ID)-c1.c1 (echo ERROR: missing games\$(GAME)\artbox\$(GAME_ID)-c1.c1 & exit /b 1)
	if not exist games\$(GAME)\artbox\$(GAME_ID)-c2.c2 (echo ERROR: missing games\$(GAME)\artbox\$(GAME_ID)-c2.c2 & exit /b 1)
	copy /Y games\$(GAME)\artbox\$(GAME_ID)-s1.s1 $(ROM_DIR)\$(GAME_ID)-s1.s1
	copy /Y games\$(GAME)\artbox\$(GAME_ID)-c1.c1 $(ROM_DIR)\$(GAME_ID)-c1.c1
	copy /Y games\$(GAME)\artbox\$(GAME_ID)-c2.c2 $(ROM_DIR)\$(GAME_ID)-c2.c2

.PHONY: srom
srom: sfix

.PHONY: art-clean
art-clean:
	call artbox\makeclean.bat
	if exist artbox\neorom.db del /Q artbox\neorom.db
	if exist artbox\map del /Q artbox\map
	if exist artbox\output1.txt del /Q artbox\output1.txt
	if exist artbox\out.srt del /Q artbox\out.srt
	if exist artbox\screens.c del /Q artbox\screens.c
	if exist artbox\sprite_meta.h del /Q artbox\sprite_meta.h
	if exist artbox\neo.pal del /Q artbox\neo.pal
	if exist artbox\std.pal del /Q artbox\std.pal
	if exist artbox\neopal.bin del /Q artbox\neopal.bin
	if exist artbox\1p.c1 del /Q artbox\1p.c1
	if exist artbox\2p.c2 del /Q artbox\2p.c2
	if exist artbox\assets.cfg del /Q artbox\assets.cfg
	if exist artbox\1c.c1 del /Q artbox\1c.c1
	if exist artbox\2c.c2 del /Q artbox\2c.c2
	if exist artbox\1c.s1 del /Q artbox\1c.s1
	if exist artbox\$(GAME_ID)-s1.s1 del /Q artbox\$(GAME_ID)-s1.s1
	if exist artbox\$(GAME_ID)-c1.c1 del /Q artbox\$(GAME_ID)-c1.c1
	if exist artbox\$(GAME_ID)-c2.c2 del /Q artbox\$(GAME_ID)-c2.c2
	if exist artbox\__pycache__ rmdir /S /Q artbox\__pycache__

.PHONY: art
# Default screen converter is the tile-local pipeline (img2neo_tile.py):
# per-tile k-means + per-tile Floyd-Steinberg dither + greedy MAE bank
# dedup + Lab-nearest pixel remap into a representative palette derived
# from the weighted union of all banks.  Override with `make art-crt`
# (ARTBOX_CRT=1) or set ARTBOX_LEGACY=1 to fall back to the original
# nearest-neighbour-against-global-palette path.
art: game-check
	$(LOG_CTX)
	set ARTBOX_TILE=1&& set GAME_ID=$(GAME_ID)&& set GAME_ART_FROM=$(GAME_ART_FROM)&& call artbox\makeartbox.bat $(GAME)
	$(PY) tools\verify_artbox_palettes.py --root "$(CURDIR)" --game "$(GAME)"
	if exist artbox\assets.cfg del /Q artbox\assets.cfg
	if exist artbox\1c.c1 del /Q artbox\1c.c1
	if exist artbox\2c.c2 del /Q artbox\2c.c2
	if exist artbox\$(GAME_ID)-s1.s1 del /Q artbox\$(GAME_ID)-s1.s1
	if exist artbox\assets_manifest.json del /Q artbox\assets_manifest.json
	if exist artbox\map del /Q artbox\map
	if exist artbox\neo.pal del /Q artbox\neo.pal
	if exist artbox\std.pal del /Q artbox\std.pal
	if exist artbox\neopal.bin del /Q artbox\neopal.bin
	if exist artbox\neorom.db del /Q artbox\neorom.db
	if exist artbox\out.srt del /Q artbox\out.srt
	if exist artbox\output1.txt del /Q artbox\output1.txt
	if exist artbox\screens.c del /Q artbox\screens.c
	if exist artbox\sprite_meta.h del /Q artbox\sprite_meta.h
	if exist artbox\__pycache__ rmdir /S /Q artbox\__pycache__

# art-crt: same as `art` but exports ARTBOX_CRT=1 so romdbimgimport routes
# screen conversions through artbox\img2neo_crt.py (CIE-Lab quantisation +
# horizontal-biased Floyd-Steinberg + CRT gamma/contrast pre-boost).
.PHONY: art-crt
art-crt: game-check
	$(LOG_CTX)
	set ARTBOX_CRT=1&& set GAME_ID=$(GAME_ID)&& set GAME_ART_FROM=$(GAME_ART_FROM)&& call artbox\makeartbox.bat $(GAME)
	$(PY) tools\verify_artbox_palettes.py --root "$(CURDIR)" --game "$(GAME)"
	if exist artbox\assets.cfg del /Q artbox\assets.cfg
	if exist artbox\1c.c1 del /Q artbox\1c.c1
	if exist artbox\2c.c2 del /Q artbox\2c.c2
	if exist artbox\$(GAME_ID)-s1.s1 del /Q artbox\$(GAME_ID)-s1.s1
	if exist artbox\assets_manifest.json del /Q artbox\assets_manifest.json
	if exist artbox\map del /Q artbox\map
	if exist artbox\neo.pal del /Q artbox\neo.pal
	if exist artbox\std.pal del /Q artbox\std.pal
	if exist artbox\neopal.bin del /Q artbox\neopal.bin
	if exist artbox\neorom.db del /Q artbox\neorom.db
	if exist artbox\out.srt del /Q artbox\out.srt
	if exist artbox\output1.txt del /Q artbox\output1.txt
	if exist artbox\screens.c del /Q artbox\screens.c
	if exist artbox\sprite_meta.h del /Q artbox\sprite_meta.h
	if exist artbox\__pycache__ rmdir /S /Q artbox\__pycache__

.PHONY: dist
dist: game-check all
	$(LOG_CTX)
	set GAME=$(GAME)&& set GAME_ID=$(GAME_ID)&& $(PY) hash_eagle\gen_hash.py --dist

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
	if exist $(DUMP_DIR)\*.dump del /Q $(DUMP_DIR)\*.dump
	if exist $(DUMP_DIR)\*.hex del /Q $(DUMP_DIR)\*.hex
	if exist $(DUMP_DIR)\*.txt del /Q $(DUMP_DIR)\*.txt
	if exist $(DUMP_DIR)\*.sym del /Q $(DUMP_DIR)\*.sym
	if exist $(DUMP_DIR)\*.gdb del /Q $(DUMP_DIR)\*.gdb
	if exist $(DUMP_DIR)\*.readelf del /Q $(DUMP_DIR)\*.readelf
	if exist $(ROM_DIR)\$(GAME_ID)-p1.p1 del /Q $(ROM_DIR)\$(GAME_ID)-p1.p1
	if exist hash_eagle\$(GAME)\neogeo.xml del /Q hash_eagle\$(GAME)\neogeo.xml

.PHONY: sound-clean
sound-clean:
	if exist out\$(GAME_ID)-m1.m1 del /Q out\$(GAME_ID)-m1.m1
	if exist out\$(GAME_ID)-v1.v1 del /Q out\$(GAME_ID)-v1.v1
	if exist out\driver.gen.asm del /Q out\driver.gen.asm
	if exist $(ROM_DIR)\$(GAME_ID)-m1.m1 del /Q $(ROM_DIR)\$(GAME_ID)-m1.m1
	if exist $(ROM_DIR)\$(GAME_ID)-v1.v1 del /Q $(ROM_DIR)\$(GAME_ID)-v1.v1
	if exist $(subst /,\,$(GAME_SOUND))\samples\out_16el_a\*.wav del /Q $(subst /,\,$(GAME_SOUND))\samples\out_16el_a\*.wav
	if exist $(subst /,\,$(GAME_SOUND))\samples\out_16el_a_voice\*.wav del /Q $(subst /,\,$(GAME_SOUND))\samples\out_16el_a_voice\*.wav
	if exist $(subst /,\,$(GAME_SOUND))\samples\out_16el_b\*.wav del /Q $(subst /,\,$(GAME_SOUND))\samples\out_16el_b\*.wav
	if exist $(subst /,\,$(GAME_SOUND))\samples\out_sr_a\*.wav del /Q $(subst /,\,$(GAME_SOUND))\samples\out_sr_a\*.wav
	if exist $(subst /,\,$(GAME_SOUND))\samples\out_sr_b\*.wav del /Q $(subst /,\,$(GAME_SOUND))\samples\out_sr_b\*.wav
	if exist $(subst /,\,$(GAME_SOUND))\samples\out_a\*.adpcma del /Q $(subst /,\,$(GAME_SOUND))\samples\out_a\*.adpcma
	if exist $(subst /,\,$(GAME_SOUND))\samples\out_a_voice\*.adpcma del /Q $(subst /,\,$(GAME_SOUND))\samples\out_a_voice\*.adpcma
	if exist $(subst /,\,$(GAME_SOUND))\samples\out_b\*.adpcmb del /Q $(subst /,\,$(GAME_SOUND))\samples\out_b\*.adpcmb
	if exist sound\driver\fm_data.inc del /Q sound\driver\fm_data.inc
	if exist sound\driver\music_data.inc del /Q sound\driver\music_data.inc
	if exist sound\driver\fm_patch_table.inc del /Q sound\driver\fm_patch_table.inc
	if exist sound\driver\sample_table.inc del /Q sound\driver\sample_table.inc
	if exist sound\driver\ssg_config.inc del /Q sound\driver\ssg_config.inc
	if exist sound\driver\ssg_data.inc del /Q sound\driver\ssg_data.inc

.PHONY: clean-all
clean-all: clean sound-clean art-clean
	if exist $(ROM_DIR)\$(GAME_ID)-s1.s1 del /Q $(ROM_DIR)\$(GAME_ID)-s1.s1
	if exist $(ROM_DIR)\$(GAME_ID)-c1.c1 del /Q $(ROM_DIR)\$(GAME_ID)-c1.c1
	if exist $(ROM_DIR)\$(GAME_ID)-c2.c2 del /Q $(ROM_DIR)\$(GAME_ID)-c2.c2
	if exist main.c del /Q main.c
	if exist user.c del /Q user.c
	if exist eyecatcher.c del /Q eyecatcher.c
	if exist demo rmdir /S /Q demo

.PHONY: dump
dump:
	if not exist $(DUMP_DIR) mkdir $(DUMP_DIR)
	$(OBJDUMP) -Dht out\neogeo.o | more
	$(OBJDUMP) -Dht out\user.o | more
	$(OBJDUMP) -Dht out\main.o | more
	$(OBJDUMP) -Dht out\game | more
	$(OBJDUMP) -Dht out\neogeo.o > $(DUMP_DIR)\neogeo.dump
	$(OBJDUMP) -Dht out\user.o > $(DUMP_DIR)\user.dump
	$(OBJDUMP) -Dht out\main.o > $(DUMP_DIR)\main.dump
	$(OBJDUMP) -Dht out\game > $(DUMP_DIR)\game.dump
	$(INFO) out\game.rom | more
	$(INFO) out\game.rom > $(DUMP_DIR)\game.hex

.PHONY: test
test: game-check test-precheck hash
	$(LOG_CTX)
	$(MAME_COMMON) -output console -nofilter -waitvsync -window

.PHONY: test-precheck
test-precheck: game-check unit-tests
	$(LOG_CTX)
	@if not exist $(ROM_DIR)\$(GAME_ID)-p1.p1 (echo ERROR: missing $(ROM_DIR)\$(GAME_ID)-p1.p1. Build first with: make -f MakefileWin32.mak all & exit /b 1)
	@if not exist $(ROM_DIR)\$(GAME_ID)-m1.m1 (echo ERROR: missing $(ROM_DIR)\$(GAME_ID)-m1.m1. Build first with: make -f MakefileWin32.mak all & exit /b 1)
	@if not exist $(ROM_DIR)\$(GAME_ID)-s1.s1 (echo ERROR: missing $(ROM_DIR)\$(GAME_ID)-s1.s1. Build first with: make -f MakefileWin32.mak all & exit /b 1)
	@if not exist $(ROM_DIR)\$(GAME_ID)-v1.v1 (echo ERROR: missing $(ROM_DIR)\$(GAME_ID)-v1.v1. Build first with: make -f MakefileWin32.mak all & exit /b 1)
	@if not exist $(ROM_DIR)\$(GAME_ID)-c1.c1 (echo ERROR: missing $(ROM_DIR)\$(GAME_ID)-c1.c1. Build first with: make -f MakefileWin32.mak all & exit /b 1)
	@if not exist $(ROM_DIR)\$(GAME_ID)-c2.c2 (echo ERROR: missing $(ROM_DIR)\$(GAME_ID)-c2.c2. Build first with: make -f MakefileWin32.mak all & exit /b 1)

.PHONY: test-build
test-build: all
	$(MAKE) -f MakefileWin32.mak GAME=$(GAME) test

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

.PHONY: games-list
games-list:
	@echo Available GAME values:
	@for /D %%d in (games\*) do @echo   %%~nxd

.PHONY: menu
menu:
	@echo NeoGeoSDK Make Menu (Win32)
	@echo.
	@echo Core build:
	@echo   make -f MakefileWin32.mak all
	@echo   make -f MakefileWin32.mak GAME^=demo all
	@echo   make -f MakefileWin32.mak p1 ^| sound ^| sfix ^| art
	@echo.
	@echo Run/Test:
	@echo   make -f MakefileWin32.mak test
	@echo   make -f MakefileWin32.mak test-build
	@echo   make -f MakefileWin32.mak debug
	@echo   make -f MakefileWin32.mak mame-trace
	@echo   make -f MakefileWin32.mak gdb-server
	@echo   make -f MakefileWin32.mak gdb-remote
	@echo.
	@echo Packaging:
	@echo   make -f MakefileWin32.mak dist
	@echo.
	@echo Utilities:
	@echo   make -f MakefileWin32.mak games-list
	@echo   make -f MakefileWin32.mak bios-list
	@echo   make -f MakefileWin32.mak clean ^| clean-all

.PHONY: help
help: menu

.PHONY: debug
debug: all
	$(LOG_CTX)
	$(MAME_COMMON) -output console -debug -verbose -nofilter -waitvsync -window

.PHONY: debug-aes
debug-aes:
	$(MAKE) -f MakefileWin32.mak PLATFORM=aes GAME=$(GAME) debug

.PHONY: mame-trace-script
mame-trace-script:
	if not exist $(DUMP_DIR) mkdir $(DUMP_DIR)
	@echo trace $(DUMP_DIR)\m68k_trace.txt,maincpu> $(DUMP_DIR)\mame_trace.mds
	@echo trace $(DUMP_DIR)\z80_trace.txt,audiocpu>> $(DUMP_DIR)\mame_trace.mds
	@echo go>> $(DUMP_DIR)\mame_trace.mds

.PHONY: mame-trace
mame-trace: all mame-trace-script
	$(LOG_CTX)
	if not exist $(DUMP_DIR) mkdir $(DUMP_DIR)
	$(NM) -n out\game > $(DUMP_DIR)\game.sym
	$(OBJDUMP) -Dht out\game > $(DUMP_DIR)\game.debug.dump
	$(MAME_COMMON) -verbose -debug -debugscript $(DUMP_DIR)\mame_trace.mds
	@echo Trace: $(DUMP_DIR)\m68k_trace.txt  ^|  Symbols: $(DUMP_DIR)\game.sym  ^|  Disasm: $(DUMP_DIR)\game.debug.dump

.PHONY: debug-build
debug-build:
	$(MAKE) -f MakefileWin32.mak GAME=$(GAME) DEBUG=1 p1
	$(MAKE) -f MakefileWin32.mak GAME=$(GAME) DEBUG=1 debug-artifacts

.PHONY: debug-artifacts
debug-artifacts: out\game
	if not exist $(DUMP_DIR) mkdir $(DUMP_DIR)
	if exist $(DUMP_DIR)\game.size.txt del /Q $(DUMP_DIR)\game.size.txt
	if exist $(DUMP_DIR)\game.sym del /Q $(DUMP_DIR)\game.sym
	if exist $(DUMP_DIR)\game.readelf del /Q $(DUMP_DIR)\game.readelf
	if exist $(DUMP_DIR)\game.debug.dump del /Q $(DUMP_DIR)\game.debug.dump
	if exist $(DUMP_DIR)\game.map del /Q $(DUMP_DIR)\game.map
	$(SIZE) out\game > $(DUMP_DIR)\game.size.txt
	$(NM) -n out\game > $(DUMP_DIR)\game.sym
	$(READELF) -a out\game > $(DUMP_DIR)\game.readelf
	$(OBJDUMP) -DhtS out\game > $(DUMP_DIR)\game.debug.dump
	if exist out\game.map copy /Y out\game.map $(DUMP_DIR)\game.map

.PHONY: gdb-script
gdb-script:
	if not exist $(DUMP_DIR) mkdir $(DUMP_DIR)
	@echo set pagination off> $(DUMP_DIR)\gdb_trace.gdb
	@echo set confirm off>> $(DUMP_DIR)\gdb_trace.gdb
	@echo file out/game>> $(DUMP_DIR)\gdb_trace.gdb
	@echo info files>> $(DUMP_DIR)\gdb_trace.gdb
	@echo info functions>> $(DUMP_DIR)\gdb_trace.gdb
	@echo info variables>> $(DUMP_DIR)\gdb_trace.gdb
	@echo maintenance info sections>> $(DUMP_DIR)\gdb_trace.gdb
	@echo quit>> $(DUMP_DIR)\gdb_trace.gdb

.PHONY: gdb-trace
gdb-trace: debug-build gdb-script
	$(GDB) --version > NUL 2> $(DUMP_DIR)\gdb_trace.err && $(GDB) -batch -x $(DUMP_DIR)\gdb_trace.gdb > $(DUMP_DIR)\gdb_trace.txt 2>> $(DUMP_DIR)\gdb_trace.err || echo GDB unavailable: $(GDB)> $(DUMP_DIR)\gdb_trace.txt

.PHONY: gdb
gdb: debug-build
	$(GDB) out\game

.PHONY: gdb-remote
gdb-remote: debug-build
	$(GDB) -ex "target remote $(GDB_REMOTE)" out\game

.PHONY: gdb-server
gdb-server: all debug-build
	$(LOG_CTX)
	@echo GDB server: $(GDB_REMOTE) ^(68000 main CPU^)
	$(MAME_COMMON) -debug -debugger gdbstub -debugger_host $(GDB_HOST) -debugger_port $(GDB_PORT) -output console -nofilter -window
.PHONY: unit-tests
unit-tests:
	$(MAKE) -C tests test
