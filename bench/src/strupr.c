/* strupr.c - Copy a string while upper-casing it.
 * The Vectrex BIOS print routines want upper-case ASCII; games often massage
 * score/name strings before drawing them.
 * Exercises: byte loop with a range compare and conditional add. */
#include "bench.h"

void str_upper(char *dst, const char *src)
{
    char c;
    while ((c = *src++) != 0) {
        if (c >= 'a' && c <= 'z')
            c = (char)(c - 32);
        *dst++ = c;
    }
    *dst = 0;
}
