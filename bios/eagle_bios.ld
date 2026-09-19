/*
 * ============================================================================
 *  EagleBIOS Linker Script for Motorola 68000
 *  Maps 128KB System ROM at 0xC00000 .. 0xC1FFFF
 *  Maps BIOS Work RAM at 0x10F400 .. 0x10FBFF
 * ============================================================================
 */

MEMORY
{
    sysrom   (rx)  : ORIGIN = 0xC00000, LENGTH = 128K
    bios_ram (rwx) : ORIGIN = 0x10F400, LENGTH = 0x800
}

SECTIONS
{
    . = 0xC00000;

    .vectors :
    {
        KEEP(*(.vectors))
    } > sysrom

    . = 0xC00400;

    .jump_table 0xC00400 :
    {
        KEEP(*(.jump_table))
    } > sysrom

    . = 0xC004E0;

    .header 0xC004E0 :
    {
        KEEP(*(.header))
    } > sysrom

    . = 0xC00520;

    .text 0xC00520 :
    {
        *(.text)
        *(.text.*)
    } > sysrom

    .rodata :
    {
        *(.rodata)
        *(.rodata.*)
    } > sysrom

    .data :
    {
        __data_start = .;
        *(.data)
        *(.data.*)
        __data_end = .;
    } > bios_ram AT > sysrom
    __data_load = LOADADDR(.data);

    .bss (NOLOAD) :
    {
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        __bss_end = .;
    } > bios_ram

    /DISCARD/ :
    {
        *(.comment)
        *(.note.*)
        *(.eh_frame)
    }
}
ASSERT(SIZEOF(.vectors) == 0x400, "vector table size changed")
ASSERT(SIZEOF(.jump_table) == 0xE0, "BIOS service addresses changed")
ASSERT(SIZEOF(.header) <= 0x40, "BIOS header overlaps code")
ASSERT(__bss_end <= 0x10FC00, "BIOS private RAM overflow")
