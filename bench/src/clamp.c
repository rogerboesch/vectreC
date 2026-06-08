/* clamp.c - Clamp a draw vector to the signed Vectrex beam range.
 * The DAC takes signed 8-bit deltas; relative moves must be clamped so the beam
 * stays on screen. Exercises: signed min/max via branches on 16-bit inputs. */
#include "bench.h"

s8 clamp8(s16 v)
{
    if (v >  127) v =  127;
    if (v < -128) v = -128;
    return (s8)v;
}

void clamp_vlist(const s16 *xin, const s16 *yin, s8 *xout, s8 *yout, u8 n)
{
    u8 i;
    for (i = 0; i < n; i++) {
        xout[i] = clamp8(xin[i]);
        yout[i] = clamp8(yin[i]);
    }
}
