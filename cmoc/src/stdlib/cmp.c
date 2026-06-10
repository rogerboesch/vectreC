#include <coco.h>


const byte cmpPalette[16] = { 18, 36, 11, 7, 63, 31, 9, 38, 0, 18, 0, 63, 0, 18, 0, 38 };


void
cmp(void)
{
    memcpy((void *) 0xFFB0, cmpPalette, 16);
}
