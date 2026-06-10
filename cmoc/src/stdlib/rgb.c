#include <coco.h>


const byte rgbPalette[16] = { 18, 54, 9, 36, 63, 27, 45, 38, 0, 18, 0, 63, 0, 18, 0, 38 };


void
rgb(void)
{
    memcpy((void *) 0xFFB0, rgbPalette, 16);
}
