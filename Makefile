#######
#https://eaglesoftware.biz
#https://github.com/eaglesoftware777
#https://github.com/eaglesoftware777/neogeosdk
#######

# Game selection from game.cfg (override with GAME=...).
# Usage: make GAME=helloworld / make GAME=tutorial / make GAME=neogeogame
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

ifndef SDKHOME
SDKHOME := $(abspath $(CURDIR)/..)
endif

# Toolchain selection:
# 1) $(SDKHOME)/x-tools-v2 (default)
# 2) $(SDKHOME)/x-tools (legacy fallback)
XTOOLS_V2 := $(SDKHOME)/x-tools-v2
XTOOLS_OLD := $(SDKHOME)/x-tools
ifeq ($(wildcard $(XTOOLS_V2)/m68k-unknown-elf/bin/m68k-unknown-elf-gcc),)
XTOOLS_ROOT ?= $(XTOOLS_OLD)
else
XTOOLS_ROOT ?= $(XTOOLS_V2)
endif

CC=$(XTOOLS_ROOT)/m68k-unknown-elf/bin/m68k-unknown-elf-gcc

# 2D engine variant:
#   USE_2D_PLUS=0  (default) — compile sdk/2d_engine   (C, std=gnu99)
#   USE_2D_PLUS=1            — compile sdk/2d_engine_plus (C++14, freestanding)
USE_2D_PLUS ?= 0

# Optional per-game extra include paths.  A game's game.mk can set
# GAME_EXTRA_INCLUDES to, for example, "-Igames/demo/artbox" so it can
# share the artbox of another game without copying it.
GAME_EXTRA_INCLUDES ?=

# Optional per-game art source.  A game's game.mk can set GAME_ART_FROM to
# another game's name to build its C/S ROMs and asset tables from that game's
# artbox/in and artbox/infix instead of keeping a second copy of the artwork.
GAME_ART_FROM ?=

ifeq ($(USE_2D_PLUS),1)
  ENGINE_DIR  := sdk/2d_engine_plus
  ENGINE_EXT  := cpp
  ENGINE_CC   := $(XTOOLS_ROOT)/m68k-unknown-elf/bin/m68k-unknown-elf-g++
  # CFLAGS: used by all .c files — C-compatible flags only, include path updated.
  # sdk/2d_engine is on the path behind sdk/2d_engine_plus because the plus
  # engine ships only .hpp headers, while game code written in C includes the
  # matching ng_*.h.  Those headers declare the same extern "C" entry points the
  # plus engine defines, so a C scene keeps compiling against either engine.
  CFLAGS= -c  -O0 -fomit-frame-pointer   -Wall  -fno-zero-initialized-in-bss  -march=68000 -mcpu=68000 -mtune=68000 -m68000 -ffreestanding -std=gnu99 -I. -Isdk -Isdk/2d_engine_plus -Isdk/2d_engine -Igames/$(GAME)/scenes -Igames/$(GAME)/artbox $(GAME_EXTRA_INCLUDES) -Wa,-march=68000,-mcpu=68000,-W,--warn
  # CXXFLAGS: used only for engine .cpp files
  CXXFLAGS= -c  -O0 -fomit-frame-pointer   -Wall  -fno-zero-initialized-in-bss  -march=68000 -mcpu=68000 -mtune=68000 -m68000 -ffreestanding -std=c++14 -fno-exceptions -fno-rtti -fno-threadsafe-statics -I. -Isdk -Isdk/2d_engine_plus -Igames/$(GAME)/scenes -Igames/$(GAME)/artbox $(GAME_EXTRA_INCLUDES) -Wa,-march=68000,-mcpu=68000,-W,--warn
else
  ENGINE_DIR  := sdk/2d_engine
  ENGINE_EXT  := c
  ENGINE_CC   := $(CC)
  CFLAGS= -c  -O0 -fomit-frame-pointer   -Wall  -fno-zero-initialized-in-bss  -march=68000 -mcpu=68000 -mtune=68000 -m68000 -ffreestanding -std=gnu99 -I. -Isdk -Isdk/2d_engine -Igames/$(GAME)/scenes -Igames/$(GAME)/artbox $(GAME_EXTRA_INCLUDES) -Wa,-march=68000,-mcpu=68000,-W,--warn
  CXXFLAGS= $(CFLAGS)
endif
CFLAGS1=-S -O0 -fomit-frame-pointer  -Wall -fno-zero-initialized-in-bss -march=68000  -mcpu=68000 -mtune=68000 -m68000  -ffreestanding

# Which compiler builds the game's own sources (main.c, user.c, the scene
# files, the SDK support .c files).  A USE_2D_PLUS game is a C++ build all the
# way through, not just in the engine, so its .c sources go through g++ as C++.
# The SDK headers carry extern "C" guards, so the declarations they provide
# keep C linkage and match what the engine and the cart entry vectors expect.
ifeq ($(USE_2D_PLUS),1)
  GAME_CC     := $(ENGINE_CC)
  GAME_CFLAGS := $(CXXFLAGS) -Isdk/2d_engine -x c++
else
  GAME_CC     := $(CC)
  GAME_CFLAGS := $(CFLAGS)
