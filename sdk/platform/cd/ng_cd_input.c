#include "ng_cd.h"
#include "neogeo.h"

/* Word accesses are atomic on the 68000. Only VBlank samples input; game
 * reads cannot lose an edge if firmware also polls during IRQ completion. */
static volatile uint16_t current_pad, pressed_pad;

void ng_cd_input_sample(uint16_t current)
{
    pressed_pad=(uint16_t)(current & ~current_pad);
    current_pad=current;
}

uint16_t NEOGEO_USER poll_joystick(void) { return current_pad; }
uint16_t NEOGEO_USER poll_joystick_edge(void) { return pressed_pad; }
