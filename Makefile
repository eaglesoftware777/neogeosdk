#######
#https://eaglesoftware.biz
#https://github.com/eaglesoftware777
#https://github.com/eaglesoftware777/neogeosdk
#######
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
CFLAGS= -c  -O0 -fomit-frame-pointer   -Wall  -fno-zero-initialized-in-bss  -march=68000 -mcpu=68000 -mtune=68000 -m68000 -ffreestanding -std=gnu99 -I. -Isdk -Isdk/2d_engine -Wa,-march=68000,-mcpu=68000,-W,--warn
CFLAGS1=-S -O0 -fomit-frame-pointer  -Wall -fno-zero-initialized-in-bss -march=68000  -mcpu=68000 -mtune=68000 -m68000  -ffreestanding
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
GDB_REMOTE?=localhost:1234
CROP=-crop 0x000000 0x080000
SCAT=srec_cat
INFO=xxd -g 2
SWAP= -byte-swap 2 -o
FILL= -fill 0xFF  0x000000 0x080000 -range-padding 4 -o
NG_ENGINE_NAMES=ng_defs ng_properties ng_game_time ng_timers ng_progress ng_status ng_game_events ng_level ng_bg ng_fix ng_sprite_group ng_actions ng_chars ng_npcs ng_physics ng_border_constraints ng_game_interupt ng_depthfx ng_render_queue ng_fixed ng_camera ng_palette_fx ng_particles ng_feedback ng_debug ng_demo_advanced
NG_ENGINE_OBJ0=$(addprefix out/,$(addsuffix 0.o,$(NG_ENGINE_NAMES)))
DEMO_NAMES=demo demo_intro demo_sprites demo_camera demo_palette demo_particles demo_depth demo_sound demo_fix demo_combat demo_stress demo_title
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

HASHPATH?=$(CURDIR)/hash_eagle:$(CURDIR)/hash
# Default BIOS for make test/debug. Override: make test BIOS=euro
# Supported values (make bios-list for full table):
#   us  us-e  us-v2  us-u4  us-u3
#   euro  euro-s1  asia-mv1c  asia-mv1b
#   japan  japan-s2  japan-s1  japan-mv1b  japan-j3a  japan-mv1c  japan-hotel
#   unibios40 unibios33 unibios32 unibios31 unibios30
#   unibios23 unibios23o unibios22 unibios21 unibios20
#   unibios13 unibios12 unibios12o unibios11 unibios10
BIOS?=euro
MAME_COMMON=mame neogeo -rompath $(CURDIR)/roms -hashpath $(HASHPATH) -bios $(BIOS) -cart1 neogeosdk

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

.PHONY: all
all: art sfix sound p1

.PHONY: aes
aes:
	$(MAKE) PLATFORM=aes p1

.PHONY: mvs
mvs:
	$(MAKE) PLATFORM=mvs p1

.PHONY: p1
p1: game 777-p1.p1