endif
LD=$(XTOOLS_ROOT)/m68k-unknown-elf/bin/m68k-unknown-elf-ld
LDFLAGS=  -nostdlib
OBJCP=$(XTOOLS_ROOT)/m68k-unknown-elf/bin/m68k-unknown-elf-objcopy
OBJDUMP=$(XTOOLS_ROOT)/m68k-unknown-elf/bin/m68k-unknown-elf-objdump
GDB=$(XTOOLS_ROOT)/m68k-unknown-elf/bin/m68k-unknown-elf-gdb
NM=$(XTOOLS_ROOT)/m68k-unknown-elf/bin/m68k-unknown-elf-nm
READELF=$(XTOOLS_ROOT)/m68k-unknown-elf/bin/m68k-unknown-elf-readelf
ADDR2LINE=$(XTOOLS_ROOT)/m68k-unknown-elf/bin/m68k-unknown-elf-addr2line
SIZE=$(XTOOLS_ROOT)/m68k-unknown-elf/bin/m68k-unknown-elf-size
WLAZ80?=wla-z80
WLALINK?=wlalink
PYTHON?=python3
SOX?=
DEBUG?=0
GDB_HOST?=127.0.0.1
GDB_PORT?=23946
GDB_REMOTE?=$(GDB_HOST):$(GDB_PORT)
CROP=-crop 0x000000 0x080000
SCAT=srec_cat
INFO=xxd -g 2
SWAP= -byte-swap 2 -o
FILL= -fill 0xFF  0x000000 0x080000 -range-padding 4 -o
NG_ENGINE_NAMES=ng_defs ng_properties ng_game_time ng_timers ng_progress ng_status ng_game_events ng_level ng_vram ng_sprite_window ng_art_asset ng_palette_assets ng_bg ng_fix ng_sprite_group ng_actions ng_chars ng_npcs ng_physics ng_border_constraints ng_game_interupt ng_scene ng_depthfx ng_render_queue ng_fixed ng_camera ng_palette_fx ng_particles ng_feedback ng_debug ng_joystick ng_demo_advanced
NG_ENGINE_OBJ0=$(addprefix out/,$(addsuffix 0.o,$(NG_ENGINE_NAMES)))
DEMO_NAMES=demo demo_intro demo_sprites demo_camera demo_palette demo_particles demo_depth demo_sound demo_fix demo_combat demo_stress demo_title demo_render
DEMO_OBJ0=$(addprefix out/,$(addsuffix 0.o,$(DEMO_NAMES)))
NG_FIX_SDK_OBJ0=out/ng_fix_sdk0.o

ifeq ($(DEBUG),1)
CFLAGS += -g3 -gdwarf-2 -DNG_DEBUG=1
LDFLAGS += -Map=out/game.map
# In debug builds keep .text/.data/.bss so DWARF relocations remain intact;
# the linker script only picks up custom sections, so extras are discarded.
STRIP_SECTS:=-R .comment
else
STRIP_SECTS:=-R .comment -R .text -R .data -R .bss
endif

# Intentionally pinned to game-first order; do not inherit ambient HASHPATH env.
HASHPATH:=$(CURDIR)/hash_eagle/$(GAME);$(CURDIR)/hash_eagle;$(CURDIR)/hash
# Default BIOS for make test/debug. Override: make test BIOS=euro
# Supported values (make bios-list for full table):
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
ROM_DIR = roms/$(GAME)
DUMP_DIR = dump/$(GAME)
MAME_PLAYBACK ?= -throttle -speed 1.0 -noautoframeskip -frameskip 0 -norefreshspeed
MAME_COMMON=mame neogeo -rompath $(CURDIR)/roms -hashpath "$(HASHPATH)" -bios $(BIOS) -cart1 $(GAME) $(MAME_PLAYBACK)
LOG_CTX=@echo "[neogeosdk] target=$@ game=$(GAME) game_id=$(GAME_ID) platform=$(PLATFORM) rom_dir=$(ROM_DIR) hashpath=$(HASHPATH)"

# PLATFORM: mvs (default) or aes
PLATFORM?=mvs
ifeq ($(PLATFORM),mvs)
NEOGEO_C=sdk/neogeo_mvs.c
PLATFORM_CFLAGS=-DNG_MVS=1
else
NEOGEO_C=sdk/neogeo_aes.c
PLATFORM_CFLAGS=-DNG_AES=1
endif

.DEFAULT_GOAL := p1

.PHONY: game-check
game-check:
	@GAME="$(GAME)" $(PYTHON) tools/check_game_cfg.py --cfg "$(GAME_CFG_FILE)"

.PHONY: all
all: game-check art sfix sound p1
	$(LOG_CTX)

.PHONY: aes
aes:
	$(MAKE) PLATFORM=aes p1

.PHONY: mvs
mvs:
	$(MAKE) PLATFORM=mvs p1

.PHONY: p1
p1: game-check game $(GAME_ID)-p1.p1
	$(LOG_CTX)

# Scene files come from GAME_SCENES in game.mk (explicit list, avoids compiling
# helper/included files).  A game's game.mk may also set GAME_SCENES_FROM to one
# or more other game names; each scene is then looked for in the game's own
# scenes directory first and in those directories after, so a game can reuse
# another game's scenes without copying the sources.
GAME_SCENES_FROM ?=
GAME_SCENE_DIRS := games/$(GAME)/scenes $(foreach g,$(GAME_SCENES_FROM),games/$(g)/scenes)
GAME_SCENE_SRCS := $(foreach s,$(GAME_SCENES),$(firstword $(wildcard $(addsuffix /$(s).c,$(GAME_SCENE_DIRS)))))
GAME_SCENE_OBJS := $(addprefix out/,$(addsuffix 0.o,$(GAME_SCENES)))

# Select per-game neogeo platform file
ifeq ($(PLATFORM),mvs)
GAME_NEOGEO_C = games/$(GAME)/neogeo_mvs.c
else
GAME_NEOGEO_C = games/$(GAME)/neogeo_aes.c
endif

