/* objmove.c - Sprite/object movement with screen wrap.
 * Classic per-frame update loop: walk an array of moving objects, advance each
 * by its velocity, wrap around the Vectrex signed coordinate space (-128..127).
 * Exercises: struct array indexing, 16-bit add, signed compares/branches. */
#include "bench.h"

#define NOBJ 16

typedef struct {
    s16 x, y;     /* position (Q sub-pixel) */
    s8  dx, dy;   /* velocity */
} Obj;

Obj objs[NOBJ];

void obj_update(void)
{
    u8 i;
    for (i = 0; i < NOBJ; i++) {
        Obj *o = &objs[i];
        o->x += o->dx;
        o->y += o->dy;
        if (o->x >  127*16) o->x -= 256*16;
        if (o->x < -128*16) o->x += 256*16;
        if (o->y >  127*16) o->y -= 256*16;
        if (o->y < -128*16) o->y += 256*16;
    }
}