game:
	$(CC) $(CFLAGS) $(PLATFORM_CFLAGS)   $(NEOGEO_C)  -o out/neogeo0.o
	$(CC) $(CFLAGS) $(PLATFORM_CFLAGS)   user.c -o out/user0.o
	$(CC) $(CFLAGS)   main.c -o out/main0.o
	$(CC) $(CFLAGS)   sdk/neogeolib.c -o out/neogeolib0.o
	$(CC) $(CFLAGS)   sdk/ng_fix/ng_fix.c -o out/ng_fix_sdk0.o
	$(CC) $(CFLAGS)   sdk/2d_engine/ng_defs.c -o out/ng_defs0.o
	$(CC) $(CFLAGS)   sdk/2d_engine/ng_properties.c -o out/ng_properties0.o
	$(CC) $(CFLAGS)   sdk/2d_engine/ng_game_time.c -o out/ng_game_time0.o
	$(CC) $(CFLAGS)   sdk/2d_engine/ng_timers.c -o out/ng_timers0.o
	$(CC) $(CFLAGS)   sdk/2d_engine/ng_progress.c -o out/ng_progress0.o
	$(CC) $(CFLAGS)   sdk/2d_engine/ng_status.c -o out/ng_status0.o
	$(CC) $(CFLAGS)   sdk/2d_engine/ng_game_events.c -o out/ng_game_events0.o
	$(CC) $(CFLAGS)   sdk/2d_engine/ng_level.c -o out/ng_level0.o
	$(CC) $(CFLAGS)   sdk/2d_engine/ng_bg.c -o out/ng_bg0.o
	$(CC) $(CFLAGS)   sdk/2d_engine/ng_fix.c -o out/ng_fix0.o
	$(CC) $(CFLAGS)   sdk/2d_engine/ng_sprite_group.c -o out/ng_sprite_group0.o
	$(CC) $(CFLAGS)   sdk/2d_engine/ng_actions.c -o out/ng_actions0.o
	$(CC) $(CFLAGS)   sdk/2d_engine/ng_chars.c -o out/ng_chars0.o
	$(CC) $(CFLAGS)   sdk/2d_engine/ng_npcs.c -o out/ng_npcs0.o
	$(CC) $(CFLAGS)   sdk/2d_engine/ng_physics.c -o out/ng_physics0.o
	$(CC) $(CFLAGS)   sdk/2d_engine/ng_border_constraints.c -o out/ng_border_constraints0.o
	$(CC) $(CFLAGS)   sdk/2d_engine/ng_game_interupt.c -o out/ng_game_interupt0.o
	$(CC) $(CFLAGS)   sdk/2d_engine/ng_depthfx.c -o out/ng_depthfx0.o
	$(CC) $(CFLAGS)   sdk/2d_engine/ng_render_queue.c -o out/ng_render_queue0.o
	$(CC) $(CFLAGS)   sdk/2d_engine/ng_fixed.c -o out/ng_fixed0.o
	$(CC) $(CFLAGS)   sdk/2d_engine/ng_camera.c -o out/ng_camera0.o
	$(CC) $(CFLAGS)   sdk/2d_engine/ng_palette_fx.c -o out/ng_palette_fx0.o
	$(CC) $(CFLAGS)   sdk/2d_engine/ng_particles.c -o out/ng_particles0.o
	$(CC) $(CFLAGS)   sdk/2d_engine/ng_feedback.c -o out/ng_feedback0.o
	$(CC) $(CFLAGS)   sdk/2d_engine/ng_debug.c -o out/ng_debug0.o
	$(CC) $(CFLAGS)   sdk/2d_engine/ng_demo_advanced.c -o out/ng_demo_advanced0.o
	$(CC) $(CFLAGS)   demo/demo.c -o out/demo0.o
	$(CC) $(CFLAGS)   demo/demo_intro.c -o out/demo_intro0.o
	$(CC) $(CFLAGS)   demo/demo_sprites.c -o out/demo_sprites0.o
	$(CC) $(CFLAGS)   demo/demo_camera.c -o out/demo_camera0.o
	$(CC) $(CFLAGS)   demo/demo_palette.c -o out/demo_palette0.o
	$(CC) $(CFLAGS)   demo/demo_particles.c -o out/demo_particles0.o
	$(CC) $(CFLAGS)   demo/demo_depth.c -o out/demo_depth0.o
	$(CC) $(CFLAGS)   demo/demo_sound.c -o out/demo_sound0.o
	$(CC) $(CFLAGS)   demo/demo_fix.c -o out/demo_fix0.o
	$(CC) $(CFLAGS)   demo/demo_combat.c -o out/demo_combat0.o
	$(CC) $(CFLAGS)   demo/demo_stress.c -o out/demo_stress0.o
	$(CC) $(CFLAGS)   demo/demo_title.c -o out/demo_title0.o
	$(CC) $(CFLAGS)   eyecatcher.c -o out/eyecatcher0.o
	$(OBJCP) $(STRIP_SECTS) out/neogeo0.o   out/neogeo.o
	$(OBJCP) $(STRIP_SECTS) out/user0.o    out/user.o
	$(OBJCP) $(STRIP_SECTS) out/main0.o    out/main.o
	$(OBJCP) $(STRIP_SECTS) out/neogeolib0.o    out/neogeolib.o
	$(OBJCP) $(STRIP_SECTS) out/eyecatcher0.o   out/eyecatcher.o
	$(LD) $(LDFLAGS)    -T sdk/neogeo.ld -o  out/game   out/neogeo.o   out/user.o out/main.o out/neogeolib.o out/eyecatcher.o $(NG_FIX_SDK_OBJ0) $(NG_ENGINE_OBJ0) $(DEMO_OBJ0)
	
