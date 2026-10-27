"""Exercise the game's actual bubble drawing function with position-recording ports."""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class BubbleRenderTests(unittest.TestCase):
    def test_shifted_rows_reposition_even_when_cell_colors_stay_the_same(self):
        source = (ROOT / 'games/sevenwonders/scenes/sw_draw.c').read_text()
        draw = source[source.index('static void NEOGEO_USER draw_bubbles('):
                      source.index('\nvoid NEOGEO_USER sw_board_draw(')]
        fixture = r'''
#include <assert.h>
#include <string.h>
#include "sdk/2d_engine/ng_puzzle.h"
#include "sdk/2d_engine/ng_sprite_group.h"
#define FALLS 24u
#define SW_ART_BUBBLES 1u
static NGSpriteGroup s_cell[120], s_fall[FALLS], s_shot, s_next;
static uint8_t s_drawn_color[120], s_drawn_kind[120], s_group[15], s_fall_on[FALLS];
static uint8_t s_drawn_parity;
static uint8_t s_bound=1;
static NGPuzzle *s_p;
static NGPuzzlePreview s_aim_preview;
static uint32_t s_aim_generation;
#define DOTS 12u
static NGSpriteGroup s_launch, s_dot[DOTS];
static unsigned previews;
static uint8_t counted_begin(NGPuzzlePreview *q, const NGPuzzle *p, uint16_t a, uint32_t gen) {
    previews++; return ng_puzzle_preview_begin(q,p,a,gen);
}
static int16_t s_fall_v[FALLS], s_fall_y[FALLS], s_fall_x[FALLS], s_ox, s_oy;
static void effect(int16_t x, int16_t y) { (void)x; (void)y; }
static void drop_bubble(const NGPuzzle *p, uint8_t c, uint8_t col) { (void)p; (void)c; (void)col; }
static void set_frame(NGSpriteGroup *g, uint8_t id, uint8_t frame) { (void)g; (void)id; (void)frame; }
static uint8_t bubble_frame(uint8_t col, uint8_t kind) { (void)kind; return col; }
static void bubble_xy(const NGPuzzle *p, uint8_t cell, int16_t *x, int16_t *y) {
    ng_puzzle_cell_px(p, cell, 0, x, y); *x += s_ox; *y += s_oy;
}
void ng_sprite_group_set_pos(NGSpriteGroup *g, int16_t x, int16_t y) { g->x=x; g->y=y; }
void ng_sprite_group_set_visible(NGSpriteGroup *g, uint8_t v) { g->visible=v; }
void ng_sprite_group_flush(NGSpriteGroup *g) { (void)g; }
'''
        fixture += draw
        fixture += '\n#define ng_puzzle_preview_begin counted_begin\n'
        fixture += source[source.index('void NEOGEO_USER sw_aim('):]
        fixture += '\n#undef ng_puzzle_preview_begin\n'
        fixture += r'''
int main(void) {
    NGPuzzleLevel lv;
    static NGPuzzle p;
    int16_t old[3];
    unsigned i;
    memset(&lv, 0, sizeof(lv));
    lv.mode=NGP_MODE_BUBBLE; lv.w=10; lv.h=12; lv.colors=4;
    lv.bubble_d=16; lv.row_h=14; lv.loss_row=11;
    lv.launch_x=80; lv.launch_y=196; lv.shot_speed_q8=1536;
    assert(ng_puzzle_init(&p,&lv,1)==NGP_OK);
    s_p=&p;
    p.color[0]=1; p.color[10]=1; p.color[20]=1;
    memset(s_drawn_color,255,sizeof(s_drawn_color));
    s_drawn_parity=255; s_ox=132; s_oy=6;
    draw_bubbles(&p);
    for(i=0;i<3;i++) { old[i]=s_cell[i*10].x; assert(s_cell[i*10].visible); }
    /* New-row insertion toggles lattice parity. These repeated-color cells
     * can keep identical data while their physical columns shift by 8px. */
    p.parity=1;
    draw_bubbles(&p);
    for(i=0;i<3;i++) {
        assert(s_cell[i*10].x==old[i]+(i==1?-8:8));
        assert(s_cell[i*10].y==6+(int)i*14);
    }
    p.parity=0;
    draw_bubbles(&p);
    for(i=0;i<3;i++) assert(s_cell[i*10].x==old[i]);
    /* Repeated draws use one unchanged prediction, including guide toggle.
     * Actual board/aim/parity changes must run the real geometry again. */
    sw_aim(&p,1,1); assert(previews==1);
    for(i=0;i<100;i++) {
        uint16_t steps=s_aim_preview.steps;
        sw_aim(&p,1,1);
        assert(s_aim_preview.steps-steps<=NGP_PREVIEW_MAX_STEPS);
    }
    assert(previews==1);
    assert(s_aim_preview.state==NGP_PREVIEW_READY);
    sw_aim(&p,1,0); sw_aim(&p,1,1); assert(previews==1);
    p.angle-=6; sw_aim(&p,1,1); assert(previews==2);
    p.color[30]=2; draw_bubbles(&p); sw_aim(&p,1,1); assert(previews==3);
    p.parity=1; draw_bubbles(&p); sw_aim(&p,1,1); assert(previews==4);
    s_aim_generation++; sw_aim(&p,1,1); assert(previews==5);
    return 0;
}
'''
        with tempfile.TemporaryDirectory() as directory:
            src, exe = Path(directory) / 'draw.c', Path(directory) / 'draw'
            src.write_text(fixture)
            subprocess.run(['cc', '-std=c99', '-I', str(ROOT), '-I', str(ROOT / 'sdk'),
                            '-I', str(ROOT / 'sdk/2d_engine'), str(src),
                            *[str(ROOT / 'sdk/2d_engine' / f'{name}.c')
                              for name in ('ng_puzzle', 'ng_trig', 'ng_trig_table')],
                            '-o', str(exe)], check=True)
            subprocess.run([str(exe)], check=True, capture_output=True)
