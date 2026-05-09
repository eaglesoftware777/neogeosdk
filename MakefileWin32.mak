#######
#https://eaglesoftware.biz
#https://github.com/eaglesoftware777
#https://github.com/eaglesoftware777/neogeosdk
#######
ifndef SDKHOME
SDKHOME := $(abspath $(CURDIR)/..)
endif

M68K_ELF_ROOT?=C:\SysGCC\m68k-elf
M68K_ELF_BIN=$(M68K_ELF_ROOT)\bin
REPO_WIN=$(subst /,\,$(CURDIR))

CC=$(M68K_ELF_BIN)\m68k-elf-gcc.exe
CFLAGS= -c  -O0 -fomit-frame-pointer   -Wall  -fno-zero-initialized-in-bss  -march=68000 -mcpu=68000 -mtune=68000 -m68000 -ffreestanding -Wa,-march=68000,-mcpu=68000,-W,--warn
CFLAGS1=-S -O0 -fomit-frame-pointer  -Wall -fno-zero-initialized-in-bss -march=68000  -mcpu=68000 -mtune=68000 -m68000  -ffreestanding
LD=$(M68K_ELF_BIN)\m68k-elf-ld.exe
LDFLAGS=  -nostartfiles -nostdlib
OBJCP=$(M68K_ELF_BIN)\m68k-elf-objcopy.exe
OBJDUMP=$(M68K_ELF_BIN)\m68k-elf-objdump.exe

WLAZ80?=wla-z80
WLALINK?=wlalink
PY?=py
SOX?=
MAME?=mame

