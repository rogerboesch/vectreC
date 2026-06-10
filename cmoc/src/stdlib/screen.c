#include <coco.h>


extern byte _CMOC_pmodeNo;
extern byte *_CMOC_pmodeScreenBuffer;


// Uses _CMOC_pmodeNo and _CMOC_pmodeScreenBuffer if type == 1 (graphics).
//
void
screen(byte type, byte colorset)
{
    byte *pia1bData = (byte *) 0xff22;
    byte b = *pia1bData;
    if (type)  // if graphics
    {
        b = ((_CMOC_pmodeNo + 3) << 4) | 0x80 | (b & 0x07);
        byte samVRegisterValue = (_CMOC_pmodeNo == 4 ? 6 : _CMOC_pmodeNo + 3);
        showGraphicsAddress(samVRegisterValue, (byte) ((word) _CMOC_pmodeScreenBuffer / 512));
    }
    else  // text mode
    {
        b &= 0x17;  // preserve bit 4 (0x10) to avoid resetting the true lowercase mode if it is on
        showLowResTextAddress();
    }
    if (colorset)
        b |= 8;
    else
        b &= ~8;
    *pia1bData = b;
}
