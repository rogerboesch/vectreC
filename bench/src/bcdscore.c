/* bcdscore.c - Add a value to a packed BCD score (3 bytes = 6 digits).
 * Many Vectrex/arcade games keep score in BCD so it can be printed digit by
 * digit. Exercises: nibble arithmetic, carry propagation, byte array walk. */
#include "bench.h"

u8 score_bcd[3];   /* score_bcd[0] = most significant pair */

void bcd_add(u8 add)
{
    s8 i;
    u8 carry = add;
    for (i = 2; i >= 0; i--) {
        u8 lo = (u8)((score_bcd[i] & 0x0F) + (carry & 0x0F));
        u8 hi = (u8)((score_bcd[i] >> 4) + (carry >> 4));
        carry = 0;
        if (lo > 9)  { lo = (u8)(lo - 10); hi++; }
        if (hi > 9)  { hi = (u8)(hi - 10); carry = 1; }
        score_bcd[i] = (u8)((hi << 4) | lo);
        if (!carry) break;
    }
}
