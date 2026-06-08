/* memops.c - Hand-rolled memcpy and memset over a frame buffer.
 * Vector/text double-buffering shuffles bytes every frame; these byte loops
 * are some of the most-executed code in a Vectrex title.
 * Exercises: pointer post-increment loops, byte loads/stores. */
#include "bench.h"

void buf_copy(u8 *dst, const u8 *src, u8 n)
{
    while (n--) *dst++ = *src++;
}

void buf_set(u8 *dst, u8 v, u8 n)
{
    while (n--) *dst++ = v;
}
