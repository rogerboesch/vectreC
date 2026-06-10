#include <coco.h>


extern byte *_CMOC_pmodeScreenBuffer;


void
setPmodeGraphicsAddressEx(byte *newPmodeScreenBuffer, BOOL makeVisible)
{
    _CMOC_pmodeScreenBuffer = newPmodeScreenBuffer;
    if (makeVisible)
        setSAMRegisters((byte *) 0xFFC6, (byte) ((word) _CMOC_pmodeScreenBuffer / 512), 7);
}
