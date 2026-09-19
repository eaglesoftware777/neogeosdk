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

    .jump_table :
    {
        KEEP(*(.jump_table))
    } > sysrom

    . = 0xC004E0;

    .header :
    {
        KEEP(*(.header))
    } > sysrom

    . = 0xC00500;

    .text :
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
        *(.data)
        *(.data.*)
    } > bios_ram AT > sysrom

    .bss (NOLOAD) :
    {
        *(.bss)
        *(.bss.*)
        *(COMMON)
    } > bios_ram

    /DISCARD/ :
    {
        *(.comment)
        *(.note.*)
        *(.eh_frame)
    }
}
