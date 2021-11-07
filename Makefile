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

all:  game 052-p1.p1

game:
	$(CC) $(CFLAGS)  neogeo.c  -o neogeo0.o
	$(CC) $(CFLAGS)   user.c -o user0.o
	$(CC) $(CFLAGS)   main.c -o main0.o
	$(CC) $(CFLAGS)   neogeolib.c -o neogeolib0.o
	$(CC) -Wa,-adlns  -c neogeo.c  user.c main.c neogeolib.c  > game.s
	$(OBJCP) -R .comment -R .text -R .data -R .bss neogeo0.o   neogeo.o
	$(OBJCP) -R .comment -R .text -R .data -R .bss user0.o    user.o
	$(OBJCP) -R .comment -R .text -R .data -R .bss main0.o    main.o
	$(OBJCP) -R .comment -R .text -R .data -R .bss neogeolib0.o    neogeolib.o
	$(LD) $(LDFLAGS)    -T neogeo.ld -o  game   neogeo.o   user.o main.o neogeolib.o
	
052-p1.p1: 
	$(OBJCP)   -O ihex    game game0
	$(SCAT)  game0 -Intel $(CROP) -o game0.rom -binary
	$(SCAT)  game0.rom -binary $(SWAP) game1.rom -binary
	$(SCAT)  game1.rom -binary $(FILL) game.rom -binary
	cp		 game.rom	052-p1.p1



art-clean:
	./artbox/makeclean.sh
art:
	./artbox/makeartbox.sh

clean:
	rm  game  *.o *.rom *.p1  *.s *.dump *.hex game0
	
dump: 	
	$(OBJDUMP)   -Dht neogeo.o | more
	$(OBJDUMP)   -Dht user.o | more 
	$(OBJDUMP)   -Dht main.o | more 
	$(OBJDUMP)   -Dht game  | more
	$(OBJDUMP)   -Dht neogeo.o > neogeo.dump
	$(OBJDUMP)   -Dht user.o  >  user.dump 
	$(OBJDUMP)   -Dht main.o  >  main.dump 
	$(OBJDUMP)   -Dht game  >  game.dump
	$(INFO) game.rom | more 
	$(INFO) game.rom > game.hex 

test:
	cp 052-p1.p1  $(SDKHOME)/neogeosdk/roms/ssideki
	mame -rompath  $(SDKHOME)/neogeosdk/roms -output console  -nofilter -waitvsync -window ssideki
	
debug:
	cp 052-p1.p1  $(SDKHOME)/neogeosdk/roms/ssideki
	mame -rompath  $(SDKHOME)/neogeosdk/roms -output console -debug -verbose  -nofilter -waitvsync -window ssideki
