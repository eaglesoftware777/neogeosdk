OUTPUT_ARCH(m68k)
ENTRY(eagle_cd_reset)
MEMORY {
    rom (rx) : ORIGIN = 0xC00000, LENGTH = 512K
    private (rw) : ORIGIN = 0x1F0000, LENGTH = 16K
}
SECTIONS {
    .vectors 0xC00000 : { KEEP(*(.vectors)) } > rom
    .services 0xC00400 : { KEEP(*(.services)) } > rom
    .identity 0xC00700 : { KEEP(*(.identity)) } > rom
    .text 0xC01000 : { *(.text*) *(.rodata*) } > rom
    .data : { __data_start = .; *(.data*) __data_end = .; } > private AT > rom
    __data_load = LOADADDR(.data);
    .bss (NOLOAD) : { __bss_start = .; *(.bss*) *(COMMON) __bss_end = .; } > private
    /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) }
}
ASSERT(SIZEOF(.vectors) == 0x400, "CD vector table must contain256 vectors")
ASSERT(SIZEOF(.services) == 0x300, "CD service table moved")
ASSERT(SIZEOF(.identity) <= 0x100, "CD identification overlaps ROM code")
ASSERT(__bss_end <= 0x1F4000, "CD firmware private RAM exhausted")
