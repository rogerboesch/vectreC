#include "coco.h"


byte
resetPalette(byte isRGB)
{
#ifdef _COCO_BASIC_
    if (* (word *) 0xFFF8 != 0xFEF7)  // if not CoCo 3
        return FALSE;

    asm { sync }  // copy during the v-sync, to prevent sparking
    if (isRGB)
        rgb();
    else
        cmp();
    return TRUE;
#else
    return FALSE;  // not supported
#endif
}
