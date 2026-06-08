/* checksum.c - ROM checksum (sum + xor rolling hash) over a byte range.
 * Cartridge self-tests and high-score table integrity checks use loops like
 * this. Exercises: 16-bit accumulate, 8-bit rotate-via-shift, load loop. */
#include "bench.h"

u16 rom_check(const u8 *p, u16 n)
{
    u16 sum = 0;
    u8 h = 0;
    while (n--) {
        u8 b = *p++;
        sum = (u16)(sum + b);
        h = (u8)((h << 1) | (h >> 7));   /* rotate left 1 */
        h ^= b;
    }
    return (u16)(sum ^ ((u16)h << 8));
}
