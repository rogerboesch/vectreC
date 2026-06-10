#include "coco.h"


byte palette(byte slot, byte color)
{
    #ifdef _COCO_BASIC_
    if (* (word *) 0xFFF8 != 0xFEF7)  // if not CoCo 3
        return FALSE;
    if (slot > 15)
        return FALSE;
    if (color > 63)
        return FALSE;
    byte *palette = (byte *) 0xFFB0;
    palette[slot] = color;
    return TRUE;
    #else
    return FALSE;
    #endif
}
