SDKHOME=C:
CC=C:\SysGCC\m68k-elf\bin\m68k-elf-gcc.exe
CFLAGS= -c  -O0 -fomit-frame-pointer   -Wall  -fno-zero-initialized-in-bss  -march=68000 -mcpu=68000 -mtune=68000 -m68000 -ffreestanding -Wa,-march=68000,-mcpu=68000,-W,--warn  
CFLAGS1=-S -O0 -fomit-frame-pointer  -Wall -fno-zero-initialized-in-bss -march=68000  -mcpu=68000 -mtune=6800 -m68000  -ffreestanding
LD=C:\SysGCC\m68k-elf\bin\m68k-elf-ld.exe
LDFLAGS=  -nostartfiles -nostdlib
OBJCP=C:\SysGCC\m68k-elf\bin\m68k-elf-objcopy.exe
OBJDUMP=C:\SysGCC\m68k-elf\bin\m68k-elf-objdump.exe
CROP=-crop 0x000000 0x01FFFF 
SCAT=$(SDKHOME)\neogeosdk\win\srec_cat.exe
INFO=$(SDKHOME)\neogeosdk\win\xxd.exe -g 2 
SWAP= -byte-swap 2 -o
FILL= -fill 0xFF  0x000000 0x080000 -range-padding 4 -o

.PHONY: all
all:  game 052-p1.p1

game:
	$(CC) $(CFLAGS)   sdk\neogeo.c  -o out\neogeo0.o
	$(CC) $(CFLAGS)   user.c -o out\user0.o
	$(CC) $(CFLAGS)   main.c -o out\main0.o
	$(CC) $(CFLAGS)   sdk\neogeolib.c -o out\neogeolib0.o
	$(OBJCP) -R .comment -R .text -R .data -R .bss out\neogeo0.o   out\neogeo.o
	$(OBJCP) -R .comment -R .text -R .data -R .bss out\user0.o    out\user.o
	$(OBJCP) -R .comment -R .text -R .data -R .bss out\main0.o    out\main.o
	$(OBJCP) -R .comment -R .text -R .data -R .bss out\neogeolib0.o    out\neogeolib.o
	$(LD) $(LDFLAGS)    -T sdk\neogeo_win.ld -o  out\game   out\neogeo.o   out\user.o out\main.o out\neogeolib.o
	
052-p1.p1: 
	$(OBJCP)   -O ihex    out\game out\game0
	$(SCAT)  out\game0 -Intel $(CROP) -o out\game0.rom -binary
	$(SCAT)  out\game0.rom -binary $(SWAP) out\game1.rom -binary
	$(SCAT)  out\game1.rom -binary $(FILL) out\game.rom -binary
	copy		 out\game.rom	out\052-p1.p1



art-clean:
	artbox\makeclean.bat
art:
	artbox\makeartbox.bat 

clean:
	del  out\game  out\*.o out\*.rom out\*.p1  dump\*.s dump\*.dump dump\*.hex out\game0
	
.PHONY: dump	
dump: 	
	$(OBJDUMP)   -Dht out\neogeo.o | more
	$(OBJDUMP)   -Dht out\user.o | more 
	$(OBJDUMP)   -Dht out\main.o | more 
	$(OBJDUMP)   -Dht out\game  | more
	$(OBJDUMP)   -Dht out\neogeo.o > dump\neogeo.dump
	$(OBJDUMP)   -Dht out\user.o  >  dump\user.dump 
	$(OBJDUMP)   -Dht out\main.o  >  dump\main.dump 
	$(OBJDUMP)   -Dht out\game  >  dump\game.dump
	$(INFO) out\game.rom | more 
	$(INFO) out\game.rom > dump\game.hex 

test:
	copy out\052-p1.p1  $(SDKHOME)\neogeosdk\roms\ssideki
	$(SDKHOME)\mame\mame.exe -rompath  $(SDKHOME)\neogeosdk\roms  -output console  -nofilter -waitvsync -window ssideki
	
debug:
	copy out\052-p1.p1  $(SDKHOME)\neogeosdk\roms\ssideki
	$(SDKHOME)\mame\mame.exe -rompath   $(SDKHOME)\neogeosdk\roms -output console -debug -verbose  -nofilter -waitvsync -window ssideki