777-p1.p1: game
	$(OBJCP)   -O ihex    out/game out/game0
	$(SCAT)  out/game0 -Intel $(CROP) -o out/game0.rom -binary
	$(SCAT)  out/game0.rom -binary $(SWAP) out/game1.rom -binary
	$(SCAT)  out/game1.rom -binary $(FILL) out/game.rom -binary
	cp		 out/game.rom	out/777-p1.p1
	mkdir -p roms/neogeosdk
	cp -f out/777-p1.p1 roms/neogeosdk/777-p1.p1
	python3 hash_eagle/gen_hash.py

.PHONY: hash
hash:
	python3 hash_eagle/gen_hash.py

.PHONY: mml
mml:
	python3 sound/tools/mml_compile.py sound/mml/*.mml -o sound/driver/music_data.inc

.PHONY: fmpatches
fmpatches:
	python3 sound/tools/fm_patch_compile.py sound/fm/patches.fm -o sound/driver/fm_patch_table.inc

.PHONY: fm
fm:
	python3 sound/tools/fm_compile.py sound/fm/*.mml -o sound/driver/fm_data.inc
	
.PHONY: ssgconfig
ssgconfig:
	python3 sound/tools/ssg_config_compile.py sound/ssg/config.ssg -o sound/driver/ssg_config.inc

.PHONY: ssg
ssg:
	python3 sound/tools/ssg_compile.py sound/ssg/*.mml -o sound/driver/ssg_data.inc	

.PHONY: samples
samples:
	cd sound/tools && PYTHON=$(PYTHON) SOX=$(SOX) ./enc_wave16le_a.sh
	cd sound/tools && PYTHON=$(PYTHON) SOX=$(SOX) ./enc_wave16le_b.sh
	cd sound/tools && PYTHON=$(PYTHON) ./adpcm_enc_process.sh

.PHONY: vrom
vrom:
	./sound/tools/vrom.sh
	mkdir -p roms/neogeosdk
	cp -f out/777-v1.v1 roms/neogeosdk/777-v1.v1

.PHONY: m1rom
m1rom: fmpatches fm mml ssgconfig ssg
	WLAZ80=$(WLAZ80) WLALINK=$(WLALINK) USE_Z80C=$(USE_Z80C) Z80C_SRC=$(Z80C_SRC_LINUX) ./sound/tools/m1rom.sh

.PHONY: m1rom-asm
m1rom-asm:
	$(MAKE) m1rom USE_Z80C=0
	mkdir -p out/compare
	cp -f out/777-m1.m1 out/compare/777-m1-asm.m1

.PHONY: m1rom-c
m1rom-c:
	$(MAKE) m1rom USE_Z80C=1 LINK_C_DRIVER=1
	mkdir -p out/compare
	cp -f out/777-m1.m1 out/compare/777-m1-c.m1

.PHONY: compare-driver
compare-driver: m1rom-asm m1rom-c
	python3 sound/tools/compare_m1.py out/compare/777-m1-asm.m1 out/compare/777-m1-c.m1

.PHONY: sound
sound: samples vrom fmpatches fm mml ssgconfig ssg m1rom


.PHONY: sound-all
sound-all: sound


.PHONY: sfix
sfix:
	cd artbox && python3 romdbfiximport.py && python3 fixtiles.py && ./romfx.sh
	mkdir -p roms/neogeosdk
	cp -f artbox/777-s1.s1 roms/neogeosdk/777-s1.s1

.PHONY: srom
srom: sfix

.PHONY: art-clean
art-clean:
	./artbox/makeclean.sh

.PHONY: art
art:
	./artbox/makeartbox.sh

.PHONY: dist
dist: p1
	python3 hash_eagle/gen_hash.py --dist

.PHONY: clean
clean:
	rm -f out/game out/game0 out/game0.rom out/game1.rom out/game.rom out/777-p1.p1
	rm -f out/*.o out/*.s out/game.map dump/*.dump dump/*.hex dump/*.txt dump/*.sym dump/*.gdb dump/*.readelf
	rm -f roms/neogeosdk/777-p1.p1

.PHONY: sound-clean
sound-clean:
	rm -f out/777-m1.m1 out/777-v1.v1 out/driver.gen.asm
	rm -f roms/neogeosdk/777-m1.m1 roms/neogeosdk/777-v1.v1
	rm -f sound/samples/out_16el_a/*.wav sound/samples/out_16el_b/*.wav
	rm -f sound/samples/out_a/*.adpcma sound/samples/out_b/*.adpcmb
	rm -f sound/driver/fm_data.inc sound/driver/music_data.inc sound/driver/fm_patch_table.inc sound/driver/sample_table.inc sound/driver/ssg_config.inc sound/driver/ssg_data.inc


.PHONY: clean-all
clean-all: clean sound-clean art-clean
	rm -f roms/neogeosdk/777-c1.c1 roms/neogeosdk/777-c2.c2
	
.PHONY: dump
dump: 	
	$(OBJDUMP)   -Dht out/neogeo.o | more
	$(OBJDUMP)   -Dht out/user.o | more 
	$(OBJDUMP)   -Dht out/main.o | more 
	$(OBJDUMP)   -Dht out/game  | more
	$(OBJDUMP)   -Dht out/neogeo.o > dump/neogeo.dump
	$(OBJDUMP)   -Dht out/user.o  >  dump/user.dump 
	$(OBJDUMP)   -Dht out/main.o  >  dump/main.dump 
	$(OBJDUMP)   -Dht out/game  >  dump/game.dump
	#$(CC) -Wa,-acdlns  -c sdk/neogeo.c  user.c main.c sdk/neogeolib.c  > out/game.s
	$(INFO) out/game.rom | more 
	$(INFO) out/game.rom > dump/game.hex 

test:
	python3 hash_eagle/gen_hash.py
	cp out/777-p1.p1  roms/neogeosdk/
	$(MAME_COMMON) -output console -nofilter -waitvsync -window

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

debug:
	python3 hash_eagle/gen_hash.py
	cp out/777-p1.p1  roms/neogeosdk/
	$(MAME_COMMON) -output console -debug -verbose -nofilter -waitvsync -window

.PHONY: debug-aes
debug-aes:
	$(MAKE) debug

.PHONY: mame-trace
mame-trace: p1
	mkdir -p dump
	python3 hash_eagle/gen_hash.py
	cp out/777-p1.p1 roms/neogeosdk/
	$(NM) -n out/game > dump/game.sym
	$(OBJDUMP) -Dht out/game > dump/game.debug.dump
	$(MAME_COMMON) -verbose -debug -debugscript dump/mame_trace.mds
	@echo "Trace: dump/m68k_trace.txt  |  Symbols: dump/game.sym  |  Disasm: dump/game.debug.dump"

.PHONY: debug-build
debug-build:
	$(MAKE) DEBUG=1 p1
	$(MAKE) DEBUG=1 debug-artifacts

.PHONY: debug-artifacts
debug-artifacts: out/game
	mkdir -p dump
	rm -f dump/game.size.txt dump/game.sym dump/game.readelf dump/game.debug.dump dump/game.map
	$(SIZE) out/game > dump/game.size.txt
	$(NM) -n out/game > dump/game.sym
	$(READELF) -a out/game > dump/game.readelf
	$(OBJDUMP) -DhtS out/game > dump/game.debug.dump
	if test -f out/game.map; then cp -f out/game.map dump/game.map; fi

.PHONY: gdb-script
gdb-script:
	mkdir -p dump
	printf "set pagination off\nset confirm off\nfile out/game\ninfo files\ninfo functions\ninfo variables\nmaintenance info sections\nquit\n" > dump/gdb_trace.gdb

.PHONY: gdb-trace
gdb-trace: debug-build gdb-script
	@if $(GDB) --version >/dev/null 2>dump/gdb_trace.err; then \
		$(GDB) -batch -x dump/gdb_trace.gdb > dump/gdb_trace.txt 2>>dump/gdb_trace.err; \
	else \
		printf "GDB unavailable: %s\n\n" "$(GDB)" > dump/gdb_trace.txt; \
		cat dump/gdb_trace.err >> dump/gdb_trace.txt; \
		printf "\nOverride with: make gdb-trace GDB=/path/to/m68k-gdb\n" >> dump/gdb_trace.txt; \
	fi

.PHONY: gdb
gdb: debug-build
	$(GDB) out/game

.PHONY: gdb-remote
gdb-remote: debug-build
	$(GDB) -ex "target remote $(GDB_REMOTE)" out/game
