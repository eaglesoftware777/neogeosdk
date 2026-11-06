#include <assert.h>
#include "sdk/ng_input_repeat.h"

int main(void)
{
    NGInputRepeat x = {0, 0}, y = {0, 0};
    unsigned i;
    assert(ng_input_repeat_step(&x, 4, 15, 4) == 4);
    for (i = 1; i < 15; i++) assert(ng_input_repeat_step(&x, 4, 15, 4) == 0);
    assert(ng_input_repeat_step(&x, 4, 15, 4) == 4);
    /* A direction reversal starts its own full delay, regardless of hold. */
    assert(ng_input_repeat_step(&x, 8, 15, 4) == 8);
    for (i = 1; i < 15; i++) assert(ng_input_repeat_step(&x, 8, 15, 4) == 0);
    assert(ng_input_repeat_step(&x, 8, 15, 4) == 8);
    /* Adding/reversing the other axis does not reset this one's repeat. */
    for (i = 1; i <= 1000000u; i++) {
        assert(ng_input_repeat_step(&x, 8, 15, 4) == (i % 4u ? 0u : 8u));
        if (i % 5u == 0) (void)ng_input_repeat_step(&y, 1, 15, 4);
        else (void)ng_input_repeat_step(&y, 2, 15, 4);
    }
    assert(ng_input_repeat_step(&x, 0, 15, 4) == 0);
    assert(ng_input_repeat_step(&x, 8, 15, 4) == 8);
    assert(ng_input_repeat_step(&x, 0, 0, 0) == 0);
    assert(ng_input_repeat_step(&x, 4, 0, 0) == 4);
    for (i = 0; i < 1000; i++) assert(ng_input_repeat_step(&x, 4, 0, 0) == 0);
    assert(ng_input_repeat_step(&x, 0, 0, 4) == 0);
    assert(ng_input_repeat_step(&x, 4, 0, 4) == 4);
    for (i = 1; i < 4; i++) assert(ng_input_repeat_step(&x, 4, 0, 4) == 0);
    assert(ng_input_repeat_step(&x, 4, 0, 4) == 4);
    return 0;
}
