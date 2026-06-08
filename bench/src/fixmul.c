/* fixmul.c - Q8.8 fixed-point vector scaling.
 * Scaling a list of vector endpoints by a fractional factor is the core of
 * zoom/explosion effects on the Vectrex. Uses a 16x16->32 multiply then a
 * shift back to Q8.8 - stresses each compiler's multiply/shift helpers. */
#include "bench.h"

#define N 16

s16 vin[N];
s16 vout[N];

void scale_q8(s16 factor)
{
    u8 i;
    for (i = 0; i < N; i++) {
        s32 p = (s32)vin[i] * factor;   /* Q8.8 * Q8.8 = Q16.16 */
        vout[i] = (s16)(p >> 8);        /* back to Q8.8 */
    }
}
