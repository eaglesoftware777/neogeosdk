#######
#https://eaglesoftware.biz
#https://github.com/eaglesoftware777
#https://github.com/eaglesoftware777/neogeosdk
#######
SDKHOME=~
CC=$(SDKHOME)/x-tools/m68k-unknown-elf/bin/m68k-unknown-elf-gcc
CFLAGS= -c  -O0 -fomit-frame-pointer   -Wall  -fno-zero-initialized-in-bss  -march=68000 -mcpu=68000 -mtune=68000 -m68000 -ffreestanding -Wa,-march=68000,-mcpu=68000,-W,--warn
CFLAGS1=-S -O0 -fomit-frame-pointer  -Wall -fno-zero-initialized-in-bss -march=68000  -mcpu=68000 -mtune=6800 -m68000  -ffreestanding
LD=$(SDKHOME)/x-tools/m68k-unknown-elf/bin/m68k-unknown-elf-ld
LDFLAGS=  -nostartfiles -nostdlib
OBJCP=$(SDKHOME)/x-tools/m68k-unknown-elf/bin/m68k-unknown-elf-objcopy
OBJDUMP=$(SDKHOME)/x-tools/m68k-unknown-elf/bin/m68k-unknown-elf-objdump
CROP=-crop 0x000000 0x01FFFF 
SCAT=srec_cat
INFO=xxd -g 2 
SWAP= -byte-swap 2 -o
FILL= -fill 0xFF  0x000000 0x080000 -range-padding 4 -o

.PHONY: all
all:  game 052-p1.p1

game:
	$(CC) $(CFLAGS)   sdk/neogeo.c  -o out/neogeo0.o
	$(CC) $(CFLAGS)   user.c -o out/user0.o
	$(CC) $(CFLAGS)   main.c -o out/main0.o
	$(CC) $(CFLAGS)   sdk/neogeolib.c -o out/neogeolib0.o
	$(OBJCP) -R .comment -R .text -R .data -R .bss out/neogeo0.o   out/neogeo.o
	$(OBJCP) -R .comment -R .text -R .data -R .bss out/user0.o    out/user.o
	$(OBJCP) -R .comment -R .text -R .data -R .bss out/main0.o    out/main.o
	$(OBJCP) -R .comment -R .text -R .data -R .bss out/neogeolib0.o    out/neogeolib.o
	$(LD) $(LDFLAGS)    -T sdk/neogeo.ld -o  out/game   out/neogeo.o   out/user.o out/main.o out/neogeolib.o
	
052-p1.p1: 
	$(OBJCP)   -O ihex    out/game out/game0
	$(SCAT)  out/game0 -Intel $(CROP) -o out/game0.rom -binary
	$(SCAT)  out/game0.rom -binary $(SWAP) out/game1.rom -binary
	$(SCAT)  out/game1.rom -binary $(FILL) out/game.rom -binary
	cp		 out/game.rom	out/052-p1.p1



art-clean:
	./artbox/makeclean.sh
art:
	./artbox/makeartbox.sh

clean:
	rm  out/game  out/*.o out/*.rom out/*.p1  out/*.s dump/*.dump dump/*.hex out/game0
	
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