game: game-check
	$(LOG_CTX)
	$(GAME_CC) $(GAME_CFLAGS) $(PLATFORM_CFLAGS)   $(GAME_NEOGEO_C) -o out/neogeo0.o
	$(GAME_CC) $(GAME_CFLAGS) $(PLATFORM_CFLAGS)   games/$(GAME)/user.c -o out/user0.o
	$(GAME_CC) $(GAME_CFLAGS)   games/$(GAME)/main.c -o out/main0.o
	$(GAME_CC) $(GAME_CFLAGS)   games/$(GAME)/eyecatcher.c -o out/eyecatcher0.o
	$(GAME_CC) $(GAME_CFLAGS)   sdk/neogeolib.c -o out/neogeolib0.o
	$(GAME_CC) $(GAME_CFLAGS)   sdk/ng_fix/ng_fix.c -o out/ng_fix_sdk0.o
	$(ENGINE_CC) $(CXXFLAGS)   $(ENGINE_DIR)/ng_defs.$(ENGINE_EXT) -o out/ng_defs0.o
	$(ENGINE_CC) $(CXXFLAGS)   $(ENGINE_DIR)/ng_properties.$(ENGINE_EXT) -o out/ng_properties0.o
	$(ENGINE_CC) $(CXXFLAGS)   $(ENGINE_DIR)/ng_game_time.$(ENGINE_EXT) -o out/ng_game_time0.o
	$(ENGINE_CC) $(CXXFLAGS)   $(ENGINE_DIR)/ng_timers.$(ENGINE_EXT) -o out/ng_timers0.o
	$(ENGINE_CC) $(CXXFLAGS)   $(ENGINE_DIR)/ng_progress.$(ENGINE_EXT) -o out/ng_progress0.o
	$(ENGINE_CC) $(CXXFLAGS)   $(ENGINE_DIR)/ng_status.$(ENGINE_EXT) -o out/ng_status0.o
	$(ENGINE_CC) $(CXXFLAGS)   $(ENGINE_DIR)/ng_game_events.$(ENGINE_EXT) -o out/ng_game_events0.o
	$(ENGINE_CC) $(CXXFLAGS)   $(ENGINE_DIR)/ng_level.$(ENGINE_EXT) -o out/ng_level0.o
	$(ENGINE_CC) $(CXXFLAGS)   $(ENGINE_DIR)/ng_vram.$(ENGINE_EXT) -o out/ng_vram0.o
	$(ENGINE_CC) $(CXXFLAGS)   $(ENGINE_DIR)/ng_sprite_window.$(ENGINE_EXT) -o out/ng_sprite_window0.o
	$(ENGINE_CC) $(CXXFLAGS)   $(ENGINE_DIR)/ng_art_asset.$(ENGINE_EXT) -o out/ng_art_asset0.o
	$(ENGINE_CC) $(CXXFLAGS)   $(ENGINE_DIR)/ng_palette_assets.$(ENGINE_EXT) -o out/ng_palette_assets0.o
	$(ENGINE_CC) $(CXXFLAGS)   $(ENGINE_DIR)/ng_bg.$(ENGINE_EXT) -o out/ng_bg0.o
	$(ENGINE_CC) $(CXXFLAGS)   $(ENGINE_DIR)/ng_fix.$(ENGINE_EXT) -o out/ng_fix0.o
	$(ENGINE_CC) $(CXXFLAGS)   $(ENGINE_DIR)/ng_sprite_group.$(ENGINE_EXT) -o out/ng_sprite_group0.o
	$(ENGINE_CC) $(CXXFLAGS)   $(ENGINE_DIR)/ng_actions.$(ENGINE_EXT) -o out/ng_actions0.o
	$(ENGINE_CC) $(CXXFLAGS)   $(ENGINE_DIR)/ng_chars.$(ENGINE_EXT) -o out/ng_chars0.o
	$(ENGINE_CC) $(CXXFLAGS)   $(ENGINE_DIR)/ng_npcs.$(ENGINE_EXT) -o out/ng_npcs0.o
	$(ENGINE_CC) $(CXXFLAGS)   $(ENGINE_DIR)/ng_physics.$(ENGINE_EXT) -o out/ng_physics0.o
	$(ENGINE_CC) $(CXXFLAGS)   $(ENGINE_DIR)/ng_border_constraints.$(ENGINE_EXT) -o out/ng_border_constraints0.o
	$(ENGINE_CC) $(CXXFLAGS)   $(ENGINE_DIR)/ng_game_interupt.$(ENGINE_EXT) -o out/ng_game_interupt0.o
	$(ENGINE_CC) $(CXXFLAGS)   $(ENGINE_DIR)/ng_scene.$(ENGINE_EXT) -o out/ng_scene0.o
	$(ENGINE_CC) $(CXXFLAGS)   $(ENGINE_DIR)/ng_depthfx.$(ENGINE_EXT) -o out/ng_depthfx0.o
	$(ENGINE_CC) $(CXXFLAGS)   $(ENGINE_DIR)/ng_render_queue.$(ENGINE_EXT) -o out/ng_render_queue0.o
	$(ENGINE_CC) $(CXXFLAGS)   $(ENGINE_DIR)/ng_fixed.$(ENGINE_EXT) -o out/ng_fixed0.o
	$(ENGINE_CC) $(CXXFLAGS)   $(ENGINE_DIR)/ng_camera.$(ENGINE_EXT) -o out/ng_camera0.o
	$(ENGINE_CC) $(CXXFLAGS)   $(ENGINE_DIR)/ng_palette_fx.$(ENGINE_EXT) -o out/ng_palette_fx0.o
	$(ENGINE_CC) $(CXXFLAGS)   $(ENGINE_DIR)/ng_particles.$(ENGINE_EXT) -o out/ng_particles0.o
	$(ENGINE_CC) $(CXXFLAGS)   $(ENGINE_DIR)/ng_feedback.$(ENGINE_EXT) -o out/ng_feedback0.o
	$(ENGINE_CC) $(CXXFLAGS)   $(ENGINE_DIR)/ng_debug.$(ENGINE_EXT) -o out/ng_debug0.o
	$(ENGINE_CC) $(CXXFLAGS)   $(ENGINE_DIR)/ng_joystick.$(ENGINE_EXT) -o out/ng_joystick0.o
	$(ENGINE_CC) $(CXXFLAGS)   $(ENGINE_DIR)/ng_demo_advanced.$(ENGINE_EXT) -o out/ng_demo_advanced0.o
	$(foreach src,$(GAME_SCENE_SRCS),$(GAME_CC) $(GAME_CFLAGS) $(src) -o out/$(notdir $(basename $(src)))0.o;)
	$(OBJCP) $(STRIP_SECTS) out/neogeo0.o     out/neogeo.o
	$(OBJCP) $(STRIP_SECTS) out/user0.o       out/user.o
	$(OBJCP) $(STRIP_SECTS) out/main0.o       out/main.o
	$(OBJCP) $(STRIP_SECTS) out/eyecatcher0.o out/eyecatcher.o
	$(OBJCP) $(STRIP_SECTS) out/neogeolib0.o  out/neogeolib.o
	$(LD) $(LDFLAGS) -T games/$(GAME)/neogeo.ld -o out/game \
	    out/neogeo.o out/user.o out/main.o out/eyecatcher.o out/neogeolib.o \
	    $(NG_FIX_SDK_OBJ0) $(NG_ENGINE_OBJ0) $(GAME_SCENE_OBJS)

