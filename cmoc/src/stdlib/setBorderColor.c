#include "coco.h"


byte setBorderColor(byte color)
{
    #ifdef _COCO_BASIC_
    if (* (word *) 0xFFF8 != 0xFEF7)  // if not CoCo 3
        return FALSE;

    * (byte *) 0xFF9A = color;
    return TRUE;
    #else
    return FALSE;
    #endif
}
