#include "bios.h"

/*
 * ============================================================================
 *  EagleBIOS Memory Card Sub-system
 * ============================================================================
 */

/*
 * SYS_CARD: Memory card access routine.
 * Function requested in BIOS_CRDF:
 *   0x00: Format card
 *   0x01: Read file
 *   0x02: Write file
 *   0x03: Delete file
 * Result returned in BIOS_CRDRESULT:
 *   0x00: Success
 *   0x80: Memory card not inserted
 */
void sys_card_c(void)
{
    /* Check if memory card is inserted (REG_STATUS_A bit 5 or similar) */
    /* Return 0x80: not inserted by default in headless / standard setups */
    BIOS_CRDRESULT = 0x80u;
}

/*
 * SYS_CARD_ERROR: Display memory card error message.
 */
void sys_card_error_c(void)
{
    /* Quick return */
}
