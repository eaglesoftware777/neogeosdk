#include "bios.h"

/*
 * ============================================================================
 *  EagleBIOS Real-Time Clock (Calendar) Sub-system
 * ============================================================================
 *
 * Full BCD RTC implementation for uPD4990A and AES software RTC:
 * Handles leap years, valid days per month, weekday computation,
 * and VBlank tick based time progression.
 */

static uint32_t s_last_vbl_tick = 0;

static uint8_t bcd_to_bin(uint8_t b)
{
    return (uint8_t)(((b >> 4) * 10u) + (b & 0x0Fu));
}

static uint8_t bin_to_bcd(uint8_t v)
{
    uint8_t tens = 0;
    while (v >= 10u) {
        v -= 10u;
        tens++;
    }
    return (uint8_t)((tens << 4) | v);
}

static uint8_t is_leap_year(uint16_t year)
{
    return (uint8_t)((year % 4u == 0u && year % 100u != 0u) || (year % 400u == 0u));
}

static uint8_t days_in_month(uint16_t year, uint8_t month)
{
    static const uint8_t k_days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2u && is_leap_year(year)) return 29u;
    if (month >= 1u && month <= 12u) return k_days[month - 1u];
    return 30u;
}

static uint8_t calc_weekday(uint16_t y, uint8_t m, uint8_t d)
{
    static const int16_t t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    int16_t iy = (int16_t)y;
    if (m < 3u) iy = (int16_t)(iy - 1);
    int16_t res = (int16_t)(iy + iy / 4 - iy / 100 + iy / 400 + t[m - 1u] + d);
    res %= 7;
    if (res < 0) res += 7;
    return (uint8_t)res;
}

static void advance_seconds(uint16_t sec_elapsed)
{
    uint16_t full_year = (uint16_t)(2000u + bcd_to_bin(BIOS_YEAR));
    uint8_t mon = bcd_to_bin(BIOS_MONTH);
    uint8_t day = bcd_to_bin(BIOS_DAY);
    uint8_t hr = bcd_to_bin(BIOS_HOUR);
    uint8_t min = bcd_to_bin(BIOS_MINUTE);
    uint8_t sec = (uint8_t)(bcd_to_bin(BIOS_SECOND) + sec_elapsed);

    while (sec >= 60u) {
        sec -= 60u;
        min++;
    }
    while (min >= 60u) {
        min -= 60u;
        hr++;
    }
    while (hr >= 24u) {
        hr -= 24u;
        day++;
    }
    while (day > days_in_month(full_year, mon)) {
        day = (uint8_t)(day - days_in_month(full_year, mon));
        mon++;
        if (mon > 12u) {
            mon = 1u;
            full_year++;
        }
    }

    BIOS_SECOND = bin_to_bcd(sec);
    BIOS_MINUTE = bin_to_bcd(min);
    BIOS_HOUR = bin_to_bcd(hr);
    BIOS_DAY = bin_to_bcd(day);
    BIOS_MONTH = bin_to_bcd(mon);
    BIOS_YEAR = bin_to_bcd((uint8_t)(full_year % 100u));
    BIOS_WEEKDAY = calc_weekday(full_year, mon, day);
}

/*
 * SYS_READ_CALENDAR: Read date and time into BIOS BCD variables.
 */
void sys_read_calendar_c(void)
{
    if (BIOS_YEAR == 0 && BIOS_MONTH == 0) {
        BIOS_YEAR    = 0x26; /* 2026 */
        BIOS_MONTH   = 0x10; /* October */
        BIOS_DAY     = 0x12; /* 12th */
        BIOS_WEEKDAY = 0x01; /* Monday */
        BIOS_HOUR    = 0x12; /* 12:00 */
        BIOS_MINUTE  = 0x00;
        BIOS_SECOND  = 0x00;
        s_last_vbl_tick = BIOS_VBL_TICK;
        return;
    }

    uint32_t current_tick = BIOS_VBL_TICK;
    uint32_t diff = current_tick - s_last_vbl_tick;
    while (diff >= 60u) {
        advance_seconds(1u);
        diff -= 60u;
        s_last_vbl_tick += 60u;
    }
}

/*
 * SYS_SET_CALENDAR: Set date and time from BIOS BCD variables.
 */
void sys_set_calendar_c(void)
{
    uint16_t full_year = (uint16_t)(2000u + bcd_to_bin(BIOS_YEAR));
    uint8_t mon = bcd_to_bin(BIOS_MONTH);
    uint8_t day = bcd_to_bin(BIOS_DAY);
    uint8_t hr = bcd_to_bin(BIOS_HOUR);
    uint8_t min = bcd_to_bin(BIOS_MINUTE);
    uint8_t sec = bcd_to_bin(BIOS_SECOND);

    if (mon < 1u || mon > 12u) mon = 1u;
    uint8_t max_d = days_in_month(full_year, mon);
    if (day < 1u || day > max_d) day = (day < 1u) ? 1u : max_d;
    if (hr >= 24u) hr = 23u;
    if (min >= 60u) min = 59u;
    if (sec >= 60u) sec = 59u;

    BIOS_YEAR = bin_to_bcd((uint8_t)(full_year % 100u));
    BIOS_MONTH = bin_to_bcd(mon);
    BIOS_DAY = bin_to_bcd(day);
    BIOS_WEEKDAY = calc_weekday(full_year, mon, day);
    BIOS_HOUR = bin_to_bcd(hr);
    BIOS_MINUTE = bin_to_bcd(min);
    BIOS_SECOND = bin_to_bcd(sec);
    s_last_vbl_tick = BIOS_VBL_TICK;
}
