#include "coco.h"


asm void
setSAMRegisters(byte *samAddr, byte value, byte numBits)
{
    asm
    {
        ldx     2,s             ; (samAddr) point to pair of bytes that target bit 0 of SAM register (X must be even)
        lda     5,s             ; (value) get bits to be written to SAM register
        ldb     7,s             ; (numBits) number of low bits of A to be written
@loop
; Bit 0 of A is the current bit to be written to the SAM register.
        bita    #1
        bne     @write1
        sta     ,x              ; write (any byte) at even address, to send a 0 to the SAM register
        bra     @bitWritten
@write1
        sta     1,x             ; write at odd address to write a 1 to the SAM register
@bitWritten
        lsra                    ; put next bit to write in bit 0 of A
        leax    2,x             ; point to next pair of bytes
        decb                    ; decrement bit count
        bne     @loop           ; branch if not finished
    }
}
