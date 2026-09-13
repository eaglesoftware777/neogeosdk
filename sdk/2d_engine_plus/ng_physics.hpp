#ifndef NG_PHYSICS_HPP
#define NG_PHYSICS_HPP

#include "ng_chars.hpp"

struct NGSolidRect {
    int16_t  x, y, w, h;
    uint16_t flags;
};

struct NGPhysicsBody {
    uint8_t  enabled;
    uint8_t  grounded;
    uint16_t flags;
    int16_t  prev_x;
    int16_t  prev_y;
    int32_t  gravity_fp;
    int32_t  max_fall_fp;
    int32_t  drag_x_fp;
    int32_t  drag_y_fp;
};

#define NG_PHYSICS_GRAVITY   0x0001
#define NG_PHYSICS_WORLD     0x0002
#define NG_PHYSICS_SOLIDS    0x0004
#define NG_PHYSICS_DRAG_X    0x0008
#define NG_PHYSICS_DRAG_Y    0x0010

/*
 * PhysicsWorld — singleton owning body array and solid rectangle list.
 */
class PhysicsWorld {
public:
    static PhysicsWorld& instance();

    void    NEOGEO_USER init();
    void    NEOGEO_USER attach(NGCharacter *c, uint16_t flags);
    void    NEOGEO_USER detach(NGCharacter *c);
    NGPhysicsBody* NEOGEO_USER body(NGCharacter *c);
    void    NEOGEO_USER setGravity(NGCharacter *c, int32_t gravity_fp, int32_t max_fall_fp);
    void    NEOGEO_USER setDrag(NGCharacter *c, int32_t drag_x_fp, int32_t drag_y_fp);
    void    NEOGEO_USER clearSolids();
    uint8_t NEOGEO_USER addSolid(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t flags);
    void    NEOGEO_USER updatePre();
    void    NEOGEO_USER resolve();
    uint8_t NEOGEO_USER isGrounded(NGCharacter *c);

private:
    PhysicsWorld() {}

    NGPhysicsBody bodies[NG_MAX_CHARS];
    NGSolidRect   solids[NG_MAX_SOLIDS];
    uint8_t       solid_count;

    static void applyDrag(int32_t *v, int32_t drag);
    static void syncPos(NGCharacter *c, int16_t x, int16_t y);
    static int16_t minI16(int16_t a, int16_t b);
    void resolveWorld(NGCharacter *c, NGPhysicsBody *body);
    void resolveSolid(NGCharacter *c, NGPhysicsBody *body, const NGSolidRect *solid);
};

#ifdef __cplusplus
extern "C" {
#endif

void           NEOGEO_USER ng_physics_init(void);
void           NEOGEO_USER ng_physics_attach(NGCharacter *c, uint16_t flags);
void           NEOGEO_USER ng_physics_detach(NGCharacter *c);
NGPhysicsBody* physics_body(NGCharacter *c);
void           NEOGEO_USER ng_physics_set_gravity(NGCharacter *c, int32_t gravity_fp, int32_t max_fall_fp);
void           NEOGEO_USER ng_physics_set_drag(NGCharacter *c, int32_t drag_x_fp, int32_t drag_y_fp);
void           NEOGEO_USER ng_physics_clear_solids(void);
uint8_t        NEOGEO_USER ng_physics_add_solid(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t flags);
void           NEOGEO_USER ng_physics_update_pre(void);
void           NEOGEO_USER ng_physics_resolve(void);
uint8_t        NEOGEO_USER ng_physics_is_grounded(NGCharacter *c);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
