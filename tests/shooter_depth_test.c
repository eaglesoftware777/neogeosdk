#include <assert.h>
#include <stdio.h>
#include "ng_depthfx.h"
/* Projection uses real engine code; these stand in for unrelated render
 * and starfield APIs retained by the SDK's single neogeo_user section. */
const uint8_t ng_shrink_tab[NGFX_SHRINK_ENTRIES]={255};
uint16_t ng_rand(void) { return 1u; }
void ng_sprite_group_init(NGSpriteGroup *g,uint16_t a,uint8_t b,uint8_t c,uint16_t d,uint8_t e)
{ (void)g;(void)a;(void)b;(void)c;(void)d;(void)e; }
void ng_sprite_group_set_tile_stride(NGSpriteGroup *g,uint16_t s) { (void)g;(void)s; }
void ng_sprite_group_set_palette_map(NGSpriteGroup *g,const uint8_t *p) { (void)g;(void)p; }
void ng_sprite_group_set_priority(NGSpriteGroup *g,uint8_t p) { (void)g;(void)p; }
void ng_sprite_group_set_pos(NGSpriteGroup *g,int16_t x,int16_t y) { (void)g;(void)x;(void)y; }
void ng_sprite_group_upload(NGSpriteGroup *g) { (void)g; }
void ng_sprite_group_flush(NGSpriteGroup *g) { (void)g; }
void ng_sprite_group_hide(NGSpriteGroup *g) { (void)g; }
void ng_sprite_group_update_transform(NGSpriteGroup *g) { (void)g; }
int main(void)
{
    NGShooterCamera c;
    NGShooterProjection p,far,near,ground,air;
    int i;
    ng_shooter_camera_init(&c);
    ng_shooter_camera_set_follow(&c,12,0,16,4);
    ng_shooter_camera_set_speed(&c,0);
    for(i=0;i<1000;i++) ng_shooter_camera_step(&c,260,112);
    assert(c.x_q8==12*256);
    far=ng_depthfx_shooter_project(&c,100,60,NG_SHOOTER_DISTANT_TERRAIN);
    near=ng_depthfx_shooter_project(&c,100,60,NG_SHOOTER_FOREGROUND);
    ground=ng_depthfx_shooter_project(&c,100,60,NG_SHOOTER_GROUND_OBJECT);
    air=ng_depthfx_shooter_project(&c,100,60,NG_SHOOTER_PLAYER_AIR);
    assert(far.x==97 && near.x==82 && ground.x==88 && air.x==100);
    assert(ground.scale==255 && air.scale==255);
    p=ng_depthfx_shooter_project(&c,100,60,NG_SHOOTER_SHADOW);
    assert(p.x==94 && p.scale==159 && p.render_band<air.render_band);
    c.x_q8=-12*256;
    assert(ng_depthfx_shooter_project(&c,100,60,NG_SHOOTER_FOREGROUND).x==118);
    c.x_q8=255;
    assert(ng_depthfx_shooter_project(&c,100,60,NG_SHOOTER_GROUND_OBJECT).x==100);
    c.x_q8=-255;
    assert(ng_depthfx_shooter_project(&c,100,60,NG_SHOOTER_GROUND_OBJECT).x==100);
    p=ng_depthfx_shooter_project(0,100,60,255);
    assert(p.x==100 && p.y==60 && p.scale==255);
    puts("Shooter depth: bounded camera, asymmetric parallax weights, stable scales, shadow priority and mirrored fractions passed");
    return 0;
}
