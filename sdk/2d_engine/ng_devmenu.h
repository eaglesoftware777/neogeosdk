/*
 * ng_devmenu.h — A developer's test menu over the running game
 *
 * Held START and pressed D (by default) opens a small menu on the FIX layer and
 * freezes the game behind it:
 *
 *   SPEED      FULL, 1/2, 1/4, 1/8, or STEP (each press of D runs one frame)
 *   TIMER      RUNS or STOPPED          (the game reads ng_dev.timer_stop)
 *   CAMERA     GAME or FREE             (ng_dev.free_cam; the stick moves
 *                                        ng_dev.cam_x / cam_y while it is closed)
 *   HIT BOXES  OFF or ON                (ng_dev.hitboxes, for the game to draw)
 *   WATCH      four RAM addresses and their words, live
 *   SOUND      a sound code: UP/DOWN +-1, LEFT/RIGHT +-10, A plays, B stops
 *
 * In the menu: UP/DOWN a row, LEFT/RIGHT change it, A opens WATCH or SOUND, B
 * goes back and closes. Nothing here changes the game by itself: the game reads
 * the switches it wants to honour, and asks ng_dev_frame each frame whether to
 * run its update:
 *
 *   for (;;) {
 *       uint16_t held = ..., pressed = ...;
 *       if (ng_dev_frame(held, pressed)) game_update(held, pressed);
 *       game_draw();
 *       wait_vblank();
 *   }
 *
 * The menu draws on rows the game may use; on closing it calls cfg->on_close so
 * the game can redraw them. A release build need not link it at all: guard the
 * calls with the game's own debug switch.
 */

#ifndef NG_DEVMENU_H
#define NG_DEVMENU_H

#include <stdint.h>
#include "macro.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t x, y;                          /* the menu's top-left FIX cell */
    uint8_t pal, pal_hi;                   /* text, and the chosen row */
    uint16_t hold, press;                  /* held + pressed opens and closes it (0, 0: START + D) */
    uint16_t up, down, left, right, a, b, d;   /* the button bits (0: the SDK's P1 layout) */
    void (*play_sound)(uint16_t code);     /* NULL: the SDK's playSoundtest */
    void (*stop_sound)(void);              /* NULL: soundStopAll */
    void (*on_close)(void);                /* redraw what the menu covered; may be NULL */
} NGDevConfig;

typedef struct {
    uint8_t open;
    uint8_t page, row;
    uint8_t speed;            /* 0 full, 1 half, 2 quarter, 3 eighth, 4 step */
    uint8_t timer_stop;
    uint8_t free_cam;
    uint8_t hitboxes;
    int16_t cam_x, cam_y;
    uint32_t watch[4];
    uint16_t sound;
    uint16_t frame;           /* frames since ng_dev_init, frozen or not */
} NGDev;

extern NGDev ng_dev;

void NEOGEO_USER ng_dev_init(const NGDevConfig *cfg);

/* Once a frame with player 1's buttons. Returns 1 when the game should run its
 * update this frame: 0 while the menu is open, on the frames a slow speed skips,
 * and between steps. */
uint8_t NEOGEO_USER ng_dev_frame(uint16_t held, uint16_t pressed);

#ifdef __cplusplus
}
#endif

#endif
