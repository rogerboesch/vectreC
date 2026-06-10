#include "coco.h"

#include "cmoc-stdlib-private.h"


void sound(byte tone, byte duration)
{
    asm
    {
        pshs    u             // protect U from Basic code
        lda     :tone
        sta     >$008C
        ldb     :duration
        jsr     _CMOC_ecb_or_dgn(0xA951, 0xBAA0)
        puls    u
    }
}
