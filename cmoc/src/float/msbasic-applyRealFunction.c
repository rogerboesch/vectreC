#include "cmoc-stdlib-private.h"


asm void
_CMOC_applyRealFunction(void *fpa0Transform, float *p)
{
    asm
    {
        pshs    u,y
;
; Stack at this point:
; 0..1,s: Saved Y (CMOC global data pointer).
; 2..3,s: Saved U (CMOC frame pointer).
; 4..5,s: Return address.
; 6..7,s: fpa0Transform: Color Basic routine that actgs on FPA0.
; 8..9,s: p: Points to argument, which gets replaced with result.
;
        ldx     8,s             ; p: point X to the float argument
        jsr     _CMOC_ecb_or_dgn(0xBC14, 0x93BF)  ; load FPA0 from X
        jsr     [6,s]           ; call routine at fpa0Transform
        ldu     2,s             ; restore frame pointer
        ldx     8,s             ; p
        jsr     _CMOC_ecb_or_dgn(0xBC35, 0x93E0)    ; pack FPA0 into *p
        puls    y,u
    }
}
