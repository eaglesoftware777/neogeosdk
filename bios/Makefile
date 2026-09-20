# ============================================================================
#  EagleBIOS Makefile
#  Compiles, links, and produces 128KB Neo Geo compatible BIOS ROM images
# ============================================================================

SDKHOME ?= $(abspath ../..)
TOOLCHAIN ?= $(firstword $(foreach version,x-tools-v3 x-tools-v2 x-tools,$(wildcard $(SDKHOME)/$(version)/m68k-unknown-elf/bin)))
CROSS     = $(if $(strip $(TOOLCHAIN)),$(TOOLCHAIN)/)m68k-unknown-elf-
CC        = $(CROSS)gcc
AS        = $(CROSS)as
LD        = $(CROSS)ld
OBJCOPY   = $(CROSS)objcopy
SREC_CAT ?= srec_cat

WLAZ80   ?= wla-z80
WLALINK  ?= wlalink

INCLUDES  = -Iinclude -I.
CFLAGS    = -O2 -fomit-frame-pointer -Wall -Wextra -Wno-array-bounds -march=68000 -mcpu=68000 -m68000 \
            -ffreestanding -nostdlib -fno-builtin $(INCLUDES)
ASFLAGS   = -march=68000 -mcpu=68000
LDFLAGS   = -nostdlib -T eagle_bios.ld

SRCS_C    = src/bios_main.c \
            src/bios_video.c \
            src/bios_io.c \
            src/bios_rtc.c \
            src/bios_card.c \
            src/bios_splash.c \
            src/bios_menu.c

SRCS_S    = src/bios_entry.s

OBJS      = out/bios_entry.o \
            out/bios_main.o \
            out/bios_video.o \
            out/bios_io.o \
            out/bios_rtc.o \
            out/bios_card.o \
            out/bios_splash.o \
            out/bios_menu.o

TARGET_ELF = out/eagle_bios.elf
TARGET_RAW = out/eagle_bios_raw.bin
TARGET_ROM  = eagle_bios.rom
TARGET_SP1  = sp-s2.sp1
TARGET_AES  = neo-epo.bin
TARGET_SM1  = sm1.sm1
TARGET_SFIX = sfix.sfix
TARGET_LO   = 000-lo.lo

PYTHON     ?= python3

.PHONY: all clean test test-games install

all: $(TARGET_SP1) $(TARGET_AES) $(TARGET_SM1) $(TARGET_SFIX) $(TARGET_LO)
	@echo "========================================================"
	@echo " EagleBIOS Firmware Suite successfully built!"
	@echo " Output 1 (Big-Endian Linear): bios/$(TARGET_ROM) (128 KB)"
	@echo " Output 2 (Byte-Swapped MVS):  bios/$(TARGET_SP1) (128 KB)"
	@echo " Output 3 (Byte-Swapped AES):  bios/$(TARGET_AES) (128 KB)"
	@echo " Output 4 (Open Sound SM1):    bios/$(TARGET_SM1) (128 KB)"
	@echo " Output 5 (Open System SFIX):  bios/$(TARGET_SFIX) (128 KB)"
	@echo " Output 6 (Open Scaling LO):   bios/$(TARGET_LO) (128 KB)"
	@echo "========================================================"

out:
	mkdir -p out

out/bios_entry.o: src/bios_entry.s | out
	$(AS) $(ASFLAGS) --defsym BIOS_ID=0x8002 $< -o $@

out/bios_entry_aes.o: src/bios_entry.s | out
	$(AS) $(ASFLAGS) --defsym BIOS_ID=0x0002 $< -o $@

out/%.o: src/%.c include/bios.h | out
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET_ELF): $(OBJS) eagle_bios.ld
	$(LD) $(LDFLAGS) $(OBJS) -o $@

$(TARGET_RAW): $(TARGET_ELF)
	$(OBJCOPY) -O binary $< $@

$(TARGET_ROM): $(TARGET_RAW)
	$(SREC_CAT) $< -binary -fill 0xFF 0x000000 0x020000 -o $@ -binary

$(TARGET_SP1): $(TARGET_ROM)
	$(SREC_CAT) $< -binary -byte-swap 2 -o $@ -binary
	cp -f $@ eagle_bios_swapped.rom

out/eagle_aes.elf: out/bios_entry_aes.o $(filter-out out/bios_entry.o,$(OBJS)) eagle_bios.ld
	$(LD) $(LDFLAGS) out/bios_entry_aes.o $(filter-out out/bios_entry.o,$(OBJS)) -o $@

out/eagle_aes_raw.bin: out/eagle_aes.elf
	$(OBJCOPY) -O binary $< $@

$(TARGET_AES): out/eagle_aes_raw.bin
	$(SREC_CAT) $< -binary -fill 0xFF 0x000000 0x020000 -byte-swap 2 -o $@ -binary

out/bios_sound.o: src/bios_sound.s | out
	$(WLAZ80) -o $@ $<

out/sm1.link: out/bios_sound.o | out
	printf "[objects]\n%s\n" "$<" > $@

out/sm1_raw.bin: out/sm1.link out/bios_sound.o
	$(WLALINK) -S -r $< $@

$(TARGET_SM1): out/sm1_raw.bin
	$(SREC_CAT) $< -binary -fill 0xFF 0x000000 0x020000 -o $@ -binary

$(TARGET_SFIX): tools/gen_sfix.py
	$(PYTHON) tools/gen_sfix.py $@

$(TARGET_LO): tools/gen_lo.py
	$(PYTHON) tools/gen_lo.py $@

# Firmware contract on both boards, then the system eye-catcher path.
test: all
	$(PYTHON) test_bios.py --game probe --platform both
	$(PYTHON) test_bios.py --game probe0 --platform aes

# Every SDK cartridge built into ../roms, on the arcade firmware.
test-games: all
	@set -e; for game in helloworld demo tutorial neogeogame skylance maiya; do \
	    if [ -f ../roms/$$game/*-p1.p1 ]; then $(PYTHON) test_bios.py --game $$game --platform mvs; fi; \
	done

# Copy the fresh firmware where the launch scripts and MAME look for it.
install: all
	mkdir -p test_roms/neogeo test_roms/aes
	cp -f $(TARGET_SP1) $(TARGET_SM1) $(TARGET_SFIX) $(TARGET_LO) test_roms/neogeo/
	cp -f $(TARGET_AES) $(TARGET_SM1) $(TARGET_SFIX) $(TARGET_LO) test_roms/aes/

clean:
	rm -rf out $(TARGET_ROM) $(TARGET_SP1) $(TARGET_AES) $(TARGET_SM1) $(TARGET_SFIX) $(TARGET_LO) eagle_bios_swapped.rom
