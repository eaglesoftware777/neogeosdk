/*OUTPUT_FORMAT("ihex")*/
MEMORY
{
  memory_ivt         (rx) : ORIGIN = 0x0  , LENGTH = 256
  memory_init        (rx) : ORIGIN = 0x100, LENGTH = 22
  memory_entry       (rx) : ORIGIN = 0x116, LENGTH = 36
  memory_padds       (rx) : ORIGIN = 0x13a, LENGTH = 71
  memory_id2         (rx) : ORIGIN = 0x182, LENGTH = 126
  memory_gamedata    (rx) : ORIGIN = 0x200, LENGTH = 384
  memory_user        (rx) : ORIGIN = 0x380, LENGTH = 400K



}

SECTIONS
{
	   . = 0x0;
     neogeo_ivt : {  out\neogeo.o(neogeo_ivt)      } > memory_ivt AT > memory_ivt
	   neogeo_init : {  out\neogeo.o(neogeo_init)     } > memory_init AT > memory_init 
	   neogeo_entry : {  out\neogeo.o(neogeo_entry)     } > memory_entry AT > memory_entry =0xFF
	   neogeo_padds : {  out\neogeo.o(neogeo_padds)    } > memory_padds AT > memory_padds =0xFF
	   neogeo_id2 : {  out\neogeo.o(neogeo_id2)      } > memory_id2 AT > memory_id2
	   neogeo_gamedata : {  out\neogeo.o(neogeo_gamedata) } > memory_gamedata AT > memory_gamedata
	   neogeo_user : {  out\user.o(neogeo_user) 
	                    out\user.o(.rodata) 
	   	                out\main.o(neogeo_user)
	   	                out\main.o(.rodata) 
						out\neogeolib.o(neogeo_user)
						softfloat\soft-float.o(neogeo_user2)
						softfloat\soft-float.o(neogeo_user3)
						softfloat\soft-float.o(neogeo_user4)
						softfloat\libfloat.o(neogeo_user2)
						softfloat\libfloat.o(neogeo_user3)
						softfloat\libfloat.o(neogeo_user4)
						} > memory_user =0xFF
}
