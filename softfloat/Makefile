SDKHOME=~
CC=$(SDKHOME)/x-tools/m68k-unknown-elf/bin/m68k-unknown-elf-gcc
OBJCP=$(SDKHOME)/x-tools/m68k-unknown-elf/bin/m68k-unknown-elf-objcopy
LD=$(SDKHOME)/x-tools/m68k-unknown-elf/bin/m68k-unknown-elf-ld
AR=$(SDKHOME)/x-tools/m68k-unknown-elf/bin/m68k-unknown-elf-ar


CFLAGS0=-S -march=68000 -mcpu=68000 -mtune=68000 -m68000 
CFLAGS=-c  -O0 -fomit-frame-pointer   -Wall  -fno-zero-initialized-in-bss  -march=68000 -mcpu=68000 -mtune=68000 -m68000 -ffreestanding -Wa,-march=68000,-mcpu=68000,-W,--warn 
EXTRA1= -DL_double -c -x assembler-with-cpp
EXTRA2= -DL_float -c -x assembler-with-cpp
EXTRA3= -DL_floatex -c -x assembler-with-cpp
EXTRA4= -DL_mulsi3  -c -x assembler-with-cpp
EXTRA5= -DL_nedf2  -c -x assembler-with-cpp
EXTRA6= -DL_umodsi3 -c -x assembler-with-cpp
EXTRA7= -DL_udivsi3 -c -x assembler-with-cpp
EXTRA8= -DL_cvt  -c -x assembler-with-cpp 


all: 
	$(CC) $(CFLAGS) $(EXTRA1) lb1sf68.S  -o double0.o
	$(OBJCP) -R .comment   double0.o   double1.o
	$(OBJCP) --rename-section .data=neogeo_user3 --rename-section .text=neogeo_user2 --rename-section  .bss=neogeo_user4 double1.o double.o
	$(CC) $(CFLAGS) $(EXTRA2) lb1sf68.S  -o float0.o
	$(OBJCP) -R .comment   float0.o   float1.o
	$(OBJCP) --rename-section .data=neogeo_user3 --rename-section .text=neogeo_user2 --rename-section  .bss=neogeo_user4 float1.o float.o
	$(CC) $(CFLAGS) $(EXTRA3) lb1sf68.S  -o floatex0.o
	$(OBJCP) -R .comment   floatex0.o   floatex1.o
	$(OBJCP) --rename-section .data=neogeo_user3 --rename-section .text=neogeo_user2 --rename-section  .bss=neogeo_user4 floatex1.o floatex.o
	$(CC) $(CFLAGS) $(EXTRA4) lb1sf68.S  -o mulsi30.o
	$(OBJCP) -R .comment   mulsi30.o   mulsi31.o
	$(OBJCP) --rename-section .data=neogeo_user3 --rename-section .text=neogeo_user2 --rename-section  .bss=neogeo_user4 mulsi31.o mulsi3.o
	$(CC) $(CFLAGS) $(EXTRA5) lb1sf68.S  -o nedf20.o
	$(OBJCP) -R .comment   nedf20.o   nedf21.o
	$(OBJCP) --rename-section .data=neogeo_user3 --rename-section .text=neogeo_user2 --rename-section  .bss=neogeo_user4 nedf21.o nedf2.o
	$(CC) $(CFLAGS) $(EXTRA6) lb1sf68.S  -o umodsi30.o
	$(OBJCP) -R .comment   umodsi30.o   umodsi31.o
	$(OBJCP) --rename-section .data=neogeo_user3 --rename-section .text=neogeo_user2 --rename-section  .bss=neogeo_user4 umodsi31.o umodsi3.o
	$(CC) $(CFLAGS) $(EXTRA7) lb1sf68.S  -o udivsi30.o
	$(OBJCP) -R .comment   udivsi30.o   udivsi31.o
	$(OBJCP) --rename-section .data=neogeo_user3 --rename-section .text=neogeo_user2 --rename-section  .bss=neogeo_user4 udivsi31.o udivsi3.o
	$(CC) $(CFLAGS) $(EXTR8) lb1sf68.S  -o fpgnu0.o
	$(OBJCP) -R .comment   fpgnu0.o   fpgnu1.o
	$(OBJCP) --rename-section .data=neogeo_user3 --rename-section .text=neogeo_user2 --rename-section  .bss=neogeo_user4 fpgnu1.o fpgnu.o

	$(LD) -r float.o floatex.o double.o mulsi3.o nedf2.o umodsi3.o udivsi3.o fpgnu.o -o soft-float.o
	$(AR)  rvs soft-float.a float.o floatex.o double.o mulsi3.o nedf2.o umodsi3.o udivsi3.o fpgnu.o 
	$(CC) $(CFLAGS)  fpgnulib.c -o libfloat0.o
	$(OBJCP) -R .comment   libfloat0.o   libfloat1.o	
	$(OBJCP) --rename-section .data=neogeo_user3 --rename-section .text=neogeo_user2 --rename-section  .bss=neogeo_user4 libfloat1.o libfloat.o



clean:
	rm *.o
	
