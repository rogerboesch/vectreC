#include "coco.h"


extern byte _CMOC_pmodeNo;
extern byte *_CMOC_pmodeScreenBuffer;


// Uses _CMOC_pmodeNo and _CMOC_pmodeScreenBuffer.
//
void pcls(byte byteToClearWith)
{
    word size;
    switch (_CMOC_pmodeNo)
    {
    case 4:
    case 3:
        size = 6144;
        break;
    case 2:
    case 1:
        size = 3072;
        break;
    default:
        size = 1536;
    }

    word *end = (word *) (_CMOC_pmodeScreenBuffer + size);

    asm {
        lda     :byteToClearWith
        tfr     a,b
        ldx     :_CMOC_pmodeScreenBuffer
@pcls_loop:
        std     ,x++
        std     ,x++
        std     ,x++
        std     ,x++
        std     ,x++
        std     ,x++
        std     ,x++
        std     ,x++
        std     ,x++
        std     ,x++
        std     ,x++
        std     ,x++
        std     ,x++
        std     ,x++
        std     ,x++
        std     ,x++
        cmpx    :end
        bne     @pcls_loop
    }
}
