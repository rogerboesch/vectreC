#include <coco.h>


extern byte _CMOC_pmodeNo;
extern byte *_CMOC_pmodeScreenBuffer;


// Sets _CMOC_pmodeNo and _CMOC_pmodeScreenBuffer.
//
BOOL
pmode(byte mode, byte *screenBuffer)
{
    if (mode > 4 || ((word) screenBuffer & 0x01FF) != 0)
        return FALSE;
    _CMOC_pmodeNo = mode;
    _CMOC_pmodeScreenBuffer = screenBuffer;
    return TRUE;
}
