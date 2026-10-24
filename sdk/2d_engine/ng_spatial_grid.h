#ifndef NG_SPATIAL_GRID_H
#define NG_SPATIAL_GRID_H
#include <stdint.h>

/* Screen-local broad phase: 64px cells, 384x320 coverage, <=16 fixed IDs.
 * Bounds are half-open. Candidates still require the caller's exact AABB test.
 * No allocation, pointer ownership, sprite state or physics precision changes. */
enum { NG_GRID_COLUMNS=6, NG_GRID_ROWS=5, NG_GRID_CELL_SHIFT=6 };
typedef struct {
    int16_t x, y;
    uint16_t cell[NG_GRID_COLUMNS*NG_GRID_ROWS];
} NGSpatialGrid;

static inline void ng_spatial_grid_reset(NGSpatialGrid *g, int16_t x, int16_t y)
{
    uint8_t i;
    g->x=x; g->y=y;
    for (i=0;i<NG_GRID_COLUMNS*NG_GRID_ROWS;++i) g->cell[i]=0;
}

static inline uint8_t ng_spatial_grid_bounds(const NGSpatialGrid *g,
    int16_t left,int16_t top,int16_t right,int16_t bottom,int16_t *b)
{
    if (right<=left || bottom<=top || right<=g->x || bottom<=g->y ||
        left>=g->x+384 || top>=g->y+320) return 0;
    b[0]=left<=g->x ? 0 : (int16_t)((left-g->x)>>6);
    b[1]=top<=g->y ? 0 : (int16_t)((top-g->y)>>6);
    b[2]=right>=g->x+384 ? 5 : (int16_t)((right-g->x-1)>>6);
    b[3]=bottom>=g->y+320 ? 4 : (int16_t)((bottom-g->y-1)>>6);
    return 1;
}

static inline void ng_spatial_grid_insert(NGSpatialGrid *g,uint8_t id,
    int16_t left,int16_t top,int16_t right,int16_t bottom)
{
    int16_t b[4],x,y;
    uint16_t bit;
    if (id>=16u || !ng_spatial_grid_bounds(g,left,top,right,bottom,b)) return;
    bit=(uint16_t)(1u<<id);
    for (y=b[1];y<=b[3];++y)
        for (x=b[0];x<=b[2];++x) g->cell[y*6+x]|=bit;
}

static inline uint16_t ng_spatial_grid_query(const NGSpatialGrid *g,
    int16_t left,int16_t top,int16_t right,int16_t bottom)
{
    int16_t b[4],x,y;
    uint16_t result=0;
    if (!ng_spatial_grid_bounds(g,left,top,right,bottom,b)) return 0;
    for (y=b[1];y<=b[3];++y)
        for (x=b[0];x<=b[2];++x) result|=g->cell[y*6+x];
    return result;
}
#endif
