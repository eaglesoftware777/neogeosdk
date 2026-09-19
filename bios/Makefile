# ============================================================================
#  EagleBIOS Makefile
#  Compiles, links, and produces 128KB Neo Geo compatible BIOS ROM images
# ============================================================================

TOOLCHAIN ?= /mnt/c/neogeo/x-tools-v3/m68k-unknown-elf/bin
CC        = $(TOOLCHAIN)/m68k-unknown-elf-gcc
AS        = $(TOOLCHAIN)/m68k-unknown-elf-as
LD        = $(TOOLCHAIN)/m68k-unknown-elf-ld
OBJCOPY   = $(TOOLCHAIN)/m68k-unknown-elf-objcopy
SREC_CAT ?= /usr/bin/srec_cat

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
TARGET_ROM = eagle_bios.rom
TARGET_SP1 = sp-s2.sp1
TARGET_SM1 = sm1.sm1

.PHONY: all clean test

all: $(TARGET_SP1) $(TARGET_SM1)
	@echo "========================================================"
	@echo " EagleBIOS successfully built!"
	@echo " Output 1 (Big-Endian Linear): bios/$(TARGET_ROM) (128 KB)"
	@echo " Output 2 (Byte-Swapped MVS):  bios/$(TARGET_SP1) (128 KB)"
	@echo " Output 3 (Open Sound SM1):    bios/$(TARGET_SM1) (128 KB)"
	@echo "========================================================"

out:
	mkdir -p out

out/bios_entry.o: src/bios_entry.s | out
	$(AS) $(ASFLAGS) $< -o $@

out/%.o: src/%.c | out
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

out/bios_sound.o: src/bios_sound.s | out
	$(WLAZ80) -o $@ $<

out/sm1.link: out/bios_sound.o | out
	printf "[objects]\n%s\n" "$<" > $@

out/sm1_raw.bin: out/sm1.link out/bios_sound.o
	$(WLALINK) -S -r $< $@

$(TARGET_SM1): out/sm1_raw.bin
	$(SREC_CAT) $< -binary -fill 0xFF 0x000000 0x020000 -o $@ -binary

clean:
	rm -rf out $(TARGET_ROM) $(TARGET_SP1) $(TARGET_SM1) eagle_bios_swapped.rom
