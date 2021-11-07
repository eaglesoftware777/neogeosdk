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
     neogeo_ivt : {  neogeo.o(neogeo_ivt)      } > memory_ivt AT > memory_ivt
	   neogeo_init : {  neogeo.o(neogeo_init)     } > memory_init AT > memory_init 
	   neogeo_entry : {  neogeo.o(neogeo_entry)     } > memory_entry AT > memory_entry =0xFF
	   neogeo_padds : {  neogeo.o(neogeo_padds)    } > memory_padds AT > memory_padds =0xFF
	   neogeo_id2 : {  neogeo.o(neogeo_id2)      } > memory_id2 AT > memory_id2
	   neogeo_gamedata : {  neogeo.o(neogeo_gamedata) } > memory_gamedata AT > memory_gamedata
	   neogeo_user : {  user.o(neogeo_user) 
	                    user.o(.rodata) 
	   	                main.o(neogeo_user)
	   	                main.o(.rodata) 
						neogeolib.o(neogeo_user)
						softfloat/soft-float.o(neogeo_user2)
						softfloat/soft-float.o(neogeo_user3)
						softfloat/soft-float.o(neogeo_user4)
						softfloat/libfloat.o(neogeo_user2)
						softfloat/libfloat.o(neogeo_user3)
						softfloat/libfloat.o(neogeo_user4)
						} > memory_user =0xFF
}
