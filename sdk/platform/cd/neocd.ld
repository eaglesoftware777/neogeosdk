OUTPUT_ARCH(m68k)
ENTRY(ng_cd_user_entry)
MEMORY {
    resident (rx) : ORIGIN = 0, LENGTH = 512K
    work (rw) : ORIGIN = 0x100800, LENGTH = 0xE800
}
SECTIONS {
    .header 0 : { KEEP(*(.cd_header)) } > resident
    .text 0x200 : { *(.text*) *(neogeo_user) *(.rodata*) } > resident
    .data : { __data_start = .; *(.data*) __data_end = .; } > work AT > resident
    __data_load = LOADADDR(.data);
    .bss (NOLOAD) : { __bss_start = .; *(.bss*) *(COMMON) __bss_end = .; } > work
    /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) }
}
ASSERT(SIZEOF(.header) == 0x200, "Native CD header size changed")
ASSERT(__bss_end <= 0x10F000, "CD program overlaps stock BIOS/stack reservation")