FM_MMLS:=$(wildcard sound/fm/*.mml)
MML_TRACKS:=$(wildcard sound/mml/*.mml)
SSG_MMLS:=$(wildcard sound/ssg/*.mml)

CROP=-crop 0x000000 0x01FFFF
SCAT=$(REPO_WIN)\win\srec_cat.exe
INFO=$(REPO_WIN)\win\xxd.exe -g 2
SWAP= -byte-swap 2 -o
FILL= -fill 0xFF  0x000000 0x080000 -range-padding 4 -o
NG_ENGINE_OBJ0=out\ng_defs0.o out\ng_properties0.o out\ng_game_time0.o out\ng_timers0.o out\ng_progress0.o out\ng_status0.o out\ng_game_events0.o out\ng_sprite_group0.o out\ng_actions0.o out\ng_chars0.o out\ng_border_constraints0.o out\ng_game_interupt0.o

.DEFAULT_GOAL := p1

.PHONY: all
all: art sfix sound p1

.PHONY: p1
p1: game 052-p1.p1

game:
	$(CC) $(CFLAGS) sdk\neogeo.c -o out\neogeo0.o
	$(CC) $(CFLAGS) user.c -o out\user0.o
	$(CC) $(CFLAGS) main.c -o out\main0.o
	$(CC) $(CFLAGS) sdk\neogeolib.c -o out\neogeolib0.o
	$(CC) $(CFLAGS) sdk\ng_defs.c -o out\ng_defs0.o
	$(CC) $(CFLAGS) sdk\ng_properties.c -o out\ng_properties0.o
	$(CC) $(CFLAGS) sdk\ng_game_time.c -o out\ng_game_time0.o
	$(CC) $(CFLAGS) sdk\ng_timers.c -o out\ng_timers0.o
	$(CC) $(CFLAGS) sdk\ng_progress.c -o out\ng_progress0.o
	$(CC) $(CFLAGS) sdk\ng_status.c -o out\ng_status0.o
	$(CC) $(CFLAGS) sdk\ng_game_events.c -o out\ng_game_events0.o
	$(CC) $(CFLAGS) sdk\ng_sprite_group.c -o out\ng_sprite_group0.o
	$(CC) $(CFLAGS) sdk\ng_actions.c -o out\ng_actions0.o
	$(CC) $(CFLAGS) sdk\ng_chars.c -o out\ng_chars0.o
	$(CC) $(CFLAGS) sdk\ng_border_constraints.c -o out\ng_border_constraints0.o
	$(CC) $(CFLAGS) sdk\ng_game_interupt.c -o out\ng_game_interupt0.o
	$(OBJCP) -R .comment -R .text -R .data -R .bss out\neogeo0.o out\neogeo.o
	$(OBJCP) -R .comment -R .text -R .data -R .bss out\user0.o out\user.o
	$(OBJCP) -R .comment -R .text -R .data -R .bss out\main0.o out\main.o
	$(OBJCP) -R .comment -R .text -R .data -R .bss out\neogeolib0.o out\neogeolib.o
	$(LD) $(LDFLAGS) -T sdk\neogeo_win.ld -o out\game out\neogeo.o out\user.o out\main.o out\neogeolib.o $(NG_ENGINE_OBJ0)

052-p1.p1: game
	$(OBJCP) -O ihex out\game out\game0
	$(SCAT) out\game0 -Intel $(CROP) -o out\game0.rom -binary
	$(SCAT) out\game0.rom -binary $(SWAP) out\game1.rom -binary
	$(SCAT) out\game1.rom -binary $(FILL) out\game.rom -binary
	copy /Y out\game.rom out\052-p1.p1
	if not exist roms\ssideki mkdir roms\ssideki
	copy /Y out\052-p1.p1 roms\ssideki\052-p1.p1

.PHONY: samples
samples:
	cd sound\tools && set PY=$(PY)&& set SOX=$(SOX)&& call enc_wave16le_a.bat
	cd sound\tools && set PY=$(PY)&& set SOX=$(SOX)&& call enc_wave16le_b.bat
	cd sound\tools && set PY=$(PY)&& call adpcm_enc_process.bat

.PHONY: vrom
vrom:
	call sound\tools\vrom.bat
	if not exist roms\ssideki mkdir roms\ssideki
	copy /Y out\052-v1.v1 roms\ssideki\052-v1.v1

.PHONY: fmpatches
fmpatches:
	$(PY) sound\tools\fm_patch_compile.py sound\fm\patches.fm -o sound\driver\fm_patch_table.inc

.PHONY: fm
fm:
	$(PY) sound/tools/fm_compile.py $(FM_MMLS) -o sound/driver/fm_data.inc

.PHONY: mml
mml:
	$(PY) sound/tools/mml_compile.py $(MML_TRACKS) -o sound/driver/music_data.inc
	
.PHONY: ssgconfig
ssgconfig:
	$(PY) sound\tools\ssg_config_compile.py sound\ssg\config.ssg -o sound\driver\ssg_config.inc

.PHONY: ssg
ssg:
	$(PY) sound/tools/ssg_compile.py $(SSG_MMLS) -o sound/driver/ssg_data.inc

.PHONY: m1rom
m1rom: fmpatches fm mml ssgconfig ssg
	set WLAZ80=$(WLAZ80)&& set WLALINK=$(WLALINK)&& set USE_Z80C=$(USE_Z80C)&& set Z80C_SRC=$(Z80C_SRC_WIN)&& call sound\tools\m1rom.bat
	if not exist roms\ssideki mkdir roms\ssideki
	copy /Y out\052-m1.m1 roms\ssideki\052-m1.m1
	copy /Y out\052-m1.m1 roms\ssideki\sm1.sm1

.PHONY: m1rom-asm
m1rom-asm:
	$(MAKE) m1rom USE_Z80C=0
	if not exist out\compare mkdir out\compare
	copy /Y out\052-m1.m1 out\compare\052-m1-asm.m1

.PHONY: m1rom-c
m1rom-c:
	$(MAKE) m1rom USE_Z80C=1 LINK_C_DRIVER=1
	if not exist out\compare mkdir out\compare
	copy /Y out\052-m1.m1 out\compare\052-m1-c.m1

.PHONY: compare-driver
compare-driver: m1rom-asm m1rom-c
	$(PY) sound\tools\compare_m1.py out\compare\052-m1-asm.m1 out\compare\052-m1-c.m1

.PHONY: sound
sound: samples vrom fmpatches fm mml ssgconfig ssg m1rom

.PHONY: sound-all
sound-all: sound

.PHONY: sfix
sfix:
	cd artbox && py romdbfiximport.py && py fixtiles.py
	if not exist roms\ssideki mkdir roms\ssideki
	copy /Y artbox\052-s1.s1 roms\ssideki\052-s1.s1

.PHONY: srom
srom: sfix

.PHONY: art-clean
art-clean:
	call artbox\makeclean.bat

.PHONY: art
art:
	call artbox\makeartbox.bat

.PHONY: clean
clean:
	if exist out\game del /Q out\game
	if exist out\game0 del /Q out\game0
	if exist out\game0.rom del /Q out\game0.rom
	if exist out\game1.rom del /Q out\game1.rom
	if exist out\game.rom del /Q out\game.rom
	if exist out\052-p1.p1 del /Q out\052-p1.p1
	if exist out\*.o del /Q out\*.o
	if exist out\*.s del /Q out\*.s
	if exist dump\*.dump del /Q dump\*.dump
	if exist dump\*.hex del /Q dump\*.hex
	if exist roms\ssideki\052-p1.p1 del /Q roms\ssideki\052-p1.p1

.PHONY: sound-clean
sound-clean:
	if exist out\052-m1.m1 del /Q out\052-m1.m1
	if exist out\052-v1.v1 del /Q out\052-v1.v1
	if exist out\driver.gen.asm del /Q out\driver.gen.asm
	if exist roms\ssideki\052-m1.m1 del /Q roms\ssideki\052-m1.m1
	if exist roms\ssideki\052-v1.v1 del /Q roms\ssideki\052-v1.v1
	if exist roms\ssideki\sm1.sm1 del /Q roms\ssideki\sm1.sm1
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
	if exist roms\ssideki\052-c1.c1 del /Q roms\ssideki\052-c1.c1
	if exist roms\ssideki\052-c2.c2 del /Q roms\ssideki\052-c2.c2

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

test:
	copy /Y out\052-p1.p1 roms\ssideki
	$(MAME) -rompath $(REPO_WIN)\roms -output console -nofilter -waitvsync -window ssideki

debug:
	copy /Y out\052-p1.p1 roms\ssideki
	$(MAME) -rompath $(REPO_WIN)\roms -output console -debug -verbose -nofilter -waitvsync -window ssideki
