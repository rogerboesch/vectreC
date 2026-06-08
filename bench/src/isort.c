/* isort.c - Insertion sort of a small byte array.
 * Vectrex line-drawing wants shapes sorted by depth/intensity so the beam moves
 * efficiently; small in-place sorts run every frame.
 * Exercises: nested loop, array shifting, signed compares. */
#include "bench.h"

#define N 16

s8 keys[N];

void isort(void)
{
    u8 i, j;
    for (i = 1; i < N; i++) {
        s8 k = keys[i];
        j = i;
        while (j > 0 && keys[j - 1] > k) {
            keys[j] = keys[j - 1];
            j--;
        }
        keys[j] = k;
    }
}
