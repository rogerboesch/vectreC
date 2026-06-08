/* collide.c - Axis-aligned bounding-box collision counting.
 * Brute-force O(n^2) overlap test over a list of sprites, the usual approach
 * for the handful of objects a Vectrex game tracks.
 * Exercises: nested loops, signed 8-bit arithmetic, short-circuit branches. */
#include "bench.h"

#define NS 12

s8 sx[NS], sy[NS];   /* sprite centers */
s8 sw[NS], sh[NS];   /* half extents   */

u8 collide_count(void)
{
    u8 i, j, n = 0;
    for (i = 0; i < NS; i++) {
        for (j = (u8)(i + 1); j < NS; j++) {
            s8 dx = (s8)(sx[i] - sx[j]);
            s8 dy = (s8)(sy[i] - sy[j]);
            if (dx < 0) dx = (s8)-dx;
            if (dy < 0) dy = (s8)-dy;
            if (dx < (s8)(sw[i] + sw[j]) && dy < (s8)(sh[i] + sh[j]))
                n++;
        }
    }
    return n;
}
