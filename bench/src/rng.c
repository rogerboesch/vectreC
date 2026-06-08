/* rng.c - 16-bit xorshift pseudo-random generator filling a buffer.
 * Games use a cheap PRNG for star fields, enemy spawns, etc.
 * Exercises: 16-bit shifts and xor, tight store loop. */
#include "bench.h"

#define N 32

u8 noise[N];
u16 rng_state = 0xACE1u;

void rng_fill(void)
{
    u16 x = rng_state;
    u8 i;
    for (i = 0; i < N; i++) {
        x ^= (u16)(x << 7);
        x ^= (u16)(x >> 9);
        x ^= (u16)(x << 8);
        noise[i] = (u8)x;
    }
    rng_state = x;
}
