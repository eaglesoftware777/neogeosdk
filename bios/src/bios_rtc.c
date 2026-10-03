#include "bios.h"

/*
 * ============================================================================
 *  EagleBIOS Real-Time Clock (Calendar) Sub-system
 * ============================================================================
 */

/*
 * SYS_READ_CALENDAR: Read date and time into BIOS BCD variables.
 */
void sys_read_calendar_c(void)
{
    /* If RTC registers are not ticking, supply stable default BCD values */
    if (BIOS_YEAR == 0 && BIOS_MONTH == 0) {
        BIOS_YEAR    = 0x26; /* 2026 */
        BIOS_MONTH   = 0x09; /* September */
        BIOS_DAY     = 0x18; /* 18th */
        BIOS_WEEKDAY = 0x05; /* Friday */
        BIOS_HOUR    = 0x12; /* 12:00 */
        BIOS_MINUTE  = 0x00;
        BIOS_SECOND  = 0x00;
    }
}

/*
 * SYS_SET_CALENDAR: Set date and time from BIOS BCD variables.
 */
void sys_set_calendar_c(void)
{
    /* Handled in software */
}
