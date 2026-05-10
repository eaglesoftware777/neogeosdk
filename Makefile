#######
#https://eaglesoftware.biz
#https://github.com/eaglesoftware777
#https://github.com/eaglesoftware777/neogeosdk
#######
ifndef SDKHOME
SDKHOME := $(abspath $(CURDIR)/..)
endif
CC=$(SDKHOME)/x-tools/m68k-unknown-elf/bin/m68k-unknown-elf-gcc
CFLAGS= -c  -O0 -fomit-frame-pointer   -Wall  -fno-zero-initialized-in-bss  -march=68000 -mcpu=68000 -mtune=68000 -m68000 -ffreestanding -Wa,-march=68000,-mcpu=68000,-W,--warn
CFLAGS1=-S -O0 -fomit-frame-pointer  -Wall -fno-zero-initialized-in-bss -march=68000  -mcpu=68000 -mtune=68000 -m68000  -ffreestanding
LD=$(SDKHOME)/x-tools/m68k-unknown-elf/bin/m68k-unknown-elf-ld
LDFLAGS=  -nostartfiles -nostdlib
OBJCP=$(SDKHOME)/x-tools/m68k-unknown-elf/bin/m68k-unknown-elf-objcopy
OBJDUMP=$(SDKHOME)/x-tools/m68k-unknown-elf/bin/m68k-unknown-elf-objdump
GDB=$(SDKHOME)/x-tools/m68k-unknown-elf/bin/m68k-unknown-elf-gdb
NM=$(SDKHOME)/x-tools/m68k-unknown-elf/bin/m68k-unknown-elf-nm
READELF=$(SDKHOME)/x-tools/m68k-unknown-elf/bin/m68k-unknown-elf-readelf
ADDR2LINE=$(SDKHOME)/x-tools/m68k-unknown-elf/bin/m68k-unknown-elf-addr2line
SIZE=$(SDKHOME)/x-tools/m68k-unknown-elf/bin/m68k-unknown-elf-size
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
NG_ENGINE_NAMES=ng_defs ng_properties ng_game_time ng_timers ng_progress ng_status ng_game_events ng_level ng_fix ng_sprite_group ng_actions ng_chars ng_npcs ng_physics ng_border_constraints ng_game_interupt
NG_ENGINE_OBJ0=$(addprefix out/,$(addsuffix 0.o,$(NG_ENGINE_NAMES)))

ifeq ($(DEBUG),1)
CFLAGS += -g3 -gdwarf-2 -DNG_DEBUG=1
LDFLAGS += -Map=out/game.map
endif

.DEFAULT_GOAL := p1

.PHONY: all
all: art sfix sound p1

.PHONY: p1
p1: game 052-p1.p1

game:
	$(CC) $(CFLAGS)   sdk/neogeo.c  -o out/neogeo0.o
	$(CC) $(CFLAGS)   user.c -o out/user0.o
	$(CC) $(CFLAGS)   main.c -o out/main0.o
	$(CC) $(CFLAGS)   sdk/neogeolib.c -o out/neogeolib0.o
	$(CC) $(CFLAGS)   sdk/ng_defs.c -o out/ng_defs0.o
	$(CC) $(CFLAGS)   sdk/ng_properties.c -o out/ng_properties0.o
	$(CC) $(CFLAGS)   sdk/ng_game_time.c -o out/ng_game_time0.o
	$(CC) $(CFLAGS)   sdk/ng_timers.c -o out/ng_timers0.o
	$(CC) $(CFLAGS)   sdk/ng_progress.c -o out/ng_progress0.o
	$(CC) $(CFLAGS)   sdk/ng_status.c -o out/ng_status0.o
	$(CC) $(CFLAGS)   sdk/ng_game_events.c -o out/ng_game_events0.o
	$(CC) $(CFLAGS)   sdk/ng_level.c -o out/ng_level0.o
	$(CC) $(CFLAGS)   sdk/ng_fix.c -o out/ng_fix0.o
	$(CC) $(CFLAGS)   sdk/ng_sprite_group.c -o out/ng_sprite_group0.o
	$(CC) $(CFLAGS)   sdk/ng_actions.c -o out/ng_actions0.o
	$(CC) $(CFLAGS)   sdk/ng_chars.c -o out/ng_chars0.o
	$(CC) $(CFLAGS)   sdk/ng_npcs.c -o out/ng_npcs0.o
	$(CC) $(CFLAGS)   sdk/ng_physics.c -o out/ng_physics0.o
	$(CC) $(CFLAGS)   sdk/ng_border_constraints.c -o out/ng_border_constraints0.o
	$(CC) $(CFLAGS)   sdk/ng_game_interupt.c -o out/ng_game_interupt0.o
	$(OBJCP) -R .comment -R .text -R .data -R .bss out/neogeo0.o   out/neogeo.o
	$(OBJCP) -R .comment -R .text -R .data -R .bss out/user0.o    out/user.o
	$(OBJCP) -R .comment -R .text -R .data -R .bss out/main0.o    out/main.o
	$(OBJCP) -R .comment -R .text -R .data -R .bss out/neogeolib0.o    out/neogeolib.o
	$(LD) $(LDFLAGS)    -T sdk/neogeo.ld -o  out/game   out/neogeo.o   out/user.o out/main.o out/neogeolib.o $(NG_ENGINE_OBJ0)
	
