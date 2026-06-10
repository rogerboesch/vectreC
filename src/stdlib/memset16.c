#include "cmoc.h"


asm void *
memset16(void *dest, unsigned short w, size_t num16BitWords)
{
    asm
    {
        ldx     2,s         ; dest
        ldd     6,s         ; num16BitWords
        lslb
        rola                ; D = num16BitWords * 2 = number of bytes to write
        leax    d,x         ; end of region to write to
        ldd     4,s         ; word to write (w)
@loop
        std     ,--x
        cmpx    2,s         ; back to dest?
        bhi     @loop
        tfr     x,d         ; return start address
    }
}