$(GAME_ID)-p1.p1: game
	$(OBJCP)   -O ihex    out/game out/game0
	$(SCAT)  out/game0 -Intel $(CROP) -o out/game0.rom -binary
	$(SCAT)  out/game0.rom -binary $(SWAP) out/game1.rom -binary
	$(SCAT)  out/game1.rom -binary $(FILL) out/game.rom -binary
	cp       out/game.rom out/$(GAME_ID)-p1.p1
	mkdir -p $(ROM_DIR)
	cp -f out/$(GAME_ID)-p1.p1 $(ROM_DIR)/$(GAME_ID)-p1.p1
	GAME=$(GAME) GAME_ID=$(GAME_ID) python3 hash_eagle/gen_hash.py

.PHONY: hash
hash:
	GAME=$(GAME) GAME_ID=$(GAME_ID) python3 hash_eagle/gen_hash.py

# Optional per-game sound source, the counterpart to GAME_ART_FROM: a game can
# set GAME_SOUND_FROM in its game.mk to build its M1/V ROMs from another game's
# tracks and samples.  The ROMs are still emitted under this game's own ID.
GAME_SOUND_FROM ?=
GAME_SOUND = games/$(or $(GAME_SOUND_FROM),$(GAME))/sound

# Wildcard expansion at parse time — empty string when a folder has no files
FM_MMLS   := $(wildcard $(GAME_SOUND)/fm/*.mml)
MML_TRACKS := $(wildcard $(GAME_SOUND)/mml/*.mml)
SSG_MMLS  := $(wildcard $(GAME_SOUND)/ssg/*.mml)

.PHONY: mml
mml:
	@if [ -n "$(MML_TRACKS)" ]; then \
	  python3 sound/tools/mml_compile.py $(MML_TRACKS) -o sound/driver/music_data.inc; \
	else \
	  echo "mml: no MML files in $(GAME_SOUND)/mml/, writing an empty table"; \
	  python3 sound/tools/mml_compile.py -o sound/driver/music_data.inc; \
	fi

.PHONY: fmpatches
fmpatches:
	@if [ -f "$(GAME_SOUND)/fm/patches.fm" ]; then \
	  python3 sound/tools/fm_patch_compile.py $(GAME_SOUND)/fm/patches.fm -o sound/driver/fm_patch_table.inc; \
	else \
	  echo "fmpatches: no patches.fm in $(GAME_SOUND)/fm/, writing an empty table"; \
	  python3 sound/tools/fm_patch_compile.py --empty -o sound/driver/fm_patch_table.inc; \
	fi

.PHONY: fm
fm:
	@if [ -n "$(FM_MMLS)" ]; then \
	  python3 sound/tools/fm_compile.py $(FM_MMLS) -o sound/driver/fm_data.inc; \
	else \
	  echo "fm: no MML files in $(GAME_SOUND)/fm/, writing an empty table"; \
	  python3 sound/tools/fm_compile.py -o sound/driver/fm_data.inc; \
	fi

.PHONY: ssgconfig
ssgconfig:
	@if [ -f "$(GAME_SOUND)/ssg/config.ssg" ]; then \
	  python3 sound/tools/ssg_config_compile.py $(GAME_SOUND)/ssg/config.ssg -o sound/driver/ssg_config.inc; \
	else \
	  echo "ssgconfig: no config.ssg in $(GAME_SOUND)/ssg/, writing an empty table"; \
	  python3 sound/tools/ssg_config_compile.py --empty -o sound/driver/ssg_config.inc; \
	fi

.PHONY: ssg
ssg:
	@if [ -n "$(SSG_MMLS)" ]; then \
	  python3 sound/tools/ssg_compile.py $(SSG_MMLS) -o sound/driver/ssg_data.inc; \
	else \
	  echo "ssg: no MML files in $(GAME_SOUND)/ssg/, writing an empty table"; \
	  python3 sound/tools/ssg_compile.py -o sound/driver/ssg_data.inc; \
	fi

.PHONY: samples
samples:
	@if [ -d "$(GAME_SOUND)/samples/in_wav_a" ]; then \
	  cd sound/tools && GAME_SOUND=../../$(GAME_SOUND) PYTHON=$(PYTHON) SOX=$(SOX) ./enc_wave16le_a.sh; \
	  else echo "samples: no in_wav_a, skipping a"; fi
	@if [ -d "$(GAME_SOUND)/samples/in_wav_a_voice" ]; then \
	  cd sound/tools && GAME_SOUND=../../$(GAME_SOUND) PYTHON=$(PYTHON) SOX=$(SOX) ./enc_wave16le_a_voice.sh; \
	  else echo "samples: no in_wav_a_voice, skipping voice"; fi
	@if [ -d "$(GAME_SOUND)/samples/in_wav_b" ]; then \
	  cd sound/tools && GAME_SOUND=../../$(GAME_SOUND) PYTHON=$(PYTHON) SOX=$(SOX) ./enc_wave16le_b.sh; \
	  else echo "samples: no in_wav_b, skipping b"; fi
	@cd sound/tools && GAME_SOUND=../../$(GAME_SOUND) PYTHON=$(PYTHON) ./adpcm_enc_process.sh

.PHONY: vrom
vrom:
	GAME=$(GAME) GAME_ID=$(GAME_ID) GAME_SOUND=$(GAME_SOUND) ./sound/tools/vrom.sh
	mkdir -p $(ROM_DIR)
	cp -f out/$(GAME_ID)-v1.v1 $(ROM_DIR)/$(GAME_ID)-v1.v1

.PHONY: m1rom
m1rom: vrom fmpatches fm mml ssgconfig ssg
	WLAZ80=$(WLAZ80) WLALINK=$(WLALINK) USE_Z80C=$(USE_Z80C) Z80C_SRC=$(Z80C_SRC_LINUX) GAME=$(GAME) GAME_SOUND=$(GAME_SOUND) GAME_ID=$(GAME_ID) ./sound/tools/m1rom.sh

.PHONY: m1rom-asm
m1rom-asm:
	$(MAKE) m1rom USE_Z80C=0
	mkdir -p out/compare
	cp -f out/$(GAME_ID)-m1.m1 out/compare/$(GAME_ID)-m1-asm.m1

.PHONY: m1rom-c
m1rom-c:
	$(MAKE) m1rom USE_Z80C=1 LINK_C_DRIVER=1
	mkdir -p out/compare
	cp -f out/$(GAME_ID)-m1.m1 out/compare/$(GAME_ID)-m1-c.m1

.PHONY: compare-driver
compare-driver: m1rom-asm m1rom-c
	python3 sound/tools/compare_m1.py out/compare/$(GAME_ID)-m1-asm.m1 out/compare/$(GAME_ID)-m1-c.m1

.PHONY: sound
sound: game-check samples
	$(MAKE) m1rom GAME=$(GAME)
	$(LOG_CTX)


.PHONY: sound-all
sound-all: sound


.PHONY: sfix
sfix: game-check
	$(LOG_CTX)
	mkdir -p games/$(GAME)/artbox
	cd games/$(GAME)/artbox && ARTBOX_DATA_DIR="$(CURDIR)/games/$(GAME)/artbox" ARTBOX_INFIX_DIR="$(CURDIR)/games/$(or $(GAME_ART_FROM),$(GAME))/artbox/infix" GAME=$(GAME) GAME_ID=$(GAME_ID) python3 "$(CURDIR)/artbox/romdbfiximport.py" && ARTBOX_DATA_DIR="$(CURDIR)/games/$(GAME)/artbox" GAME=$(GAME) GAME_ID=$(GAME_ID) python3 "$(CURDIR)/artbox/fixtiles.py" && GAME=$(GAME) GAME_ID=$(GAME_ID) "$(CURDIR)/artbox/romfx.sh"
	python3 tools/verify_sfix_output.py --root "$(CURDIR)" --game "$(GAME)" --game-id "$(GAME_ID)"
	mkdir -p $(ROM_DIR)
	@set -e; \
	for ext in s1 c1 c2; do \
		src_game="games/$(GAME)/artbox/$(GAME_ID)-$$ext.$$ext"; \
		dst="$(ROM_DIR)/$(GAME_ID)-$$ext.$$ext"; \
		if [ -f "$$src_game" ]; then \
			cp -f "$$src_game" "$$dst"; \
		else \
			echo "ERROR: missing $$src_game" >&2; \
			exit 1; \
		fi; \
	done

.PHONY: srom
srom: sfix

.PHONY: art-clean
art-clean:
	./artbox/makeclean.sh
	rm -f artbox/neorom.db artbox/map artbox/output1.txt artbox/out.srt artbox/screens.c artbox/sprite_meta.h
	rm -f artbox/neo.pal artbox/std.pal artbox/neopal.bin artbox/1p.c1 artbox/2p.c2
	rm -f artbox/1c.c1 artbox/2c.c2 artbox/1c.s1
	rm -f artbox/$(GAME_ID)-s1.s1 artbox/$(GAME_ID)-c1.c1 artbox/$(GAME_ID)-c2.c2
	rm -rf artbox/__pycache__

.PHONY: art
# Default screen converter is the tile-local pipeline (img2neo_tile.py):
# per-tile k-means + per-tile Floyd-Steinberg dither + greedy MAE bank
# dedup + Lab-nearest pixel remap into a representative palette derived
# from the weighted union of all banks.  Override with `make art-crt`
# (ARTBOX_CRT=1) or by exporting ARTBOX_LEGACY=1 to fall back to the
# original nearest-neighbour-against-global-palette path.
art: game-check
	$(LOG_CTX)
	ARTBOX_TILE=1 GAME_ID=$(GAME_ID) GAME_ART_FROM="$(GAME_ART_FROM)" ./artbox/makeartbox.sh $(GAME)
	python3 tools/verify_artbox_palettes.py --root "$(CURDIR)" --game "$(GAME)"
	rm -f artbox/assets.cfg artbox/1c.c1 artbox/2c.c2 artbox/$(GAME_ID)-s1.s1 artbox/assets_manifest.json artbox/map artbox/neo.pal artbox/std.pal artbox/neopal.bin artbox/neorom.db artbox/out.srt artbox/output1.txt artbox/screens.c artbox/sprite_meta.h
	rm -rf artbox/__pycache__

# art-crt: same pipeline as `art` but flips ARTBOX_CRT=1 so romdbimgimport
# routes screen conversions through artbox/img2neo_crt.py (CIE-Lab k-means
# palette + horizontal-biased Floyd-Steinberg + CRT gamma/contrast pre-boost).
# Use when you want the best-fidelity rendering of photographic / hand-painted
# title screens.  Sprites are unaffected — they keep the legacy nearest-
# neighbour path so existing shared-palette behaviour is preserved.
.PHONY: art-crt
art-crt: game-check
	$(LOG_CTX)
	ARTBOX_CRT=1 GAME_ID=$(GAME_ID) GAME_ART_FROM="$(GAME_ART_FROM)" ./artbox/makeartbox.sh $(GAME)
	python3 tools/verify_artbox_palettes.py --root "$(CURDIR)" --game "$(GAME)"
	rm -f artbox/assets.cfg artbox/1c.c1 artbox/2c.c2 artbox/$(GAME_ID)-s1.s1 artbox/assets_manifest.json artbox/map artbox/neo.pal artbox/std.pal artbox/neopal.bin artbox/neorom.db artbox/out.srt artbox/output1.txt artbox/screens.c artbox/sprite_meta.h
	rm -rf artbox/__pycache__

.PHONY: dist
dist: game-check all
	$(LOG_CTX)
	GAME=$(GAME) GAME_ID=$(GAME_ID) python3 hash_eagle/gen_hash.py --dist

.PHONY: clean
clean:
	rm -f out/game out/game0 out/game0.rom out/game1.rom out/game.rom out/$(GAME_ID)-p1.p1
	rm -f out/*.o out/*.s out/game.map $(DUMP_DIR)/*.dump $(DUMP_DIR)/*.hex $(DUMP_DIR)/*.txt $(DUMP_DIR)/*.sym $(DUMP_DIR)/*.gdb $(DUMP_DIR)/*.readelf
	rm -f $(ROM_DIR)/$(GAME_ID)-p1.p1
	rm -f hash_eagle/$(GAME)/neogeo.xml

.PHONY: sound-clean
# Encoded samples are deleted only where the WAVs that produce them are
# present.  `make samples` skips an encode whose in_wav_* directory is
# missing, so for a game that ships ADPCM without source in the tree -
# skylance has none at all, and only demo has voice or B-channel WAVs -
# removing them is not a clean, it is a loss with no way back.  This
# target is reached by clean-all, which is the first thing test.bat runs.
sound-clean:
	rm -f out/$(GAME_ID)-m1.m1 out/$(GAME_ID)-v1.v1 out/driver.gen.asm
	rm -f $(ROM_DIR)/$(GAME_ID)-m1.m1 $(ROM_DIR)/$(GAME_ID)-v1.v1
	@if [ -d "$(GAME_SOUND)/samples/in_wav_a" ]; then \
	    rm -f $(GAME_SOUND)/samples/out_16el_a/*.wav \
	          $(GAME_SOUND)/samples/out_sr_a/*.wav \
	          $(GAME_SOUND)/samples/out_a/*.adpcma; \
	  else \
	    echo "sound-clean: keeping out_a, no in_wav_a to rebuild it from"; \
	  fi
	@if [ -d "$(GAME_SOUND)/samples/in_wav_a_voice" ]; then \
	    rm -f $(GAME_SOUND)/samples/out_16el_a_voice/*.wav \
	          $(GAME_SOUND)/samples/out_a_voice/*.adpcma; \
	  else \
	    echo "sound-clean: keeping out_a_voice, no in_wav_a_voice to rebuild it from"; \
	  fi
	@if [ -d "$(GAME_SOUND)/samples/in_wav_b" ]; then \
	    rm -f $(GAME_SOUND)/samples/out_16el_b/*.wav \
	          $(GAME_SOUND)/samples/out_sr_b/*.wav \
	          $(GAME_SOUND)/samples/out_b/*.adpcmb; \
	  else \
	    echo "sound-clean: keeping out_b, no in_wav_b to rebuild it from"; \
	  fi
	rm -f sound/driver/fm_data.inc sound/driver/music_data.inc sound/driver/fm_patch_table.inc sound/driver/sample_table.inc sound/driver/ssg_config.inc sound/driver/ssg_data.inc


.PHONY: clean-all
clean-all: clean sound-clean art-clean
	rm -f $(ROM_DIR)/$(GAME_ID)-s1.s1 $(ROM_DIR)/$(GAME_ID)-c1.c1 $(ROM_DIR)/$(GAME_ID)-c2.c2
	rm -f main.c user.c eyecatcher.c
	rm -rf demo
	
.PHONY: dump
dump: 	
	mkdir -p $(DUMP_DIR)
	$(OBJDUMP)   -Dht out/neogeo.o | more
	$(OBJDUMP)   -Dht out/user.o | more 
	$(OBJDUMP)   -Dht out/main.o | more 
	$(OBJDUMP)   -Dht out/game  | more
	$(OBJDUMP)   -Dht out/neogeo.o > $(DUMP_DIR)/neogeo.dump
	$(OBJDUMP)   -Dht out/user.o  >  $(DUMP_DIR)/user.dump 
	$(OBJDUMP)   -Dht out/main.o  >  $(DUMP_DIR)/main.dump 
	$(OBJDUMP)   -Dht out/game  >  $(DUMP_DIR)/game.dump
	#$(CC) -Wa,-acdlns  -c sdk/neogeo.c  user.c main.c sdk/neogeolib.c  > out/game.s
	$(INFO) out/game.rom | more 
	$(INFO) out/game.rom > $(DUMP_DIR)/game.hex 

.PHONY: test
test: game-check test-precheck hash
	$(LOG_CTX)
	$(MAME_COMMON) -output console -nofilter -throttle -waitvsync -window

.PHONY: test-precheck
test-precheck: game-check
	$(LOG_CTX)
	@[ -f "$(ROM_DIR)/$(GAME_ID)-p1.p1" ] || (echo "ERROR: missing $(ROM_DIR)/$(GAME_ID)-p1.p1. Build first with: make all" && exit 1)
	@[ -f "$(ROM_DIR)/$(GAME_ID)-m1.m1" ] || (echo "ERROR: missing $(ROM_DIR)/$(GAME_ID)-m1.m1. Build first with: make all" && exit 1)
	@[ -f "$(ROM_DIR)/$(GAME_ID)-s1.s1" ] || (echo "ERROR: missing $(ROM_DIR)/$(GAME_ID)-s1.s1. Build first with: make all" && exit 1)
	@[ -f "$(ROM_DIR)/$(GAME_ID)-v1.v1" ] || (echo "ERROR: missing $(ROM_DIR)/$(GAME_ID)-v1.v1. Build first with: make all" && exit 1)
	@[ -f "$(ROM_DIR)/$(GAME_ID)-c1.c1" ] || (echo "ERROR: missing $(ROM_DIR)/$(GAME_ID)-c1.c1. Build first with: make all" && exit 1)
	@[ -f "$(ROM_DIR)/$(GAME_ID)-c2.c2" ] || (echo "ERROR: missing $(ROM_DIR)/$(GAME_ID)-c2.c2. Build first with: make all" && exit 1)

.PHONY: test-build
test-build: all
	$(MAKE) test

.PHONY: test-aes
test-aes:
	$(MAKE) PLATFORM=aes test

.PHONY: test-mvs
test-mvs:
	$(MAKE) PLATFORM=mvs test

.PHONY: bios-list
bios-list:
	@echo "Supported BIOS values for: make test BIOS=<name>"
	@echo ""
	@echo "  euro             Europe MVS (Ver. 2)"
	@echo "  euro-s1          Europe MVS (Ver. 1)"
	@echo "  asia-mv1c        Asia NEO-MVH MV1C"
	@echo "  asia-mv1b        Asia MV1B"
	@echo "  us               US MVS (Ver. 2?)          [default]"
	@echo "  us-e             US MVS (Ver. 1)"
	@echo "  us-v2            US MVS (4 slot, Ver 2)"
	@echo "  us-u4            US MVS (U4)"
	@echo "  us-u3            US MVS (U3)"
	@echo "  japan            Japan MVS (Ver. 3)"
	@echo "  japan-s2         Japan MVS (Ver. 2)"
	@echo "  japan-s1         Japan MVS (Ver. 1)"
	@echo "  japan-mv1b       Japan MV1B"
	@echo "  japan-j3a        Japan MVS (J3, alt)"
	@echo "  japan-mv1c       Japan NEO-MVH MV1C"
	@echo "  japan-hotel      Custom Japanese Hotel"
	@echo "  unibios40        Universe BIOS (Hack, Ver. 4.0)"
	@echo "  unibios33        Universe BIOS (Hack, Ver. 3.3)"
	@echo "  unibios32        Universe BIOS (Hack, Ver. 3.2)"
	@echo "  unibios31        Universe BIOS (Hack, Ver. 3.1)"
	@echo "  unibios30        Universe BIOS (Hack, Ver. 3.0)"
	@echo "  unibios23        Universe BIOS (Hack, Ver. 2.3)"
	@echo "  unibios23o       Universe BIOS (Hack, Ver. 2.3, older?)"
	@echo "  unibios22        Universe BIOS (Hack, Ver. 2.2)"
	@echo "  unibios21        Universe BIOS (Hack, Ver. 2.1)"
	@echo "  unibios20        Universe BIOS (Hack, Ver. 2.0)"
	@echo "  unibios13        Universe BIOS (Hack, Ver. 1.3)"
	@echo "  unibios12        Universe BIOS (Hack, Ver. 1.2)"
	@echo "  unibios12o       Universe BIOS (Hack, Ver. 1.2, older)"
	@echo "  unibios11        Universe BIOS (Hack, Ver. 1.1)"
	@echo "  unibios10        Universe BIOS (Hack, Ver. 1.0)"

.PHONY: games-list
games-list:
	@echo "Available GAME values:"
	@for d in games/*; do \
		if [ -d "$$d" ]; then \
			basename "$$d"; \
		fi; \
	done

.PHONY: menu
menu:
	@echo "NeoGeoSDK Make Menu (Linux)"
	@echo ""
	@echo "Core build:"
	@echo "  make all                     # full pipeline (art+sfix+sound+p1)"
	@echo "  make GAME=<name> all         # build specific game"
	@echo "  make p1 / sound / sfix / art # individual stages"
	@echo ""
	@echo "Run/Test:"
	@echo "  make test                    # run already-built ROM set (no build)"
	@echo "  make test-build              # build then run"
	@echo "  make debug                   # build then run with debugger flags"
	@echo "  make mame-trace              # build + MAME trace script"
	@echo "  make gdb-server              # MAME 68000 GDB server on port 23946"
	@echo "  make gdb-remote              # connect cross-GDB to that server"
	@echo ""
	@echo "Packaging:"
	@echo "  make dist                    # full build + dist package for GAME"
	@echo ""
	@echo "Utilities:"
	@echo "  make games-list              # list game folders"
	@echo "  make bios-list               # list BIOS values"
	@echo "  make clean / clean-all       # clear current game artifacts"

.PHONY: help
help: menu

.PHONY: debug
debug: all
	$(LOG_CTX)
	$(MAME_COMMON) -output console -debug -verbose -nofilter -throttle -waitvsync -window

.PHONY: debug-aes
debug-aes:
	$(MAKE) debug

.PHONY: mame-trace-script
mame-trace-script:
	mkdir -p $(DUMP_DIR)
	printf "trace $(DUMP_DIR)/m68k_trace.txt,maincpu\ntrace $(DUMP_DIR)/z80_trace.txt,audiocpu\ngo\n" > $(DUMP_DIR)/mame_trace.mds

.PHONY: mame-trace
mame-trace: all mame-trace-script
	$(LOG_CTX)
	mkdir -p $(DUMP_DIR)
	$(NM) -n out/game > $(DUMP_DIR)/game.sym
	$(OBJDUMP) -Dht out/game > $(DUMP_DIR)/game.debug.dump
	$(MAME_COMMON) -verbose -debug -debugscript $(DUMP_DIR)/mame_trace.mds
	@echo "Trace: $(DUMP_DIR)/m68k_trace.txt  |  Symbols: $(DUMP_DIR)/game.sym  |  Disasm: $(DUMP_DIR)/game.debug.dump"

.PHONY: debug-build
debug-build:
	$(MAKE) DEBUG=1 p1
	$(MAKE) DEBUG=1 debug-artifacts

.PHONY: debug-artifacts
debug-artifacts: out/game
	mkdir -p $(DUMP_DIR)
	rm -f $(DUMP_DIR)/game.size.txt $(DUMP_DIR)/game.sym $(DUMP_DIR)/game.readelf $(DUMP_DIR)/game.debug.dump $(DUMP_DIR)/game.map
	$(SIZE) out/game > $(DUMP_DIR)/game.size.txt
	$(NM) -n out/game > $(DUMP_DIR)/game.sym
	$(READELF) -a out/game > $(DUMP_DIR)/game.readelf
	$(OBJDUMP) -DhtS out/game > $(DUMP_DIR)/game.debug.dump
	if test -f out/game.map; then cp -f out/game.map $(DUMP_DIR)/game.map; fi

.PHONY: gdb-script
gdb-script:
	mkdir -p $(DUMP_DIR)
	printf "set pagination off\nset confirm off\nfile out/game\ninfo files\ninfo functions\ninfo variables\nmaintenance info sections\nquit\n" > $(DUMP_DIR)/gdb_trace.gdb

.PHONY: gdb-trace
gdb-trace: debug-build gdb-script
	@if $(GDB) --version >/dev/null 2>$(DUMP_DIR)/gdb_trace.err; then \
		$(GDB) -batch -x $(DUMP_DIR)/gdb_trace.gdb > $(DUMP_DIR)/gdb_trace.txt 2>>$(DUMP_DIR)/gdb_trace.err; \
	else \
		printf "GDB unavailable: %s\n\n" "$(GDB)" > $(DUMP_DIR)/gdb_trace.txt; \
		cat $(DUMP_DIR)/gdb_trace.err >> $(DUMP_DIR)/gdb_trace.txt; \
		printf "\nOverride with: make gdb-trace GDB=/path/to/m68k-gdb\n" >> $(DUMP_DIR)/gdb_trace.txt; \
	fi

.PHONY: gdb
gdb: debug-build
	$(GDB) out/game

.PHONY: gdb-remote
gdb-remote: debug-build
	$(GDB) -ex "target remote $(GDB_REMOTE)" out/game

.PHONY: gdb-server
gdb-server: all debug-build
	$(LOG_CTX)
	@echo "GDB server: $(GDB_REMOTE) (68000 main CPU)"
	$(MAME_COMMON) -debug -debugger gdbstub -debugger_host $(GDB_HOST) -debugger_port $(GDB_PORT) -output console -nofilter -window

# Host-side sprite renderer tests.  Deliberately NOT a prerequisite of
# "make test": launching the ROM in MAME must not depend on a host C++
# toolchain being installed.  Use "make check" to gate on both.
.PHONY: unit-tests
unit-tests:
	$(MAKE) -C tests test

.PHONY: check
check: unit-tests test-precheck
	@echo "check: unit tests and ROM set OK"