052-p1.p1: game
	$(OBJCP)   -O ihex    out/game out/game0
	$(SCAT)  out/game0 -Intel $(CROP) -o out/game0.rom -binary
	$(SCAT)  out/game0.rom -binary $(SWAP) out/game1.rom -binary
	$(SCAT)  out/game1.rom -binary $(FILL) out/game.rom -binary
	cp		 out/game.rom	out/052-p1.p1
	mkdir -p roms/ssideki
	cp -f out/052-p1.p1 roms/ssideki/052-p1.p1

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
	mkdir -p roms/ssideki
	cp -f out/052-v1.v1 roms/ssideki/052-v1.v1

.PHONY: m1rom
m1rom: fmpatches fm mml ssgconfig ssg
	WLAZ80=$(WLAZ80) WLALINK=$(WLALINK) USE_Z80C=$(USE_Z80C) Z80C_SRC=$(Z80C_SRC_LINUX) ./sound/tools/m1rom.sh
	mkdir -p roms/ssideki
	cp -f out/052-m1.m1 roms/ssideki/052-m1.m1
	cp -f out/052-m1.m1 roms/ssideki/sm1.sm1

.PHONY: m1rom-asm
m1rom-asm:
	$(MAKE) m1rom USE_Z80C=0
	mkdir -p out/compare
	cp -f out/052-m1.m1 out/compare/052-m1-asm.m1

.PHONY: m1rom-c
m1rom-c:
	$(MAKE) m1rom USE_Z80C=1 LINK_C_DRIVER=1
	mkdir -p out/compare
	cp -f out/052-m1.m1 out/compare/052-m1-c.m1

.PHONY: compare-driver
compare-driver: m1rom-asm m1rom-c
	python3 sound/tools/compare_m1.py out/compare/052-m1-asm.m1 out/compare/052-m1-c.m1

.PHONY: sound
sound: samples vrom fmpatches fm mml ssgconfig ssg m1rom


.PHONY: sound-all
sound-all: sound


.PHONY: sfix
sfix:
	cd artbox && python3 romdbfiximport.py && python3 fixtiles.py && ./romfx.sh
	mkdir -p roms/ssideki
	cp -f artbox/052-s1.s1 roms/ssideki/052-s1.s1

.PHONY: srom
srom: sfix

.PHONY: art-clean
art-clean:
	./artbox/makeclean.sh

.PHONY: art
art:
	./artbox/makeartbox.sh

.PHONY: clean
clean:
	rm -f out/game out/game0 out/game0.rom out/game1.rom out/game.rom out/052-p1.p1
	rm -f out/*.o out/*.s out/game.map dump/*.dump dump/*.hex dump/*.txt dump/*.sym dump/*.gdb dump/*.readelf
	rm -f roms/ssideki/052-p1.p1

.PHONY: sound-clean
sound-clean:
	rm -f out/052-m1.m1 out/052-v1.v1 out/driver.gen.asm
	rm -f roms/ssideki/052-m1.m1 roms/ssideki/052-v1.v1 roms/ssideki/sm1.sm1
	rm -f sound/samples/out_16el_a/*.wav sound/samples/out_16el_b/*.wav
	rm -f sound/samples/out_a/*.adpcma sound/samples/out_b/*.adpcmb
	rm -f sound/driver/fm_data.inc sound/driver/music_data.inc sound/driver/fm_patch_table.inc sound/driver/sample_table.inc sound/driver/ssg_config.inc sound/driver/ssg_data.inc


.PHONY: clean-all
clean-all: clean sound-clean art-clean
	rm -f roms/ssideki/052-c1.c1 roms/ssideki/052-c2.c2
	
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
	cp out/052-p1.p1  $(SDKHOME)/neogeosdk/roms/ssideki
	mame -rompath  $(SDKHOME)/neogeosdk/roms -output console  -nofilter -waitvsync -window ssideki
	
debug:
	cp out/052-p1.p1  $(SDKHOME)/neogeosdk/roms/ssideki
	mame -rompath  $(SDKHOME)/neogeosdk/roms -output console -debug -verbose  -nofilter -waitvsync -window ssideki

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
